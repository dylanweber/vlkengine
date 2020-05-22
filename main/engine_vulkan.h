#include "application.h"
#include "config.h"
#include "glfw/glfw3.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef ENGINE_VULKAN_H
#define ENGINE_VULKAN_H

#define GFX_INDICES_SIZE 8

struct QueueFamilies {
	uint32_t graphicsIndices[GFX_INDICES_SIZE];
};

struct VulkanData {
	VkInstance instance;
	VkDebugUtilsMessengerEXT debug_messenger;
	VkPhysicalDevice physical_device;
	struct QueueFamilies indices;
};

bool vulkan_checkextensions();
void vulkan_getextensions(uint32_t *, const char ***);
bool vulkan_createinstance(struct Application *);
bool vulkan_findqueuefamilies(struct Application *);
bool vulkan_checkvalidationlayers();
void vulkan_close(struct Application *);
bool vulkan_setupdebugmessenger(struct Application *);
bool vulkan_pickdevice(struct Application *);

// Callbacks & wrappers
VKAPI_ATTR VkBool32 VKAPI_CALL vulkan_debugcallback(VkDebugUtilsMessageSeverityFlagBitsEXT,
													VkDebugUtilsMessageTypeFlagsEXT,
													const VkDebugUtilsMessengerCallbackDataEXT *,
													void *);
VkResult vulkan_createdebugutilsmessenger(VkInstance, const VkDebugUtilsMessengerCreateInfoEXT *,
										  const VkAllocationCallbacks *,
										  VkDebugUtilsMessengerEXT *);
VkResult vulkan_destroydebugutilsmessenger(VkInstance, VkDebugUtilsMessengerEXT,
										   const VkAllocationCallbacks *);

#endif