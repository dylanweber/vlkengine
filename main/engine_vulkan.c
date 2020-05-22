#include "engine_vulkan.h"

const char *validation_extensions[] = {VK_EXT_DEBUG_UTILS_EXTENSION_NAME};
const char *validation_layers[] = {"VK_LAYER_KHRONOS_validation"};

bool vulkan_checkextensions() {
	VkResult ret;

	// Check extension count
	uint32_t extension_count = 0;
	ret = vkEnumerateInstanceExtensionProperties(NULL, &extension_count, NULL);
	if (ret != VK_SUCCESS) {
		perror("Failure enumerating extensions.");
		return false;
	}

	// Allocation extension array
	VkExtensionProperties *extensions = malloc(sizeof(*extensions) * extension_count);
	if (extensions == NULL) {
		perror("Failure allocating memory.");
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
	int i, j, match_count = 0;
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
	int i;
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
		perror("Validation layers are not supported.");
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
		perror("Failure to allocate memory.");
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
	VkResult ret = vkCreateInstance(&create_info, NULL, &app->instance);
	if (ret != VK_SUCCESS) {
		perror("Failed to create Vulkan instance.");
		return false;
	}

	free(extensions);
	return true;
}

bool vulkan_checkvalidationlayers() {
	VkResult ret;

	uint32_t layer_count;
	ret = vkEnumerateInstanceLayerProperties(&layer_count, NULL);
	if (ret != VK_SUCCESS) {
		perror("Failure enumerating layer properties.");
	}

	VkLayerProperties *available_layers = malloc(sizeof(*available_layers) * layer_count);
	if (available_layers == NULL) {
		perror("Failure allocating memory.");
		return false;
	}

	vkEnumerateInstanceLayerProperties(&layer_count, available_layers);

	int i, j, match_count = 0,
			  validation_layers_size = sizeof(validation_layers) / sizeof(*validation_layers);
	for (i = 0; i < validation_layers_size; i++) {
		for (j = 0; j < layer_count; j++) {
			if (strcmp(available_layers[j].layerName, validation_layers[i]) == 0) {
				match_count++;
			}
		}
	}

	free(available_layers);
	return match_count == validation_layers_size;
}

void vulkan_close(struct Application *app) {
	vkDestroyInstance(app->instance, NULL);
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

	VkResult ret =
		vulkan_createdebugutilsmessenger(app->instance, &create_info, NULL, &app->debug_messenger);

	return ret == VK_SUCCESS;
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