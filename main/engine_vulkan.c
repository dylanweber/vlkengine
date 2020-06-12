#include "engine_vulkan.h"

#include "engine_object.h"

const char *device_extensions[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
const char *validation_extensions[] = {VK_EXT_DEBUG_UTILS_EXTENSION_NAME};
const char *validation_layers[] = {"VK_LAYER_KHRONOS_validation"};

bool vulkan_init(struct Application *app) {
	bool ret = vulkan_checkextensions();
	if (ret == false) {
		fprintf(stderr, "Failure to find all required extensions.\n");
		return false;
	} else {
		printf("All required extensions found.\n");
	}

	// Create Vulkan instance
	ret = vulkan_createinstance(app);
	if (ret == false) {
		fprintf(stderr, "Could not create Vulkan instance.\n");
		return false;
	}

	// Create validation layers (if enabled)
	if (enable_validation_layers)
		vulkan_setupdebugmessenger(app);

	// Create surface
	ret = vulkan_createsurface(app);
	if (ret == false) {
		return false;
	}

	// Pick graphics device
	ret = vulkan_pickdevice(app);
	if (ret == false) {
		return false;
	}
	// Create logical device
	ret = vulkan_createlogicaldevice(app);
	if (ret == false) {
		return false;
	}
	// Create swapchain
	ret = vulkan_createswapchain(app);
	if (ret == false) {
		return false;
	}
	// Create swapchain image views
	ret = vulkan_createimageviews(app);
	if (ret == false) {
		return false;
	}

	// Create shader modules from all current objects
	ret = objectlink_createshadermodules(app);
	if (ret == false) {
		fprintf(stderr, "Failure to create all shader modules.\n");
		return false;
	}

	// Create render pass
	ret = vulkan_createrenderpass(app);
	if (ret == false) {
		fprintf(stderr, "Failure to create render pass.\n");
		return false;
	}
	// Create pipeline using shaders
	ret = vulkan_createpipeline(app);
	if (ret == false) {
		fprintf(stderr, "Failure to create pipeline.\n");
		return false;
	}
	// Create framebuffers
	ret = vulkan_createframebuffers(app);
	if (ret == false) {
		fprintf(stderr, "Failure to create framebuffers.\n");
		return false;
	}
	// Create command pool
	ret = vulkan_createcommandpool(app);
	if (ret == false) {
		fprintf(stderr, "Failure to create command pool.\n");
		return false;
	}
	// Create command buffesr
	ret = vulkan_createcommandbuffers(app);
	if (ret == false) {
		fprintf(stderr, "Failure to create command buffers.\n");
		return false;
	}
	// Create semaphores
	ret = vulkan_createsemaphores(app);
	if (ret == false) {
		fprintf(stderr, "Failure making sempahores.\n");
		return false;
	}

	return true;
}

bool vulkan_checkextensions() {
	VkResult ret;

	// Check extension count
	uint32_t extension_count = 0;
	ret = vkEnumerateInstanceExtensionProperties(NULL, &extension_count, NULL);
	if (ret != VK_SUCCESS) {
		fprintf(stderr, "Failure enumerating extensions.");
		return false;
	}

	// Allocation extension array
	VkExtensionProperties *extensions = malloc(sizeof(*extensions) * extension_count);
	if (extensions == NULL) {
		fprintf(stderr, "Failure allocating memory.");
		return false;
	}

	// Retrieve extensions
	vkEnumerateInstanceExtensionProperties(NULL, &extension_count, extensions);

	// Get GLFW required extensions
	uint32_t glfw_extension_count;
	const char **glfw_extensions;

	glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);

	// Print and check extensions
	printf("Supported extensions:\n");
	uint32_t i, j, match_count = 0;
	for (i = 0; i < extension_count; i++) {
		printf("\t%s", extensions[i].extensionName);
		for (j = 0; j < glfw_extension_count; j++) {
			if (strcmp(extensions[i].extensionName, glfw_extensions[j]) == 0) {
				printf(" REQUIRED");
				match_count++;
			}
		}
		printf("\n");
	}

	free(extensions);
	return match_count == glfw_extension_count;
}

void vulkan_getextensions(uint32_t *extension_size, const char ***extension_list) {
	// Get GLFW required extensions
	uint32_t glfw_extension_count;
	const char **glfw_extensions;

	glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);

	// Set size variable
	*extension_size = glfw_extension_count;

	// Add validation extensions
	if (enable_validation_layers == true) {
		*extension_size += sizeof(validation_extensions) / sizeof(*validation_extensions);
	}

	// Return if not setting extension list
	if (extension_list == NULL)
		return;

	// Copy const pointers for all extensions
	uint32_t i;
	for (i = 0; i < glfw_extension_count; i++) {
		(*extension_list)[i] = glfw_extensions[i];
	}
	for (i = 0; i < sizeof(validation_extensions) / sizeof(*validation_extensions); i++) {
		(*extension_list)[glfw_extension_count + i] = validation_extensions[i];
	}

	// Print requested extensions
	printf("Requested extensions:\n");
	for (i = 0; i < *extension_size; i++) {
		printf("\t%s\n", (*extension_list)[i]);
	}
}

bool vulkan_createinstance(struct Application *app) {
	// Check for validation layer support
	if (enable_validation_layers && !vulkan_checkvalidationlayers()) {
		fprintf(stderr, "Validation layers are not supported.");
		return false;
	}

	// Create required structures
	VkApplicationInfo app_info = {0};
	app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	app_info.pApplicationName = "VLK Engine";
	app_info.applicationVersion = VK_MAKE_VERSION(1, 1, 0);
	app_info.pEngineName = "VLK Engine";
	app_info.engineVersion = BUILD_NUMBER;
	app_info.apiVersion = VK_MAKE_VERSION(1, 1, 0);

	VkInstanceCreateInfo create_info = {0};
	create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	create_info.pApplicationInfo = &app_info;

	// Get extensions
	uint32_t extension_count;
	const char **extensions;

	vulkan_getextensions(&extension_count, NULL);

	extensions = malloc(sizeof(*extensions) * extension_count);
	if (extensions == NULL) {
		fprintf(stderr, "Failure to allocate memory.");
		return false;
	}

	vulkan_getextensions(&extension_count, &extensions);

	// Attach extensions to structure
	create_info.enabledExtensionCount = extension_count;
	create_info.ppEnabledExtensionNames = extensions;

	VkDebugUtilsMessengerCreateInfoEXT debug_create_info = {0};
	if (enable_validation_layers) {
		int validation_layers_size = sizeof(validation_layers) / sizeof(*validation_layers);
		create_info.enabledLayerCount = validation_layers_size;
		create_info.ppEnabledLayerNames = validation_layers;

		debug_create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		debug_create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
											VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
											VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		debug_create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
										VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
										VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		debug_create_info.pfnUserCallback = vulkan_debugcallback;
		debug_create_info.pUserData = app;
		debug_create_info.pNext = &debug_create_info;
	} else {
		create_info.enabledLayerCount = 0;
	}

	// Create instance
	VkResult ret = vkCreateInstance(&create_info, NULL, &app->vulkan_data->instance);
	if (ret != VK_SUCCESS) {
		fprintf(stderr, "Failed to create Vulkan instance.");
		return false;
	}

	free(extensions);
	return true;
}

struct QueueFamilies vulkan_getqueuefamilies(struct Application *app,
											 VkPhysicalDevice physical_device) {
	struct QueueFamilies indices = {0};

	// Check queue count
	uint32_t queue_family_count;
	vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, NULL);

	// Allocate family array
	VkQueueFamilyProperties *queue_families = malloc(sizeof(*queue_families) * queue_family_count);
	if (queue_families == NULL) {
		fprintf(stderr, "Failure to allocate memory.");
		return indices;
	}

	// Get all queue family properties
	vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, queue_families);

	// Select applicable graphics queue family
	VkBool32 present_support = VK_FALSE;
	uint32_t i;
	for (i = 0; i < queue_family_count; i++) {
		// Check for graphics queue
		if (queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT &&
			indices.graphics_count != GFX_INDICES_SIZE) {
			// Add graphics index to struct
			indices.graphics_indices[indices.graphics_count] = i;
			indices.graphics_count += 1;
		}

		// Check for present queue
		vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, i, app->vulkan_data->surface,
											 &present_support);
		if (present_support == VK_TRUE && indices.present_count != PRESENT_INDICES_SIZE) {
			// Add present index to struct
			indices.present_indices[indices.present_count] = i;
			indices.present_count += 1;
		}

		// If struct is suitable, exit
		if (indices.graphics_count > 0 && indices.present_count > 0) {
			break;
		}
	}

	free(queue_families);
	return indices;
}

struct SwapChainSupportDetails vulkan_getswapchainsupport(struct Application *app,
														  VkPhysicalDevice physical_device) {
	struct SwapChainSupportDetails details = {0};

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, app->vulkan_data->surface,
											  &details.capabilities);

	vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, app->vulkan_data->surface,
										 &details.format_count, NULL);
	if (details.format_count != 0) {
		details.formats = malloc(sizeof(*details.formats) * details.format_count);
		if (details.formats == NULL) {
			fprintf(stderr, "Failure to allocate memory.\n");
			return details;
		}

		vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, app->vulkan_data->surface,
											 &details.format_count, details.formats);
	}

	vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, app->vulkan_data->surface,
											  &details.present_mode_count, NULL);

	if (details.present_mode_count != 0) {
		details.present_modes = malloc(sizeof(*details.present_modes) * details.present_mode_count);
		if (details.present_modes == NULL) {
			fprintf(stderr, "Failure to allocate memory.\n");
			return details;
		}

		vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, app->vulkan_data->surface,
												  &details.present_mode_count,
												  details.present_modes);
	}

	return details;
}

void vulkan_destroyswapchainsupport(struct SwapChainSupportDetails details) {
	free(details.formats);
	free(details.present_modes);
}

inline bool vulkan_compareextensions(VkExtensionProperties *extensions, uint32_t extensions_size,
									 const char **extension_set, uint32_t set_size) {
	struct HashSet *set = hashset_create(VULKAN_HASHSET_SIZE);

	bool flag = true;
	int i;
	for (i = 0; i < extensions_size; i++) {
		hashset_store(set, extensions[i].extensionName);
	}

	for (i = 0; i < set_size; i++) {
		if (hashset_exists(set, extension_set[i]) == false) {
			flag = false;
			break;
		}
	}

	hashset_destroy(set);
	return flag;
}

bool vulkan_checkvalidationlayers() {
	VkResult ret;

	// Get instance layer properties count
	uint32_t layer_count;
	ret = vkEnumerateInstanceLayerProperties(&layer_count, NULL);
	if (ret != VK_SUCCESS) {
		fprintf(stderr, "Failure enumerating layer properties.");
	}

	// Get all instance layer properties
	VkLayerProperties *available_layers = malloc(sizeof(*available_layers) * layer_count);
	if (available_layers == NULL) {
		fprintf(stderr, "Failure allocating memory.");
		return false;
	}

	vkEnumerateInstanceLayerProperties(&layer_count, available_layers);

	// Make sure required validation layers are found in instance layer properties
	struct HashSet *set = hashset_create(VULKAN_HASHSET_SIZE);

	bool flag = true;
	int i;
	for (i = 0; i < layer_count; i++) {
		hashset_store(set, available_layers[i].layerName);
	}
	for (i = 0; i < sizeof(validation_layers) / sizeof(*validation_layers); i++) {
		if (hashset_exists(set, validation_layers[i]) == false) {
			flag = false;
			break;
		}
	}

	hashset_print(set);

	free(available_layers);
	hashset_destroy(set);
	return flag;
}

void vulkan_close(struct Application *app) {
	uint32_t i;	 // Loop index variable for destruction

	// Destroy sempahores
	vkDestroySemaphore(app->vulkan_data->device, app->vulkan_data->render_finished_sem, NULL);
	vkDestroySemaphore(app->vulkan_data->device, app->vulkan_data->image_available_sem, NULL);

	// Destroy command pool
	vkDestroyCommandPool(app->vulkan_data->device, app->vulkan_data->command_pool, NULL);

	// Destroy all framebuffers
	for (i = 0; i < app->vulkan_data->swapchain_framebuffers_size; i++) {
		vkDestroyFramebuffer(app->vulkan_data->device, app->vulkan_data->swapchain_framebuffers[i],
							 NULL);
	}

	// Destroy graphics pipeline
	vkDestroyPipeline(app->vulkan_data->device, app->vulkan_data->graphics_pipeline, NULL);
	// Destroy graphics pipeline layout
	vkDestroyPipelineLayout(app->vulkan_data->device, app->vulkan_data->pipeline_layout, NULL);
	// Destroy render pass
	vkDestroyRenderPass(app->vulkan_data->device, app->vulkan_data->render_pass, NULL);

	// Destroy all swapchain image views
	for (i = 0; i < app->vulkan_data->swapchain_imageviews_size; i++) {
		vkDestroyImageView(app->vulkan_data->device, app->vulkan_data->swapchain_imageviews[i],
						   NULL);
	}
	free(app->vulkan_data->swapchain_imageviews);

	// Free array of swapchain images
	free(app->vulkan_data->swapchain_images);

	// Destroy swapchain
	vkDestroySwapchainKHR(app->vulkan_data->device, app->vulkan_data->swapchain, NULL);
	vulkan_destroyswapchainsupport(app->vulkan_data->sc_details);

	// Destroy debug messenger
	if (enable_validation_layers)
		vulkan_destroydebugutilsmessenger(app->vulkan_data->instance,
										  app->vulkan_data->debug_messenger, NULL);

	// Destroy instance
	vkDestroyDevice(app->vulkan_data->device, NULL);
	vkDestroySurfaceKHR(app->vulkan_data->instance, app->vulkan_data->surface, NULL);
	vkDestroyInstance(app->vulkan_data->instance, NULL);
}

bool vulkan_setupdebugmessenger(struct Application *app) {
	if (!enable_validation_layers)
		return VK_SUCCESS;

	VkDebugUtilsMessengerCreateInfoEXT create_info = {0};
	create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
								  VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
								  VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
							  VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
							  VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	create_info.pfnUserCallback = vulkan_debugcallback;
	create_info.pUserData = app;

	VkResult ret = vulkan_createdebugutilsmessenger(app->vulkan_data->instance, &create_info, NULL,
													&app->vulkan_data->debug_messenger);

	return ret == VK_SUCCESS;
}

bool vulkan_createsurface(struct Application *app) {
	VkResult ret = glfwCreateWindowSurface(app->vulkan_data->instance, app->window, NULL,
										   &app->vulkan_data->surface);
	if (ret != VK_SUCCESS) {
		fprintf(stderr, "Unable to make Vulkan surface.");
		return false;
	}

	return true;
}

bool vulkan_pickdevice(struct Application *app) {
	VkResult ret;

	// Get count of devices
	uint32_t device_count = 0;
	ret = vkEnumeratePhysicalDevices(app->vulkan_data->instance, &device_count, NULL);
	if (ret != VK_SUCCESS) {
		fprintf(stderr, "Failure enumerating physical devices.");
		return false;
	}

	// Show error if none are found
	if (device_count == 0) {
		fprintf(stderr, "Failed to find devices that support Vulkan.");
		return false;
	}

	// Allocate array to store devices
	VkPhysicalDevice *physical_devices = malloc(sizeof(*physical_devices) * device_count);
	if (physical_devices == NULL) {
		fprintf(stderr, "Failure to allocate memory.");
		return false;
	}

	// Get devices from Vulkan
	ret = vkEnumeratePhysicalDevices(app->vulkan_data->instance, &device_count, physical_devices);
	if (ret != VK_SUCCESS) {
		fprintf(stderr, "Failure to enumerate physical devices for Vulkan.");
		return false;
	}

	// Determine most suitable device
	printf("Found GPUs:\n");
	int i;
	uint32_t score, max_score = 0;
	VkPhysicalDeviceProperties device_properties;
	VkPhysicalDeviceFeatures device_features;
	struct QueueFamilies indices;
	struct SwapChainSupportDetails details;
	for (i = 0; i < device_count; i++) {
		vkGetPhysicalDeviceProperties(physical_devices[i], &device_properties);
		vkGetPhysicalDeviceFeatures(physical_devices[i], &device_features);

		// Calculate score based on features
		score = 0;
		memset(&indices, 0, sizeof(indices));

		if (device_properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
			score += 1000;
		}

		score += device_properties.limits.maxImageDimension2D;

		if (!device_features.geometryShader ||
			!vulkan_devicesupportsextensions(physical_devices[i])) {
			score = 0;
			continue;
		}

		indices = vulkan_getqueuefamilies(app, physical_devices[i]);
		details = vulkan_getswapchainsupport(app, physical_devices[i]);

		// Pick best option
		if (score > max_score && vulkan_deviceissuitable(indices, details)) {
			max_score = score;
			app->vulkan_data->physical_device = physical_devices[i];
			app->vulkan_data->qf_indices = indices;
			app->vulkan_data->sc_details = details;
		} else {
			vulkan_destroyswapchainsupport(details);
		}

		// Print device name
		printf("\t%s\n\t\tScore: %d\n\t\tSuitable: %s\n", device_properties.deviceName, score,
			   (app->vulkan_data->physical_device == physical_devices[i]) ? "true" : "false");
	}

	free(physical_devices);

	return app->vulkan_data->physical_device != NULL;
}

inline bool vulkan_deviceissuitable(struct QueueFamilies indices,
									struct SwapChainSupportDetails details) {
	return indices.graphics_count > 0 && indices.present_count > 0 && details.format_count > 0 &&
		   details.present_mode_count > 0;
}

inline bool vulkan_devicesupportsextensions(VkPhysicalDevice physical_device) {
	uint32_t extension_count;
	vkEnumerateDeviceExtensionProperties(physical_device, NULL, &extension_count, NULL);

	VkExtensionProperties *extensions = malloc(sizeof(*extensions) * extension_count);
	vkEnumerateDeviceExtensionProperties(physical_device, NULL, &extension_count, extensions);

	bool result = vulkan_compareextensions(extensions, extension_count, device_extensions,
										   sizeof(device_extensions) / sizeof(*device_extensions));

	free(extensions);
	return result;
}

bool vulkan_createlogicaldevice(struct Application *app) {
	// Create set of VkDeviceQueueCreateInfos for every index in our QueueFamilies
	uint32_t queue_create_infos_size = 0;
	VkDeviceQueueCreateInfo queue_create_infos[GFX_INDICES_SIZE + PRESENT_INDICES_SIZE] = {0};

	float queue_priority = 1.0f;

	// Go through every graphics index then present index and add to queue_create_infos if not found
	int i, j;
	for (i = 0; i < app->vulkan_data->qf_indices.graphics_count; i++) {
		for (j = 0; j <= queue_create_infos_size; i++) {
			// If at end of queue_create_infos array, add new element
			if (j == queue_create_infos_size) {
				queue_create_infos[j].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
				queue_create_infos[j].queueFamilyIndex =
					app->vulkan_data->qf_indices.graphics_indices[i];
				queue_create_infos[j].queueCount = 1;
				queue_create_infos[j].pQueuePriorities = &queue_priority;
				queue_create_infos_size += 1;
				break;
			}
			// If match is found, go to next index
			if (queue_create_infos[j].queueFamilyIndex ==
				app->vulkan_data->qf_indices.graphics_indices[i])
				break;
		}
	}
	for (i = 0; i < app->vulkan_data->qf_indices.present_count; i++) {
		for (j = 0; j <= queue_create_infos_size; i++) {
			// If at end of queue_create_infos array, add new element
			if (j == queue_create_infos_size) {
				queue_create_infos[j].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
				queue_create_infos[j].queueFamilyIndex =
					app->vulkan_data->qf_indices.present_indices[i];
				queue_create_infos[j].queueCount = 1;
				queue_create_infos[j].pQueuePriorities = &queue_priority;
				queue_create_infos_size += 1;
				break;
			}
			// If match is found, go to next index
			if (queue_create_infos[j].queueFamilyIndex ==
				app->vulkan_data->qf_indices.present_indices[i])
				break;
		}
	}

	// Create info passed to device creation function
	VkPhysicalDeviceFeatures device_features = {0};
	VkDeviceCreateInfo device_info = {0};

	device_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	device_info.pQueueCreateInfos = queue_create_infos;
	device_info.queueCreateInfoCount = queue_create_infos_size;
	device_info.pEnabledFeatures = &device_features;

	device_info.enabledExtensionCount = sizeof(device_extensions) / sizeof(*device_extensions);
	device_info.ppEnabledExtensionNames = device_extensions;

	if (enable_validation_layers) {
		int validation_layers_size = sizeof(validation_layers) / sizeof(*validation_layers);
		device_info.enabledLayerCount = validation_layers_size;
		device_info.ppEnabledLayerNames = validation_layers;
	} else {
		device_info.enabledLayerCount = 0;
	}

	// Create logical device
	VkResult res = vkCreateDevice(app->vulkan_data->physical_device, &device_info, NULL,
								  &app->vulkan_data->device);
	if (res != VK_SUCCESS) {
		fprintf(stderr, "Failure creating logical Vulkan device.");
		return false;
	}

	// Get graphics queue
	vkGetDeviceQueue(app->vulkan_data->device, app->vulkan_data->qf_indices.graphics_indices[0], 0,
					 &app->vulkan_data->graphics_queue);
	// Get present queue
	vkGetDeviceQueue(app->vulkan_data->device, app->vulkan_data->qf_indices.present_indices[0], 0,
					 &app->vulkan_data->present_queue);

	return true;
}

VkSurfaceFormatKHR vulkan_choosescsurfaceformat(struct SwapChainSupportDetails sc_details) {
	uint32_t i;
	for (i = 0; i < sc_details.format_count; i++) {
		if (sc_details.formats[i].format == VK_FORMAT_B8G8R8_SRGB &&
			sc_details.formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
			return sc_details.formats[i];
		}
	}

	return sc_details.formats[0];
}

VkPresentModeKHR vulkan_choosescpresentmode(struct SwapChainSupportDetails sc_details) {
	uint32_t i;
	for (i = 0; i < sc_details.present_mode_count; i++) {
		if (sc_details.present_modes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
			return sc_details.present_modes[i];
		}
	}

	return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D vulkan_choosescextent(struct Application *app,
								 struct SwapChainSupportDetails sc_details) {
	if (sc_details.capabilities.currentExtent.width != UINT32_MAX) {
		return sc_details.capabilities.currentExtent;
	} else {
		// Get current framebuffer size
		int width, height;
		glfwGetFramebufferSize(app->window, &width, &height);

		VkExtent2D actual_extent = {.width = width, .height = height};

		if (sc_details.capabilities.maxImageExtent.width < actual_extent.width) {
			actual_extent.width = sc_details.capabilities.maxImageExtent.width;
		}
		if (sc_details.capabilities.minImageExtent.width > actual_extent.width) {
			actual_extent.width = sc_details.capabilities.minImageExtent.width;
		}

		if (sc_details.capabilities.maxImageExtent.height < actual_extent.height) {
			actual_extent.height = sc_details.capabilities.maxImageExtent.height;
		}
		if (sc_details.capabilities.minImageExtent.height > actual_extent.height) {
			actual_extent.height = sc_details.capabilities.minImageExtent.height;
		}

		return actual_extent;
	}
}

bool vulkan_createswapchain(struct Application *app) {
	struct SwapChainSupportDetails sc_details = app->vulkan_data->sc_details;

	// Get swapchain settings
	VkSurfaceFormatKHR surface_format = vulkan_choosescsurfaceformat(sc_details);
	VkPresentModeKHR present_mode = vulkan_choosescpresentmode(sc_details);
	VkExtent2D extent = vulkan_choosescextent(app, sc_details);

	// Determine swapchain image count
	uint32_t image_count = sc_details.capabilities.minImageCount + 1;
	if (sc_details.capabilities.maxImageCount > 0 &&
		image_count > sc_details.capabilities.maxImageCount) {
		image_count = sc_details.capabilities.maxImageCount;
	}

	// Create swapchain
	VkSwapchainCreateInfoKHR sc_create_info = {0};
	sc_create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	sc_create_info.surface = app->vulkan_data->surface;
	sc_create_info.minImageCount = image_count;
	sc_create_info.imageFormat = surface_format.format;
	sc_create_info.imageColorSpace = surface_format.colorSpace;
	sc_create_info.imageExtent = extent;
	sc_create_info.imageArrayLayers = 1;
	sc_create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	if (enable_validation_layers && app->vulkan_data->qf_indices.graphics_count > 1) {
		printf("Only using one graphics queue.\n");
	}

	// Assuming only 1 graphics index & present index
	if (app->vulkan_data->qf_indices.graphics_indices[0] !=
		app->vulkan_data->qf_indices.present_indices[0]) {
		sc_create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		sc_create_info.queueFamilyIndexCount = 2;
		uint32_t queue_indices[] = {app->vulkan_data->qf_indices.graphics_indices[0],
									app->vulkan_data->qf_indices.present_indices[0]};
		sc_create_info.pQueueFamilyIndices = queue_indices;
	} else {
		sc_create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	}

	sc_create_info.preTransform = sc_details.capabilities.currentTransform;
	sc_create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	sc_create_info.presentMode = present_mode;
	sc_create_info.clipped = VK_TRUE;
	sc_create_info.oldSwapchain = VK_NULL_HANDLE;

	VkResult ret = vkCreateSwapchainKHR(app->vulkan_data->device, &sc_create_info, NULL,
										&app->vulkan_data->swapchain);
	if (ret != VK_SUCCESS) {
		fprintf(stderr, "Failed to create swapchain.\n");
		return false;
	}

	vkGetSwapchainImagesKHR(app->vulkan_data->device, app->vulkan_data->swapchain,
							&app->vulkan_data->swapchain_images_size, NULL);

	app->vulkan_data->swapchain_images = malloc(sizeof(*app->vulkan_data->swapchain_images) *
												app->vulkan_data->swapchain_images_size);
	if (app->vulkan_data->swapchain_images == NULL) {
		fprintf(stderr, "Failure allocating memory for swapchain images.");
		return false;
	}

	vkGetSwapchainImagesKHR(app->vulkan_data->device, app->vulkan_data->swapchain,
							&app->vulkan_data->swapchain_images_size,
							app->vulkan_data->swapchain_images);

	app->vulkan_data->swapchain_imageformat = surface_format.format;
	app->vulkan_data->swapchain_extent = extent;

	return true;
}

bool vulkan_createrenderpass(struct Application *app) {
	VkAttachmentDescription color_attachment = {0};
	color_attachment.format = app->vulkan_data->swapchain_imageformat;
	color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
	color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference color_attachment_ref = {0};
	color_attachment_ref.attachment = 0;
	color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass = {0};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &color_attachment_ref;

	VkSubpassDependency dependency = {0};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	VkRenderPassCreateInfo render_pass_info = {0};
	render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	render_pass_info.attachmentCount = 1;
	render_pass_info.pAttachments = &color_attachment;
	render_pass_info.subpassCount = 1;
	render_pass_info.pSubpasses = &subpass;
	render_pass_info.dependencyCount = 1;
	render_pass_info.pDependencies = &dependency;

	VkResult ret = vkCreateRenderPass(app->vulkan_data->device, &render_pass_info, NULL,
									  &app->vulkan_data->render_pass);
	if (ret != VK_SUCCESS) {
		fprintf(stderr, "Failure to create render pass.\n");
		return false;
	}

	return true;
}

bool vulkan_createimageviews(struct Application *app) {
	app->vulkan_data->swapchain_imageviews_size = app->vulkan_data->swapchain_images_size;
	app->vulkan_data->swapchain_imageviews =
		malloc(sizeof(*app->vulkan_data->swapchain_imageviews) *
			   app->vulkan_data->swapchain_imageviews_size);
	if (app->vulkan_data->swapchain_imageviews == NULL) {
		fprintf(stderr, "Failed to allocate memory.\n");
		return false;
	}

	uint32_t i;
	for (i = 0; i < app->vulkan_data->swapchain_imageviews_size; i++) {
		VkImageViewCreateInfo iv_create_info = {0};
		iv_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		iv_create_info.image = app->vulkan_data->swapchain_images[i];
		iv_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
		iv_create_info.format = app->vulkan_data->swapchain_imageformat;
		iv_create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		iv_create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		iv_create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		iv_create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		iv_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		iv_create_info.subresourceRange.baseMipLevel = 0;
		iv_create_info.subresourceRange.levelCount = 1;
		iv_create_info.subresourceRange.baseArrayLayer = 0;
		iv_create_info.subresourceRange.layerCount = 1;

		VkResult ret = vkCreateImageView(app->vulkan_data->device, &iv_create_info, NULL,
										 &app->vulkan_data->swapchain_imageviews[i]);
		if (ret != VK_SUCCESS) {
			fprintf(stderr, "Failed to create image view.\n");
			return false;
		}
	}

	return true;
}

bool vulkan_createpipeline(struct Application *app) {
	// Get size of game object list
	size_t objects_size = objectlist_getsize(app);

	printf("Object linked list is %llu long.\n", objects_size);

	// Allocate shader stages
	VkPipelineShaderStageCreateInfo *shader_stages =
		calloc(objects_size * 2, sizeof(*shader_stages));
	if (shader_stages == NULL) {
		fprintf(stderr, "Failed to allocate memory.\n");
		return false;
	}

	// Populate all shader stages
	struct RenderObjectLink *curr = app->objects;
	size_t i;
	for (i = 0; i < 2 * objects_size; i += 2) {
		// Add vertex shader
		shader_stages[i].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shader_stages[i].stage = VK_SHADER_STAGE_VERTEX_BIT;
		shader_stages[i].module = curr->render_object->vertex_shader;
		shader_stages[i].pName = "main";

		// Add fragment shader
		shader_stages[i + 1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shader_stages[i + 1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		shader_stages[i + 1].module = curr->render_object->fragment_shader;
		shader_stages[i + 1].pName = "main";

		// Move to next game object
		assert(i < 2 * objects_size || curr->next != NULL);
		curr = curr->next;
	}

	VkPipelineVertexInputStateCreateInfo vertex_input_info = {0};
	vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertex_input_info.vertexBindingDescriptionCount = 0;
	vertex_input_info.pVertexBindingDescriptions = NULL;
	vertex_input_info.vertexAttributeDescriptionCount = 0;
	vertex_input_info.pVertexAttributeDescriptions = NULL;

	VkPipelineInputAssemblyStateCreateInfo input_assembly = {0};
	input_assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	input_assembly.primitiveRestartEnable = VK_FALSE;

	VkViewport viewport = {0};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = (float)app->vulkan_data->swapchain_extent.width;
	viewport.height = (float)app->vulkan_data->swapchain_extent.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor = {0};
	scissor.offset = (VkOffset2D){0, 0};
	scissor.extent = app->vulkan_data->swapchain_extent;

	VkPipelineViewportStateCreateInfo viewport_state = {0};
	viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewport_state.viewportCount = 1;
	viewport_state.pViewports = &viewport;
	viewport_state.scissorCount = 1;
	viewport_state.pScissors = &scissor;

	VkPipelineRasterizationStateCreateInfo rasterizer = {0};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer.lineWidth = 1.0f;
	rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
	rasterizer.depthBiasEnable = VK_FALSE;
	rasterizer.depthBiasConstantFactor = 0.0f;
	rasterizer.depthBiasClamp = 0.0f;
	rasterizer.depthBiasSlopeFactor = 0.0f;

	VkPipelineMultisampleStateCreateInfo multisampling = {0};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisampling.minSampleShading = 1.0f;
	multisampling.pSampleMask = NULL;
	multisampling.alphaToCoverageEnable = VK_FALSE;
	multisampling.alphaToOneEnable = VK_FALSE;

	VkPipelineColorBlendAttachmentState colorblend_attachment = {0};
	colorblend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
										   VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorblend_attachment.blendEnable = VK_FALSE;
	colorblend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
	colorblend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorblend_attachment.colorBlendOp = VK_BLEND_OP_ADD;
	colorblend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	colorblend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorblend_attachment.alphaBlendOp = VK_BLEND_OP_ADD;

	VkPipelineColorBlendStateCreateInfo colorblending = {0};
	colorblending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorblending.logicOpEnable = VK_FALSE;
	colorblending.logicOp = VK_LOGIC_OP_COPY;
	colorblending.attachmentCount = 1;
	colorblending.pAttachments = &colorblend_attachment;
	colorblending.blendConstants[0] = 0.0f;
	colorblending.blendConstants[1] = 0.0f;
	colorblending.blendConstants[2] = 0.0f;
	colorblending.blendConstants[3] = 0.0f;

	VkPipelineLayoutCreateInfo pipeline_layout_info = {0};
	pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipeline_layout_info.setLayoutCount = 0;
	pipeline_layout_info.pushConstantRangeCount = 0;

	VkResult ret = vkCreatePipelineLayout(app->vulkan_data->device, &pipeline_layout_info, NULL,
										  &app->vulkan_data->pipeline_layout);
	if (ret != VK_SUCCESS) {
		fprintf(stderr, "Failure to create graphics pipeline layout.\n");
		return false;
	}

	// Graphics pipeline creation information
	VkGraphicsPipelineCreateInfo pipeline_info = {0};
	pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipeline_info.stageCount = 2 * objects_size;
	pipeline_info.pStages = shader_stages;
	pipeline_info.pVertexInputState = &vertex_input_info;
	pipeline_info.pInputAssemblyState = &input_assembly;
	pipeline_info.pViewportState = &viewport_state;
	pipeline_info.pRasterizationState = &rasterizer;
	pipeline_info.pMultisampleState = &multisampling;
	pipeline_info.pDepthStencilState = NULL;
	pipeline_info.pColorBlendState = &colorblending;
	pipeline_info.pDynamicState = NULL;
	// Specify graphics pipeline layout
	pipeline_info.layout = app->vulkan_data->pipeline_layout;
	pipeline_info.renderPass = app->vulkan_data->render_pass;
	pipeline_info.subpass = 0;
	pipeline_info.basePipelineHandle = NULL;
	pipeline_info.basePipelineIndex = -1;

	ret = vkCreateGraphicsPipelines(app->vulkan_data->device, NULL, 1, &pipeline_info, NULL,
									&app->vulkan_data->graphics_pipeline);
	if (ret != VK_SUCCESS) {
		fprintf(stderr, "Failure to create graphics pipeline.\n");
		return false;
	}

	// NOTE: Possible to delete shaders here

	// Free shader stages
	free(shader_stages);
	return true;
}

bool vulkan_createframebuffers(struct Application *app) {
	app->vulkan_data->swapchain_framebuffers_size = app->vulkan_data->swapchain_imageviews_size;
	app->vulkan_data->swapchain_framebuffers =
		malloc(sizeof(*app->vulkan_data->swapchain_framebuffers) *
			   app->vulkan_data->swapchain_framebuffers_size);
	if (app->vulkan_data->swapchain_framebuffers == NULL) {
		fprintf(stderr, "Failure to allocate framebuffers.\n");
		return false;
	}

	VkResult ret;
	uint32_t i;
	for (i = 0; i < app->vulkan_data->swapchain_imageviews_size; i++) {
		VkFramebufferCreateInfo framebuffer_info = {0};
		framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebuffer_info.renderPass = app->vulkan_data->render_pass;
		framebuffer_info.attachmentCount = 1;
		framebuffer_info.pAttachments = &app->vulkan_data->swapchain_imageviews[i];
		framebuffer_info.width = app->vulkan_data->swapchain_extent.width;
		framebuffer_info.height = app->vulkan_data->swapchain_extent.height;
		framebuffer_info.layers = 1;

		ret = vkCreateFramebuffer(app->vulkan_data->device, &framebuffer_info, NULL,
								  &app->vulkan_data->swapchain_framebuffers[i]);
		if (ret != VK_SUCCESS) {
			fprintf(stderr, "Failure to create framebuffer %u.", i);
			return false;
		}
	}

	return true;
}

bool vulkan_createcommandpool(struct Application *app) {
	VkCommandPoolCreateInfo pool_info = {0};
	pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	pool_info.queueFamilyIndex =
		app->vulkan_data->qf_indices.graphics_indices[0];  // Assume first graphics index
	pool_info.flags = 0;

	VkResult ret = vkCreateCommandPool(app->vulkan_data->device, &pool_info, NULL,
									   &app->vulkan_data->command_pool);
	if (ret != VK_SUCCESS) {
		fprintf(stderr, "Failed to create command pool.\n");
		return false;
	}

	return true;
}

bool vulkan_createcommandbuffers(struct Application *app) {
	app->vulkan_data->command_buffers_size = app->vulkan_data->swapchain_framebuffers_size;
	app->vulkan_data->command_buffers =
		malloc(sizeof(*app->vulkan_data->command_buffers) * app->vulkan_data->command_buffers_size);
	if (app->vulkan_data->command_buffers == NULL) {
		fprintf(stderr, "Failure to allocate command buffers.\n");
		return false;
	}

	VkCommandBufferAllocateInfo alloc_info = {0};
	alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	alloc_info.commandPool = app->vulkan_data->command_pool;
	alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	alloc_info.commandBufferCount = app->vulkan_data->command_buffers_size;

	VkResult ret = vkAllocateCommandBuffers(app->vulkan_data->device, &alloc_info,
											app->vulkan_data->command_buffers);
	if (ret != VK_SUCCESS) {
		fprintf(stderr, "Failure to allocate command buffers from Vulkan.\n");
		return false;
	}

	uint32_t i;
	for (i = 0; i < app->vulkan_data->command_buffers_size; i++) {
		VkCommandBufferBeginInfo begin_info = {0};
		begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		ret = vkBeginCommandBuffer(app->vulkan_data->command_buffers[i], &begin_info);
		if (ret != VK_SUCCESS) {
			fprintf(stderr, "Failure to begin recording to command buffer.\n");
			return false;
		}

		VkRenderPassBeginInfo renderpass_info = {0};
		renderpass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderpass_info.renderPass = app->vulkan_data->render_pass;
		renderpass_info.framebuffer = app->vulkan_data->swapchain_framebuffers[i];
		renderpass_info.renderArea.offset = (VkOffset2D){0, 0};
		renderpass_info.renderArea.extent = app->vulkan_data->swapchain_extent;

		VkClearValue clear_color = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
		renderpass_info.clearValueCount = 1;
		renderpass_info.pClearValues = &clear_color;

		vkCmdBeginRenderPass(app->vulkan_data->command_buffers[i], &renderpass_info,
							 VK_SUBPASS_CONTENTS_INLINE);
		vkCmdBindPipeline(app->vulkan_data->command_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS,
						  app->vulkan_data->graphics_pipeline);
		vkCmdDraw(app->vulkan_data->command_buffers[i], 3, 1, 0, 0);
		vkCmdEndRenderPass(app->vulkan_data->command_buffers[i]);
		ret = vkEndCommandBuffer(app->vulkan_data->command_buffers[i]);
		if (ret != VK_SUCCESS) {
			fprintf(stderr, "Failed to record command buffer.\n");
			return false;
		}
	}

	return true;
}

bool vulkan_createsemaphores(struct Application *app) {
	VkSemaphoreCreateInfo semaphore_info = {0};
	semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkResult ret = vkCreateSemaphore(app->vulkan_data->device, &semaphore_info, NULL,
									 &app->vulkan_data->image_available_sem);
	if (ret != VK_SUCCESS) {
		fprintf(stderr, "Failure making image available semaphore.\n");
		return false;
	}

	ret = vkCreateSemaphore(app->vulkan_data->device, &semaphore_info, NULL,
							&app->vulkan_data->render_finished_sem);
	if (ret != VK_SUCCESS) {
		fprintf(stderr, "Failure making render finished semaphore.\n");
		return false;
	}

	return true;
}

bool vulkan_drawframe(struct Application *app) {
	uint32_t image_index;
	vkAcquireNextImageKHR(app->vulkan_data->device, app->vulkan_data->swapchain, UINT64_MAX,
						  app->vulkan_data->image_available_sem, NULL, &image_index);

	VkPipelineStageFlags wait_stages[1] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
	VkSubmitInfo submit_info = {0};
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.waitSemaphoreCount = 1;
	submit_info.pWaitSemaphores = &app->vulkan_data->image_available_sem;
	submit_info.pWaitDstStageMask = wait_stages;
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = &app->vulkan_data->command_buffers[image_index];
	submit_info.signalSemaphoreCount = 1;
	submit_info.pSignalSemaphores = &app->vulkan_data->render_finished_sem;

	VkResult ret = vkQueueSubmit(app->vulkan_data->graphics_queue, 1, &submit_info, NULL);
	if (ret != VK_SUCCESS) {
		fprintf(stderr, "Failed to submit to graphics queue.\n");
		return false;
	}

	VkPresentInfoKHR present_info = {0};
	present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	present_info.waitSemaphoreCount = 1;
	present_info.pWaitSemaphores = &app->vulkan_data->render_finished_sem;
	present_info.swapchainCount = 1;
	present_info.pSwapchains = &app->vulkan_data->swapchain;
	present_info.pImageIndices = &image_index;
	present_info.pResults = NULL;

	vkQueuePresentKHR(app->vulkan_data->present_queue, &present_info);

	return true;
}

// Shader functions
struct ShaderFile vulkan_readshaderfile(const char *filename) {
	struct ShaderFile return_shader = {0};

	// Open file
	FILE *fp = fopen(filename, "rb");
	if (fp == NULL) {
		fprintf(stderr, "Failed to open shader file.\n");
		return return_shader;
	}

	// Get file size
	fseek(fp, 0, SEEK_END);
	return_shader.size = ftell(fp);
	printf("Shader \"%s\" is %llu bytes long.\n", filename, return_shader.size);
	assert(return_shader.size % 4 == 0);

	return_shader.data = malloc(sizeof(*return_shader.data) * return_shader.size);
	if (return_shader.data == NULL) {
		fprintf(stderr, "Failed to allocate memory for shader.\n");
		return_shader.size = 0;
		return return_shader;
	}

	// Read file
	fseek(fp, 0, SEEK_SET);
	size_t read = fread(return_shader.data, return_shader.size, sizeof(*return_shader.data), fp);
	if (read != 1) {
		fprintf(stderr, "Failed to read entire shader.\n");
		free(return_shader.data);
		return_shader.size = 0;
	}

	fclose(fp);
	return return_shader;
}

void vulkan_destroyshaderfile(struct ShaderFile shaderfile) {
	free(shaderfile.data);
}

VkShaderModule vulkan_createshadermodule(struct Application *app, struct ShaderFile shaderfile) {
	VkShaderModuleCreateInfo sm_create_info = {0};

	sm_create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	sm_create_info.codeSize = shaderfile.size;
	sm_create_info.pCode = (uint32_t *)shaderfile.data;

	VkShaderModule shader_module;
	VkResult ret =
		vkCreateShaderModule(app->vulkan_data->device, &sm_create_info, NULL, &shader_module);
	if (ret != VK_SUCCESS) {
		fprintf(stderr, "Failed to create shader module.\n");
		return NULL;
	}

	printf("Created shader module @ 0x%p\n", shader_module);

	return shader_module;
}

// Callbacks & wrappers
VKAPI_ATTR VkBool32 VKAPI_CALL
vulkan_debugcallback(VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
					 VkDebugUtilsMessageTypeFlagsEXT message_type,
					 const VkDebugUtilsMessengerCallbackDataEXT *p_callbackdata, void *p_userdata) {
	if (message_severity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
		fprintf(stderr, "\033[31;1mValidation layer:\033[0m %s\n", p_callbackdata->pMessage);
	}

	return VK_FALSE;
}

VkResult vulkan_createdebugutilsmessenger(VkInstance instance,
										  const VkDebugUtilsMessengerCreateInfoEXT *p_createinfo,
										  const VkAllocationCallbacks *p_allocator,
										  VkDebugUtilsMessengerEXT *p_debugmessenger) {
	PFN_vkCreateDebugUtilsMessengerEXT func =
		(PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance,
																  "vkCreateDebugUtilsMessengerEXT");
	if (func != NULL) {
		return func(instance, p_createinfo, p_allocator, p_debugmessenger);
	}
	return VK_ERROR_EXTENSION_NOT_PRESENT;
}

VkResult vulkan_destroydebugutilsmessenger(VkInstance instance,
										   VkDebugUtilsMessengerEXT debugmessenger,
										   const VkAllocationCallbacks *p_allocator) {
	PFN_vkDestroyDebugUtilsMessengerEXT func =
		(PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
			instance, "vkDestroyDebugUtilsMessengerEXT");
	if (func != NULL) {
		func(instance, debugmessenger, p_allocator);
		return VK_SUCCESS;
	}
	return VK_ERROR_EXTENSION_NOT_PRESENT;
}