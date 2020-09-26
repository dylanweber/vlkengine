#include "engine_vertex.h"

VkVertexInputBindingDescription vertex_getbindingdescription() {
	VkVertexInputBindingDescription binding_description = {0};

	binding_description.binding = 0;
	binding_description.stride = sizeof(struct Vertex);
	binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	return binding_description;
}

void vertex_getattributedescriptions(uint32_t *attr_count,
									 VkVertexInputAttributeDescription *attr_descriptions) {
	*attr_count = 2;
	if (attr_descriptions == NULL)
		return;

	attr_descriptions[0].binding = 0;
	attr_descriptions[0].location = 0;
	attr_descriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
	attr_descriptions[0].offset = offsetof(struct Vertex, pos);

	attr_descriptions[1].binding = 0;
	attr_descriptions[1].location = 1;
	attr_descriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
	attr_descriptions[1].offset = offsetof(struct Vertex, color);
}