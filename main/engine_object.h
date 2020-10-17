#include "application.h"
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

union UniformData {
	struct u2d {
		float translate[4];
		float rotation[4];
		float scale[4];
	};
	struct u3d {
		float model[16];
		float view[16];
		float projection[16];
	};
};

/*
	How RenderData would eventually be drawn when properly bound via command buffer:

	> vkCmdBindDescriptorSets(buff, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 0, 1, &descriptor_set,
		1, &uniform_position);
	> vkCmdDrawIndexed(buff, indices_size, 1, indices_position, vertex_position, 0);

*/

struct RenderData {
	// Pipeline
	enum PipelineType pltype;

	// Vertex & index buffer
	VkBuffer vi_buffer;

	// Vertices
	struct Vertex *vertices;
	size_t vertices_size;
	VkDeviceSize vertex_alignment;
	VkDeviceSize vertex_position;

	// Indices
	uint64_t *indices;
	size_t indices_size;
	VkDeviceSize indices_alignment;
	VkDeviceSize indices_position;

	// Uniform buffer as descriptor set
	VkBuffer uni_buffer;
	VkDescriptorSet desc_set;

	// Uniforms
	union UniformData *uniforms;
	size_t uniform_size;
	VkDeviceSize uniform_alignment;
	VkDeviceSize uniform_position;
};

struct RenderObject {
	struct RenderData render_data;

	// x, y, z position
	float pos[3];

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

struct RenderPipelineLink {
	struct RenderObject *render_object;
	struct RenderPipelineLink *next;
};

struct RenderPipelineChain {
	size_t size;
	size_t *pl_sizes;
	struct RenderPipelineLink **links;
};

struct RenderGroup {
	struct RenderPipelineChain *pipelines;
	struct RenderObjectChain *objects;
};

// Object link functions
bool objectlink_init(struct RenderObjectChain *);
bool objectlink_add(struct RenderObjectChain *, struct RenderObject *);
size_t objectlist_getsize(struct RenderObjectChain *);
struct RenderObjectLink *objectlist_gethead(struct RenderObjectChain *);
bool objectlink_destroy(struct RenderObjectChain *, struct Application *);

// Pipeline link functions
bool pipelinelink_init(struct RenderPipelineChain *);
bool pipelinelink_add(struct RenderPipelineChain *, struct RenderObject *);
struct RenderPipelineLink *pipelinelink_gethead(struct RenderPipelineChain *, enum PipelineType);
size_t pipelinelink_getsize(struct RenderPipelineChain *, enum PipelineType);

// Render group functions
bool rendergroup_init(struct RenderGroup *);
bool rendergroup_add(struct RenderGroup *, struct RenderObject *);
bool rendergroup_destroy(struct RenderGroup *, struct Application *);

// Object functions
bool object_init(struct Application *, struct RenderObjectCreateInfo *, struct RenderObject *);
bool object_retain(struct RenderObject *);
bool object_release(struct RenderObject *);
bool object_destroy(struct RenderObject *, struct Application *);
void object_destroybuffers(struct RenderObject *, struct Application *);

#endif