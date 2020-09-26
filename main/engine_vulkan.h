#include "application.h"
#include "config.h"
#include "engine_vertex.h"
#include "glfw/glfw3.h"
#include "hashdata.h"

#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef ENGINE_VULKAN_H
#define ENGINE_VULKAN_H

#define GFX_INDICES_SIZE 1
#define PRESENT_INDICES_SIZE 1
#define MAX_FRAMES_IN_FLIGHT 2
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

struct ShaderFile {
	char *data;
	size_t size;
};

struct VulkanData {
	// Instance
	VkInstance instance;

	// Debug messenger for validation layers
	VkDebugUtilsMessengerEXT debug_messenger;

	// Physical and logical device
	VkPhysicalDevice physical_device;
	VkDevice device;

	// Graphics and present queue
	VkQueue graphics_queue, present_queue;

	// Surface, swapchain, and associated variables
	VkSurfaceKHR surface;
	VkSwapchainKHR swapchain;
	uint32_t swapchain_images_size;
	VkImage *swapchain_images;
	VkFormat swapchain_imageformat;
	VkExtent2D swapchain_extent;
	uint32_t swapchain_imageviews_size;
	VkImageView *swapchain_imageviews;

	// Graphics pipeline
	VkRenderPass render_pass;
	VkPipelineLayout pipeline_layout;
	VkPipeline graphics_pipeline;

	// Framebuffers & command buffers
	uint32_t swapchain_framebuffers_size;
	VkFramebuffer *swapchain_framebuffers;
	VkCommandPool command_pool;
	uint32_t command_buffers_size;
	VkCommandBuffer *command_buffers;

	// Memory allocation info
	uint32_t vertex_buffer_size;
	uint32_t allocated_memory_size;
	VkDeviceMemory vertex_buffer_memory;

	// Semaphores for presentation
	VkSemaphore image_available_sem[MAX_FRAMES_IN_FLIGHT];
	VkSemaphore render_finished_sem[MAX_FRAMES_IN_FLIGHT];
	VkFence in_flight_fen[MAX_FRAMES_IN_FLIGHT];
	VkFence *imgs_in_flight;
	uint32_t current_frame;
	bool framebuffer_resized;

	// Structures required for creation
	// struct QueueFamilies qf_indices;
	// struct SwapChainSupportDetails sc_details;
};

bool vulkan_init(struct Application *);
bool vulkan_checkextensions();
void vulkan_getextensions(uint32_t *, const char ***);
bool vulkan_createinstance(struct Application *);
struct QueueFamilies vulkan_getqueuefamilies(struct Application *, VkPhysicalDevice);
struct SwapChainSupportDetails vulkan_getswapchainsupport(struct Application *, VkPhysicalDevice);
void vulkan_destroyswapchainsupport(struct SwapChainSupportDetails);
bool vulkan_deviceissuitable(struct QueueFamilies, struct SwapChainSupportDetails);
bool vulkan_devicesupportsextensions(VkPhysicalDevice);
bool vulkan_compareextensions(VkExtensionProperties *, uint32_t, const char **, uint32_t);
bool vulkan_checkvalidationlayers();
bool vulkan_createsurface(struct Application *);
void vulkan_close(struct Application *);
bool vulkan_cleanupswapchain(struct Application *);
bool vulkan_setupdebugmessenger(struct Application *);
bool vulkan_pickdevice(struct Application *);
bool vulkan_createlogicaldevice(struct Application *);
VkSurfaceFormatKHR vulkan_choosescsurfaceformat(struct SwapChainSupportDetails);
VkPresentModeKHR vulkan_choosescpresentmode(struct SwapChainSupportDetails);
VkExtent2D vulkan_choosescextent(struct Application *, struct SwapChainSupportDetails);
bool vulkan_recreateswapchain(struct Application *);
bool vulkan_createswapchain(struct Application *);
bool vulkan_createimageviews(struct Application *);
bool vulkan_createrenderpass(struct Application *);
bool vulkan_createpipeline(struct Application *);
bool vulkan_createframebuffers(struct Application *);
bool vulkan_createcommandpool(struct Application *);
bool vulkan_createcommandbuffers(struct Application *);
bool vulkan_createsynchronization(struct Application *);
bool vulkan_createvertexbuffers(struct Application *);
uint32_t vulkan_findmemorytype(struct Application *, uint32_t, VkMemoryPropertyFlags);
bool vulkan_drawframe(struct Application *);

// Shader functions
struct ShaderFile vulkan_readshaderfile(const char *);
void vulkan_destroyshaderfile(struct ShaderFile);
VkShaderModule vulkan_createshadermodule(struct Application *, struct ShaderFile);

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