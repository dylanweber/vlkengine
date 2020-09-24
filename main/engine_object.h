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
	uint16_t retain_count;
	bool is_static : 1;
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

struct RenderObjectChain {
	size_t size;
	struct RenderObjectLink *link;
};

bool objectlink_init(struct RenderObjectChain *);
bool objectlink_add(struct RenderObjectChain *, struct RenderObject *);
bool objectlink_createshadermodules(struct RenderObjectChain *, struct Application *);
size_t objectlist_getsize(struct RenderObjectChain *);
bool objectlink_destroy(struct RenderObjectChain *, struct Application *);
bool object_init(struct Application *, struct RenderObjectCreateInfo *, struct RenderObject *);
bool object_retain(struct RenderObject *);
bool object_release(struct RenderObject *);
bool object_destroy(struct RenderObject *, struct Application *);
bool object_populateshaders(struct RenderObject *, struct Application *);
bool object_processshaders(struct RenderObject *, struct Application *);
void object_destroyshaders(struct RenderObject *, struct Application *);

#endif