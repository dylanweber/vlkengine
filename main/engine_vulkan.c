#include "engine_vulkan.h"

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
	vulkan_createinstance(app);

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
	ret = vulkan_createlogicaldevice(app);
	if (ret == false) {
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
		if (vulkan_queuefamilyissuitable(indices)) {
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

inline bool vulkan_deviceissuitable(struct QueueFamilies indices,
									struct SwapChainSupportDetails details,
									VkPhysicalDevice physical_device) {
	return vulkan_devicesupportsextensions(physical_device) &&
		   vulkan_queuefamilyissuitable(indices) && details.format_count > 0 &&
		   details.present_mode_count > 0;
}

bool vulkan_queuefamilyissuitable(struct QueueFamilies indices) {
	if (indices.graphics_count < 1 || indices.present_count < 1) {
		return false;
	}

	return true;
}

bool vulkan_devicesupportsextensions(VkPhysicalDevice physical_device) {
	uint32_t extension_count;
	vkEnumerateDeviceExtensionProperties(physical_device, NULL, &extension_count, NULL);

	VkExtensionProperties *extensions = malloc(sizeof(*extensions) * extension_count);
	vkEnumerateDeviceExtensionProperties(physical_device, NULL, &extension_count, extensions);

	bool result = vulkan_compareextensions(extensions, extension_count, device_extensions,
										   sizeof(device_extensions) / sizeof(*device_extensions));

	free(extensions);
	return result;
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
	vulkan_destroyswapchainsupport(app->vulkan_data->sc_details);

	// Destroy debug messenger
	if (enable_validation_layers)
		vulkan_destroydebugutilsmessenger(app->vulkan_data->instance,
										  app->vulkan_data->debug_messenger, NULL);

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

		if (!device_features.geometryShader) {
			score = 0;
			continue;
		}

		indices = vulkan_getqueuefamilies(app, physical_devices[i]);
		details = vulkan_getswapchainsupport(app, physical_devices[i]);

		// Pick best option
		if (score > max_score && vulkan_deviceissuitable(indices, details, physical_devices[i])) {
			max_score = score;
			app->vulkan_data->physical_device = physical_devices[i];
			app->vulkan_data->qf_indices = indices;
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

VKAPI_ATTR VkBool32 VKAPI_CALL
vulkan_debugcallback(VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
					 VkDebugUtilsMessageTypeFlagsEXT message_type,
					 const VkDebugUtilsMessengerCallbackDataEXT *p_callbackdata, void *p_userdata) {
	if (message_severity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
		fprintf(stderr, "Validation layer: %s\n", p_callbackdata->pMessage);
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