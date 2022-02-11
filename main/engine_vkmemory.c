#include "engine_vkmemory.h"

bool vkmemory_init(struct VulkanMemory *vmem, VkPhysicalDevice physical_device, VkDevice device,
				   uint32_t gfx_index, uint32_t tfr_index) {
	vmem->physical_device = physical_device;
	vmem->device = device;
	vmem->gfx_index = gfx_index;
	vmem->tfr_index = tfr_index;
	vmem->allocation = NULL;

	pthread_mutex_init(&vmem->allocation_lock, NULL);

	return true;
}

bool vkmemory_destroy(struct VulkanMemory *vmem) {
	// Get lock
	pthread_mutex_lock(&vmem->allocation_lock);

	// Free all buffers and memory
	struct VulkanAllocation *curr = vmem->allocation, *next;
	struct VulkanBuffer *bcurr, *bnext;

	while (curr != NULL) {
		bcurr = curr->buffers;
		while (bcurr != NULL) {
			vkDestroyBuffer(vmem->device, bcurr->buffer, NULL);

			bnext = bcurr->next;

			free(bcurr);
			bcurr = bnext;
		}

		vkFreeMemory(vmem->device, curr->mem, NULL);
		next = curr->next;

		free(curr);
		curr = next;
	}

	// Destroy lock
	pthread_mutex_unlock(&vmem->allocation_lock);
	pthread_mutex_destroy(&vmem->allocation_lock);

	return true;
}

bool vkmemory_createbuffer(struct VulkanMemory *vmem, VkDeviceSize buff_size,
						   VkBufferUsageFlags usage, VkMemoryPropertyFlags properties,
						   struct VulkanBuffer **struct_buff) {
	// Get lock
	pthread_mutex_lock(&vmem->allocation_lock);

	if (enable_validation_layers) {
		printf("Creating GPU buffer... size = %llu\n", buff_size);
	}

	// Check size
	if (buff_size > VK_ALLOC_BLOCK_SIZE) {
		fprintf(stderr, "Trying to allocate a chunk of memory too big.\n");
		return false;
	}

	// Create buffer before allocation
	VkBufferCreateInfo buffer_info = {0};

	uint32_t queue_indices[2] = {vmem->gfx_index, vmem->tfr_index};

	buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	buffer_info.size = buff_size;
	buffer_info.usage = usage;
	/* Set to concurrent
	   (non-optimal for mobile GPUs)
	   Would require ownership transfer if exclusive */
	buffer_info.sharingMode = VK_SHARING_MODE_CONCURRENT;
	buffer_info.queueFamilyIndexCount = 2;
	buffer_info.pQueueFamilyIndices = queue_indices;
	buffer_info.flags = 0;

	VkBuffer buff;

	if (vkCreateBuffer(vmem->device, &buffer_info, NULL, &buff) != VK_SUCCESS) {
		fprintf(stderr, "Failured creating buffer before allocation.\n");
		return false;
	}

	// Get memory requirements
	VkMemoryRequirements mem_requirements;
	vkGetBufferMemoryRequirements(vmem->device, buff, &mem_requirements);

	uint32_t desired_index =
		vkmemory_findmemorytype(vmem->physical_device, mem_requirements.memoryTypeBits, properties);

	// Calculate aligned size of buffer
	/* VkDeviceSize buff_pgs = buff_size / mem_requirements.alignment;
	buff_pgs++;
	VkDeviceSize aligned_size = buff_pgs * mem_requirements.alignment; */

	// Find or allocate spot of GPU memory
	struct VulkanAllocation *prev = NULL, *curr = vmem->allocation;
	struct VulkanBuffer *new_buff = NULL;

	while (curr != NULL) {
		// If memory types are equal
		if (curr->req == desired_index) {
			// Traverse through buffers
			struct VulkanBuffer *bcurr = curr->buffers;
			struct MemoryOffsets offsets = {0};
			bool fits = false;

			// Check beginning of linked list
			if (bcurr != NULL) {
				fits = vkmemory_calculateoffsets(0, bcurr->start, buff_size,
												 mem_requirements.alignment, &offsets);
				if (fits) {
					new_buff = curr->buffers = vkmemory_createbufferstruct(
						buff, curr, buff_size, offsets.start, offsets.end);
					curr->buffers->next = bcurr;
				}
			}

			// Check after every element
			while (bcurr != NULL) {
				VkDeviceSize next_start =
					(bcurr->next == NULL) ? curr->mem_size : bcurr->next->start;
				fits = vkmemory_calculateoffsets(bcurr->end, next_start, buff_size,
												 mem_requirements.alignment, &offsets);
				if (fits) {
					new_buff = bcurr->next = vkmemory_createbufferstruct(
						buff, curr, buff_size, offsets.start, offsets.end);
					break;
				}

				bcurr = bcurr->next;
			}
		}

		// Find next memory allocation
		prev = curr;
		curr = curr->next;
	}

	// If not allocated
	if (new_buff == NULL) {
		// Create alloc structure
		struct VulkanAllocation *mem_salloc = malloc(sizeof(*mem_salloc));

		mem_salloc->buffers = NULL;
		mem_salloc->mem_size = VK_ALLOC_BLOCK_SIZE;
		mem_salloc->req = vkmemory_findmemorytype(vmem->physical_device,
												  mem_requirements.memoryTypeBits, properties);
		mem_salloc->next = (curr == NULL) ? NULL : curr->next;

		assert(curr == NULL || curr->next != NULL);

		// Allocate memory
		VkMemoryAllocateInfo alloc_info = {0};

		alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		alloc_info.allocationSize = VK_ALLOC_BLOCK_SIZE;
		alloc_info.memoryTypeIndex = mem_salloc->req;

		if (vkAllocateMemory(vmem->device, &alloc_info, NULL, &mem_salloc->mem) != VK_SUCCESS) {
			fprintf(stderr, "Failured to allocate GPU device memory.\n");
			return false;
		}

		curr = mem_salloc;

		// Add to memory linked list
		if (prev == NULL) {
			vmem->allocation = mem_salloc;
		} else {
			prev->next = mem_salloc;
		}

		new_buff = curr->buffers = vkmemory_createbufferstruct(buff, curr, buff_size, 0, buff_size);
	}

	if (new_buff == NULL) {
		fprintf(stderr, "Memory allocation failure in vkmemory_createbuffer.\n");
		return false;
	}

	// Bind buffer to memory
	vkBindBufferMemory(vmem->device, new_buff->buffer, new_buff->allocation->mem, new_buff->start);

	*struct_buff = new_buff;

	// Unlock
	pthread_mutex_unlock(&vmem->allocation_lock);

	printf("GPU buffer created: %p\n", (*struct_buff)->buffer);

	return true;
}

bool vkmemory_destroybuffer(struct VulkanMemory *vmem, struct VulkanBuffer *struct_buff) {
	if (struct_buff == NULL || vmem == NULL) {
		fprintf(stderr, "NULL values passed into destroy buffer function.\n");
		return true;
	}

	// Get lock
	pthread_mutex_lock(&vmem->allocation_lock);

	// Find spot of GPU memory
	struct VulkanAllocation *curr = struct_buff->allocation;
	struct VulkanBuffer *bprev = NULL, *bcurr = curr->buffers;
	while (bcurr != NULL) {
		if (bcurr->buffer == struct_buff->buffer) {
			// Destroy buffer
			vkDestroyBuffer(vmem->device, bcurr->buffer, NULL);

			// Patch linked list
			if (bprev == NULL) {
				curr->buffers = bcurr->next;
			} else {
				bprev->next = bcurr->next;
			}

			// Free GPU memory if unused & patch linked list
			/* Impossible to patch memory allocation linked list without traversing or making it
			more complex, so we just wait to free until vkmemory_destroy */

			/* if (bprev == NULL && bcurr->next == NULL) {
				vkFreeMemory(vmem->device, curr->mem, NULL);
				if (vmem->allocation == curr) {
					vmem->allocation = curr->next;
				} else {
					prev->next = curr->next;
				}
				free(curr);
			} */

			// Free allocated buffer struct
			free(bcurr);
			bcurr = NULL;

			// Unlock before return
			pthread_mutex_unlock(&vmem->allocation_lock);

			return true;
		}

		bprev = bcurr;
		bcurr = bcurr->next;
	}

	// Unlock before returning failure
	pthread_mutex_unlock(&vmem->allocation_lock);

	fprintf(stderr, "Could not find buffer in allocated memory lists.\n");
	return false;
}

bool vkmemory_mapbuffer(struct VulkanMemory *vmem, struct VulkanBuffer *struct_buff, void **map) {
	if (struct_buff == NULL || vmem == NULL) {
		fprintf(stderr, "NULL values passed into destroy buffer function.\n");
		return true;
	}

	vkMapMemory(vmem->device, struct_buff->allocation->mem, struct_buff->start,
				struct_buff->buffer_size, 0, map);

	return true;
}

bool vkmemory_unmapbuffer(struct VulkanMemory *vmem, struct VulkanBuffer *struct_buff) {
	if (struct_buff == NULL || vmem == NULL) {
		fprintf(stderr, "NULL values passed into destroy buffer function.\n");
		return false;
	}

	vkUnmapMemory(vmem->device, struct_buff->allocation->mem);

	return true;
}

// Helper functions
struct VulkanBuffer *vkmemory_createbufferstruct(VkBuffer buff, struct VulkanAllocation *vk_alloc,
												 VkDeviceSize size, VkDeviceSize start,
												 VkDeviceSize end) {
	struct VulkanBuffer *ret = malloc(sizeof(*ret));
	if (ret == NULL) {
		fprintf(stderr, "Failure allocating structure memory for buffer structure.\n");
		return NULL;
	}

	ret->buffer = buff;
	ret->allocation = vk_alloc;
	ret->buffer_size = size;
	ret->start = start;
	ret->end = end;
	ret->next = NULL;
	return ret;
}

uint32_t vkmemory_findmemorytype(VkPhysicalDevice p_device, uint32_t type_filter,
								 VkMemoryPropertyFlags properties) {
	VkPhysicalDeviceMemoryProperties mem_properties;

	vkGetPhysicalDeviceMemoryProperties(p_device, &mem_properties);

	uint32_t i;
	for (i = 0; i < mem_properties.memoryTypeCount; i++) {
		if (type_filter & (1 << i) &&
			(mem_properties.memoryTypes[i].propertyFlags & properties) == properties) {
			return i;
		}
	}

	fprintf(stderr, "Failed to find suitable GPU memory type.\n");
	return 0;
}

bool vkmemory_calculateoffsets(VkDeviceSize start, VkDeviceSize end, VkDeviceSize size,
							   VkDeviceSize alignment, struct MemoryOffsets *offsets) {
	VkDeviceSize start_offset = alignment - start % alignment;
	VkDeviceSize area_start = start + start_offset;

	VkDeviceSize area_end = area_start + size;

	// If block cannot fit in free space
	if (area_start < start || area_end > end) {
		return false;
	}

	offsets->start = area_start;
	offsets->end = area_end;
	return true;
}