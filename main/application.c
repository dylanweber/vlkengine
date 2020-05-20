#include "application.h"

void application_init(struct Application *app) {
	// Initialize GLFW
	glfwInit();

	// Create window
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	app->window = glfwCreateWindow(800, 600, "Vulkan window", NULL, NULL);

	uint32_t extensionCount = 0;
	vkEnumerateInstanceExtensionProperties(NULL, &extensionCount, NULL);
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
	// End window & GLFW
	glfwDestroyWindow(app->window);
	glfwTerminate();
}