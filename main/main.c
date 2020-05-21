#include "application.h"
#include "config.h"
#include "glfw/glfw3.h"

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
	printf("VLK Engine - Version %s\n", VERSION_NUMBER);
	if (enable_validation_layers) {
		printf("Validation layers enabled.\n");
	}
	printf("\n");

	struct Application app = {0};

	application_init(&app);

	while (application_loopcondition(&app)) {
		application_loopevent(&app);
	}

	application_close(&app);

	return EXIT_SUCCESS;
}