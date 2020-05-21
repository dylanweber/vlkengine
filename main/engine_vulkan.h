#include "application.h"
#include "config.h"
#include "glfw/glfw3.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef ENGINE_VULKAN_H
#define ENGINE_VULKAN_H

bool vulkan_checkextensions();
bool vulkan_createinstance(struct Application *);
void vulkan_close(struct Application *);

#endif