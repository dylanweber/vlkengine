#include "config.h"
#include "glfw/glfw3.h"

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
	printf("VLK Engine - Version %s", VERSION_NUMBER);

	// Initialize GLFW
	glfwInit();

	// Create window
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	GLFWwindow *window = glfwCreateWindow(800, 600, "Vulkan window", NULL, NULL);

	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();
	}

	// End window & GLFW
	glfwDestroyWindow(window);
	glfwTerminate();

	return EXIT_SUCCESS;
}