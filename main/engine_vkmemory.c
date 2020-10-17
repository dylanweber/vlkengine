#include "engine_vkmemory.h"

bool vkmemory_init(struct VulkanMemory *vmem, VkPhysicalDevice physical_device, VkDevice device) {
	vmem->physical_device = physical_device;
	vmem->device = device;
	vmem->allocation = NULL;
	return true;
}

bool vkmemory_destroy(struct VulkanMemory *vmem) {
	// Free all buffers and memory
	struct VulkanAllocation *prev = NULL, *curr = vmem->allocation;
	struct VulkanBuffer *bprev, *bcurr;

	while (curr != NULL) {
		bprev = NULL;
		bcurr = curr->buffers;
		while (bcurr != NULL) {
			vkDestroyBuffer(vmem->device, bcurr->buffer, NULL);

			bprev = bcurr;
			bcurr = bcurr->next;

			free(bcurr);
		}

		vkFreeMemory(vmem->device, curr->mem, NULL);

		prev = curr;
		curr = curr->next;

		free(curr);
	}
	return true;
}

bool vkmemory_createbuffer(struct VulkanMemory *vmem, VkDeviceSize buff_size,
						   VkBufferUsageFlags usage, VkMemoryPropertyFlags properties,
						   VkBuffer *buff) {
	// Create buffer before allocation
	VkBufferCreateInfo buffer_info = {0};

	buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	buffer_info.size = buff_size;
	buffer_info.usage = usage;
	buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	buffer_info.flags = 0;

	if (vkCreateBuffer(vmem->device, &buffer_info, NULL, buff) != VK_SUCCESS) {
		fprintf(stderr, "Failured creating buffer before allocation.\n");
		return false;
	}

	// Get memory requirements
	VkMemoryRequirements mem_requirements;
	vkGetBufferMemoryRequirements(vmem->device, *buff, &mem_requirements);

	uint32_t desired_index =
		vkmemory_findmemorytype(vmem->physical_device, mem_requirements.memoryTypeBits, properties);

	// Calculate aligned size of buffer
	VkDeviceSize buff_pgs = buff_size / mem_requirements.alignment;
	buff_pgs++;
	VkDeviceSize aligned_size = buff_pgs * mem_requirements.alignment;

	// Find or allocate spot of GPU memory
	struct VulkanAllocation *prev = NULL, *curr = vmem->allocation;
	struct VulkanBuffer *vk_buff = NULL;

	while (curr != NULL) {
		// If memory types are equal
		if (curr->req == desired_index) {
			// Traverse through buffers
			struct VulkanBuffer *bprev = NULL, *bcurr = curr->buffers;

			while (bcurr != NULL) {
				// If we're at the start
				if (bprev == NULL && bcurr != NULL) {
					if (bcurr->start > aligned_size) {
						// Insert new buffer
						vk_buff = curr->buffers =
							vkmemory_createbufferstruct(*buff, aligned_size, 0, aligned_size);
						// Set next buffer to previous first buffer
						curr->buffers->next = bcurr;
						break;
					}
				}

				// If we're in the middle
				if (bprev != NULL && bcurr != NULL) {
					if (bprev->end - bcurr->start > aligned_size) {
						// Insert new buffer
						vk_buff = bprev->next = vkmemory_createbufferstruct(
							*buff, aligned_size, bprev->end, bprev->end + aligned_size);
						// Set next buffer to previous next buffer
						bprev->next->next = bcurr;
						break;
					}
				}

				// If we're at the end
				if (bprev != NULL && bcurr == NULL) {
					// Insert new buffer
					vk_buff = bprev->next = vkmemory_createbufferstruct(
						*buff, aligned_size, bprev->end, bprev->end + aligned_size);
					bprev->next->next = NULL;
				}

				bprev = bcurr;
				bcurr = bcurr->next;
			}
		}

		// Find next memory allocation
		prev = curr;
		curr = curr->next;
	}

	// No memory exists, must allocate
	if (curr == NULL) {
		// Create alloc structure
		struct VulkanAllocation *mem_salloc = malloc(sizeof(*mem_salloc));

		mem_salloc->buffers = NULL;
		mem_salloc->mem_size = VK_ALLOC_BLOCK_SIZE;
		mem_salloc->req = vkmemory_findmemorytype(vmem->physical_device,
												  mem_requirements.memoryTypeBits, properties);
		mem_salloc->next = NULL;

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
	}

	// Bind buffer to memory
	vkBindBufferMemory(vmem->device, vk_buff->buffer, curr->mem, vk_buff->start);

	return true;
}

bool vkmemory_destroybuffer(struct VulkanMemory *vmem, VkBuffer buff) {
	if (buff == NULL || vmem == NULL) {
		fprintf(stderr, "NULL values passed into destroy buffer function.\n");
		return true;
	}

	// Find spot of GPU memory
	struct VulkanAllocation *prev = NULL, *curr = vmem->allocation;
	struct VulkanBuffer *bprev, *bcurr;

	while (curr != NULL) {
		bprev = NULL;
		bcurr = curr->buffers;
		while (bcurr != NULL) {
			if (bcurr->buffer == buff) {
				// Destroy buffer
				vkDestroyBuffer(vmem->device, bcurr->buffer, NULL);

				// Patch linked list
				if (bprev == NULL) {
					curr->buffers = bcurr->next;
				} else {
					bprev->next = bcurr->next;
				}

				// Free GPU memory if unused & patch linked list
				if (bprev == NULL && bcurr->next == NULL) {
					vkFreeMemory(vmem->device, curr->mem, NULL);
					if (prev == NULL) {
						vmem->allocation = curr->next;
					} else {
						prev->next = curr->next;
					}
					free(curr);
				}

				// Free allocated buffer struct
				free(bcurr);

				return true;
			}

			bprev = bcurr;
			bcurr = bcurr->next;
		}

		prev = curr;
		curr = curr->next;
	}

	fprintf(stderr, "Could not find buffer in allocated memory lists.\n");
	return false;
}

// Helper functions
struct VulkanBuffer *vkmemory_createbufferstruct(VkBuffer buff, VkDeviceSize size,
												 VkDeviceSize start, VkDeviceSize end) {
	struct VulkanBuffer *ret = malloc(sizeof(*ret));
	if (ret == NULL) {
		fprintf(stderr, "Failure allocating structure memory for buffer structure.\n");
		return NULL;
	}

	ret->buffer = buff;
	ret->buffer_size = size;
	ret->start = start;
	ret->end = end;
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