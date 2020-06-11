#include "application.h"

#include "engine_object.h"
#include "engine_vulkan.h"

#include <windows.h>

bool application_init(struct Application *app) {
	bool ret;

	// Get executable path in Windows
	memset(app->execute_path, '\0', EXECUTE_PATH_LEN);
	size_t path_len = GetModuleFileName(NULL, app->execute_path, EXECUTE_PATH_LEN);
	size_t i = path_len;
	while (app->execute_path[i] != '\\') {
		i--;
	}
	app->execute_path[i + 1] = '\0';

	// Initialize GLFW
	glfwInit();

	// Create window
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	app->window = glfwCreateWindow(800, 600, "Vulkan window", NULL, NULL);
	if (app->window == NULL) {
		fprintf(stderr, "Failed to create window.\n");
		return false;
	}

	printf("Created window @ 0x%p\n", app->window);

	// Initialize game object list
	objectlink_init(app);

	// Create game objects
	struct RenderObjectCreateInfo ro_create_info = {.vertex_shader_path = "shaders/shader.vert.spv",
													.fragment_shader_path =
														"shaders/shader.frag.spv",
													.is_static = false};
	struct RenderObject *triangle = malloc(sizeof(*triangle));
	object_init(app, &ro_create_info, triangle);

	objectlink_add(app, triangle);

	// Init Vulkan
	ret = vulkan_init(app);
	if (!ret) {
		fprintf(stderr, "Failed to initialize Vulkan.\n");
		return false;
	}

	return true;
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
	// Destroy objects
	objectlink_destroy(app);
	// Close Vulkan instance
	vulkan_close(app);
	// End window & GLFW
	glfwDestroyWindow(app->window);
	glfwTerminate();
}