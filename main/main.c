#include "application.h"
#include "config.h"
#include "engine_vulkan.h"
#include "glfw/glfw3.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
	printf("VLK Engine - Version %s\n", VERSION_NUMBER);
	if (enable_validation_layers) {
		printf("Validation layers enabled.\n");
	}
	printf("\n");

	struct VulkanData vulkan_data = {0};
	struct Application app = {.window = NULL, .vulkan_data = &vulkan_data};

	bool ret = application_init(&app);
	if (ret == false) {
		fprintf(stderr, "Cannot initialize applcation.\n");
		return EXIT_FAILURE;
	}

	while (application_loopcondition(&app)) {
		application_loopevent(&app);
	}

	application_close(&app);

	return EXIT_SUCCESS;
}