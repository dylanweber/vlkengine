#include "application.h"
#include "glfw/glfw3.h"

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
	struct VulkanAllocation *allocation;
};

// Structure functions
bool vkmemory_init(struct VulkanMemory *, VkPhysicalDevice, VkDevice);
bool vkmemory_destroy(struct VulkanMemory *);

// Buffer functions
bool vkmemory_createbuffer(struct VulkanMemory *, VkDeviceSize, VkBufferUsageFlags,
						   VkMemoryPropertyFlags, VkBuffer *);
bool vkmemory_destroybuffer(struct VulkanMemory *, VkBuffer);

// Helper functions
uint32_t vkmemory_findmemorytype(VkPhysicalDevice, uint32_t, VkMemoryPropertyFlags);
struct VulkanBuffer *vkmemory_createbufferstruct(VkBuffer, VkDeviceSize, VkDeviceSize,
												 VkDeviceSize);

#endif	// ENGINE_VKMEMORY_H