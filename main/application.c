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
	glfwSetWindowUserPointer(app->window, app);
	glfwSetFramebufferSizeCallback(app->window, application_resize);
	glfwSetWindowSizeLimits(app->window, 200, 200, GLFW_DONT_CARE, GLFW_DONT_CARE);

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
	if (ret == false) {
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
	bool ret, iconified = glfwGetWindowAttrib(app->window, GLFW_ICONIFIED);

	// Draw frame
	if (iconified == false) {
		ret = vulkan_drawframe(app);
		if (ret == false) {
			fprintf(stderr, "Problem drawing frame.\n");
		}
	}

	// Poll for events like keyboard & mouse
	glfwPollEvents();

	// Wait for queue to idle
	vkQueueWaitIdle(app->vulkan_data->present_queue);
}

void application_resize(GLFWwindow *window, int width, int height) {
	struct Application *app = glfwGetWindowUserPointer(window);
	app->vulkan_data->framebuffer_resized = true;
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