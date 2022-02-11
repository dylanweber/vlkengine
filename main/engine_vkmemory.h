#include "GLFW/glfw3.h"
#include "application.h"
#include "config.h"

#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef ENGINE_VKMEMORY_H
#define ENGINE_VKMEMORY_H

#define VK_ALLOC_BLOCK_SIZE 16777216

struct VulkanBuffer {
	VkBuffer buffer;
	size_t buffer_size;
	VkDeviceSize start;
	VkDeviceSize end;
	uint16_t retain_count;
	struct VulkanAllocation *allocation;
	struct VulkanBuffer *next;
};

struct VulkanAllocation {
	VkDeviceMemory mem;
	VkDeviceSize mem_size;
	uint32_t req;
	struct VulkanBuffer *buffers;
	struct VulkanAllocation *next;
};

struct VulkanMemory {
	VkPhysicalDevice physical_device;
	VkDevice device;
	uint32_t gfx_index, tfr_index;
	pthread_mutex_t allocation_lock;
	struct VulkanAllocation *allocation;
};

struct MemoryOffsets {
	VkDeviceSize start;
	VkDeviceSize end;
};

// Structure functions
bool vkmemory_init(struct VulkanMemory *, VkPhysicalDevice, VkDevice, uint32_t, uint32_t);
bool vkmemory_destroy(struct VulkanMemory *);

// Buffer functions
bool vkmemory_createbuffer(struct VulkanMemory *, VkDeviceSize, VkBufferUsageFlags,
						   VkMemoryPropertyFlags, struct VulkanBuffer **);
bool vkmemory_destroybuffer(struct VulkanMemory *, struct VulkanBuffer *);
bool vkmemory_mapbuffer(struct VulkanMemory *, struct VulkanBuffer *, void **);
bool vkmemory_unmapbuffer(struct VulkanMemory *, struct VulkanBuffer *);

// Helper functions
uint32_t vkmemory_findmemorytype(VkPhysicalDevice, uint32_t, VkMemoryPropertyFlags);
struct VulkanBuffer *vkmemory_createbufferstruct(VkBuffer, struct VulkanAllocation *, VkDeviceSize,
												 VkDeviceSize, VkDeviceSize);
bool vkmemory_calculateoffsets(VkDeviceSize, VkDeviceSize, VkDeviceSize, VkDeviceSize,
							   struct MemoryOffsets *);

#endif	// ENGINE_VKMEMORY_H