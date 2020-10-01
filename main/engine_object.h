#include "engine_vertex.h"
#include "engine_vulkan.h"
#include "glfw/glfw3.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef ENGINE_OBJECT_H
#define ENGINE_OBJECT_H

enum PipelineType { NO_PIPELINE = 0, PIPELINE_2D = 1, PIPELINE_3D = 2 };

struct RenderObject {
	// Pipeline
	enum PipelineType pltype;

	// Vertices & vertex buffer
	struct Vertex *vertices;
	size_t vertices_size;
	VkBuffer vertex_buffer;

	// Memory allocation information
	uint16_t retain_count;
	bool is_static : 1;
};

struct RenderObjectCreateInfo {
	enum PipelineType pltype;
	struct Vertex *vertices;
	size_t vertices_size;
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

struct RenderPipelineObject {
	struct RenderObject *render_object;
	struct RenderPipelineObject *next;
};

struct RenderPipelineLink {
	struct RenderObjectObject *pipeline_objects;
	VkPipeline pipeline;
	struct RenderPipelineLink *next_pipeline;
};

struct RenderPipelineChain {
	size_t size;
	struct RenderPipelineLink *link;
};

// Object link functions
bool objectlink_init(struct RenderObjectChain *);
bool objectlink_add(struct RenderObjectChain *, struct RenderObject *);
size_t objectlist_getsize(struct RenderObjectChain *);
struct RenderObjectLink *objectlist_gethead(struct RenderObjectChain *);
bool objectlink_destroy(struct RenderObjectChain *, struct Application *);

// Pipeline link functions
bool pipelinelink_init(struct RenderPipelineChain *);
bool pipelinelink_add(struct RenderObjectChain *, struct RenderObject *);

// Object functions
bool object_init(struct Application *, struct RenderObjectCreateInfo *, struct RenderObject *);
bool object_retain(struct RenderObject *);
bool object_release(struct RenderObject *);
bool object_destroy(struct RenderObject *, struct Application *);
void object_destroybuffers(struct RenderObject *, struct Application *);

#endif