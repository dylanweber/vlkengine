#include "application.h"
#include "config.h"
#include "glfw/glfw3.h"
#include "hashdata.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef ENGINE_VULKAN_H
#define ENGINE_VULKAN_H

#define GFX_INDICES_SIZE 1
#define PRESENT_INDICES_SIZE 1
#define VULKAN_HASHSET_SIZE 32

struct QueueFamilies {
	uint32_t graphics_count;
	uint32_t graphics_indices[GFX_INDICES_SIZE];
	uint32_t present_count;
	uint32_t present_indices[PRESENT_INDICES_SIZE];
};

struct SwapChainSupportDetails {
	VkSurfaceCapabilitiesKHR capabilities;
	uint32_t format_count;
	VkSurfaceFormatKHR *formats;
	uint32_t present_mode_count;
	VkPresentModeKHR *present_modes;
};

struct VulkanData {
	VkInstance instance;
	VkDebugUtilsMessengerEXT debug_messenger;
	VkPhysicalDevice physical_device;
	VkDevice device;
	VkQueue graphics_queue, present_queue;
	VkSurfaceKHR surface;
	struct QueueFamilies qf_indices;
	struct SwapChainSupportDetails sc_details;
};

bool vulkan_init(struct Application *);
bool vulkan_checkextensions();
void vulkan_getextensions(uint32_t *, const char ***);
bool vulkan_createinstance(struct Application *);
struct QueueFamilies vulkan_getqueuefamilies(struct Application *, VkPhysicalDevice);
struct SwapChainSupportDetails vulkan_getswapchainsupport(struct Application *, VkPhysicalDevice);
void vulkan_destroyswapchainsupport(struct SwapChainSupportDetails);
bool vulkan_deviceissuitable(struct QueueFamilies, struct SwapChainSupportDetails,
							 VkPhysicalDevice);
bool vulkan_queuefamilyissuitable(struct QueueFamilies);
bool vulkan_devicesupportsextensions(VkPhysicalDevice);
bool vulkan_compareextensions(VkExtensionProperties *, uint32_t, const char **, uint32_t);
bool vulkan_checkvalidationlayers();
bool vulkan_createsurface(struct Application *);
void vulkan_close(struct Application *);
bool vulkan_setupdebugmessenger(struct Application *);
bool vulkan_pickdevice(struct Application *);
bool vulkan_createlogicaldevice(struct Application *);

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