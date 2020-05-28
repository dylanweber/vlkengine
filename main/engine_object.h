#include "engine_vulkan.h"
#include "glfw/glfw3.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef ENGINE_OBJECT_H
#define ENGINE_OBJECT_H

struct RenderObject {
	char *vertex_shader_path;
	struct ShaderFile vertex_shader_data;
	VkShaderModule vertex_shader;
	char *fragment_shader_path;
	struct ShaderFile fragment_shader_data;
	VkShaderModule fragment_shader;
	bool is_static;
};

struct RenderObjectCreateInfo {
	char *vertex_shader_path;
	char *fragment_shader_path;
	bool is_static;
};

struct RenderObjectLink {
	struct RenderObject *render_object;
	struct RenderObjectLink *next;
};

bool objectlink_init(struct Application *);
bool objectlink_add(struct Application *, struct RenderObject *);
bool objectlink_createshadermodules(struct Application *);
bool objectlink_destroy(struct Application *);
bool object_init(struct Application *, struct RenderObjectCreateInfo *, struct RenderObject *);
bool object_destroy(struct Application *, struct RenderObject *);
bool object_populateshaders(struct Application *, struct RenderObject *);
bool object_processshaders(struct Application *, struct RenderObject *);
void object_destroyshaders(struct Application *, struct RenderObject *);

#endif