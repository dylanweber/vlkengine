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
	glfwSetWindowRefreshCallback(app->window, application_refresh);
	glfwSetWindowSizeLimits(app->window, 200, 200, GLFW_DONT_CARE, GLFW_DONT_CARE);

	printf("Created window @ 0x%p\n", app->window);

	// Initialize game object list
	rendergroup_init(app->render_group);

	// Create game objects
	struct Vertex tri_vertices[3] = {{.pos = {0.0f, -0.5f}, .color = {1.0f, 0.0f, 0.0f}},
									 {.pos = {0.5f, 0.5f}, .color = {0.0f, 1.0f, 0.0f}},
									 {.pos = {-0.5f, 0.5f}, .color = {0.0f, 0.0f, 1.0f}}};
	struct RenderObjectCreateInfo ro_create_info = {.pltype = PIPELINE_2D,
													.vertices = tri_vertices,
													.vertices_size = sizeof(tri_vertices) /
																	 sizeof(*tri_vertices),
													.is_static = false};
	struct RenderObject *triangle = malloc(sizeof(*triangle));
	ret = object_init(app, &ro_create_info, triangle);
	if (ret == false) {
		fprintf(stderr, "Failure initializing object.\n");
		return false;
	}

	struct Vertex tri2_vertices[3] = {{.pos = {0.2f, -0.2f}, .color = {0.0f, 1.0f, 0.0f}},
									  {.pos = {0.2f, 0.2f}, .color = {1.0f, 1.0f, 1.0f}},
									  {.pos = {-0.2f, 0.2f}, .color = {1.0f, 1.0f, 1.0f}}};
	struct RenderObjectCreateInfo ro2_create_info = {.pltype = PIPELINE_2D,
													 .vertices = tri2_vertices,
													 .vertices_size = sizeof(tri2_vertices) /
																	  sizeof(*tri2_vertices),
													 .is_static = false};
	struct RenderObject *triangle2 = malloc(sizeof(*triangle2));
	ret = object_init(app, &ro2_create_info, triangle2);
	if (ret == false) {
		fprintf(stderr, "Failure initializing object.\n");
		return false;
	}

	rendergroup_add(app->render_group, triangle);
	rendergroup_add(app->render_group, triangle2);

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
}

void application_resize(GLFWwindow *window, int width, int height) {
	struct Application *app = glfwGetWindowUserPointer(window);
	app->vulkan_data->framebuffer_resized = true;
	printf("Resized.\n");
}

void application_refresh(GLFWwindow *window) {
	struct Application *app = glfwGetWindowUserPointer(window);

	bool ret, iconified = glfwGetWindowAttrib(app->window, GLFW_ICONIFIED);
	app->vulkan_data->framebuffer_resized = true;

	// Draw frame
	if (iconified == false) {
		ret = vulkan_drawframe(app);
		if (ret == false) {
			fprintf(stderr, "Problem drawing frame.\n");
		}
	}

	printf("Refreshed.\n");
}

void application_close(struct Application *app) {
	// Destroy objects
	rendergroup_destroy(app->render_group, app);
	// Close Vulkan instance
	vulkan_close(app);
	// End window & GLFW
	glfwDestroyWindow(app->window);
	glfwTerminate();
}