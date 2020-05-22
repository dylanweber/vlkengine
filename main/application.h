#include "glfw/glfw3.h"

#include <stdbool.h>

#ifndef APPLICATION_H
#define APPLICATION_H

struct Application {
	GLFWwindow *window;
	struct VulkanData *vulkan_data;
};

void application_init(struct Application *);
bool application_loopcondition(struct Application *);
void application_loopevent(struct Application *);
void application_close(struct Application *);

#endif