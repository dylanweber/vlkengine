#include "glfw/glfw3.h"

#include <stdbool.h>

#ifndef APPLICATION_H
#define APPLICATION_H

#define EXECUTE_PATH_LEN 256

struct Application {
	char execute_path[EXECUTE_PATH_LEN];
	GLFWwindow *window;
	struct VulkanData *vulkan_data;
	struct RenderGroup *render_group;
};

bool application_init(struct Application *);
bool application_loopcondition(struct Application *);
void application_loopevent(struct Application *);
void application_resize(GLFWwindow *, int, int);
void application_refresh(GLFWwindow *);
void application_close(struct Application *);

#endif