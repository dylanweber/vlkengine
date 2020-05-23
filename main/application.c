#include "application.h"

#include "engine_vulkan.h"

void application_init(struct Application *app) {
	bool ret;

	// Initialize GLFW
	glfwInit();

	// Create window
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	app->window = glfwCreateWindow(800, 600, "Vulkan window", NULL, NULL);

	ret = vulkan_init(app);
	if (!ret) {
		fprintf(stderr, "Failed to initialize Vulkan.");
	}
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
		vulkan_destroydebugutilsmessenger(app->vulkan_data->instance,
										  app->vulkan_data->debug_messenger, NULL);
	// Close Vulkan instance
	vulkan_close(app);
	// End window & GLFW
	glfwDestroyWindow(app->window);
	glfwTerminate();
}