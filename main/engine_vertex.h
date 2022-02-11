#include "GLFW/glfw3.h"

#include <stdbool.h>

#ifndef ENGINE_VERTEX_H
#define ENGINE_VERTEX_H

struct Vertex {
	float pos[2];
	float color[3];
};

VkVertexInputBindingDescription vertex_getbindingdescription();
void vertex_getattributedescriptions(uint32_t *, VkVertexInputAttributeDescription *);

#endif