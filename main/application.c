#include "application.h"

#include "engine_vulkan.h"

void application_init(struct Application *app) {
	bool ret;

	// Initialize GLFW
	glfwInit();

	// Create window
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	app->window = glfwCreateWindow(800, 600, "Vulkan window", NULL, NULL);

	ret = vulkan_checkextensions();
	if (ret == false) {
		perror("Failure to find all required extensions.\n");
	} else {
		printf("All required extensions found.\n");
	}

	// Create Vulkan instance
	vulkan_createinstance(app);

	// Create validation layers (if enabled)
	if (enable_validation_layers)
		vulkan_setupdebugmessenger(app);

	// Pick graphics device
	vulkan_pickdevice(app);
}

bool application_loopcondition(struct Application *app) {
	// Close only if GLFW says so
	return !glfwWindowShouldClose(app->window);
}

void application_loopevent(struct Application *app) {
	// Poll for events like keyboard & mouse
	glfwPollEvents();
}

void application_close(struct Application *app) {
	// Destroy debug messenger
	if (enable_validation_layers)
		vulkan_destroydebugutilsmessenger(app->instance, app->debug_messenger, NULL);
	// Close Vulkan instance
	vulkan_close(app);
	// End window & GLFW
	glfwDestroyWindow(app->window);
	glfwTerminate();
}