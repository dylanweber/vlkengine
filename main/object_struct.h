#include "GLFW/glfw3.h"

#ifndef OBJECTS_H
#define OBJECTS_H

enum PipelineType { NO_PIPELINE, PIPELINE_2D, PIPELINE_3D, NUM_PIPELINES };

union UniformData {
	struct {
		float translate[4];
		float rotation[4];
		float scale[4];
	} u2d;
	struct {
		float model[16];
		float view[16];
		float projection[16];
	} u3d;
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
	struct VulkanBuffer *vi_buffer;
	VkDeviceSize vertex_offset;
	VkDeviceSize index_offset;

	// Vertices
	struct Vertex *vertices;
	size_t vertices_size;

	// Indices
	uint64_t *indices;
	size_t indices_size;

	// Uniform buffer as descriptor set
	VkBuffer uni_buffer;
	VkDescriptorSet desc_set;
	VkDeviceSize uniform_offset;

	// Uniforms
	union UniformData *uniforms;
	size_t uniform_size;
};

struct EngineObject {
	struct RenderData render_data;

	// x, y, z position
	float pos[3];
	// rotation along axis
	float rot[3];

	// Functional information
	struct Application *owner;

	// Memory allocation information
	uint16_t retain_count;
	bool is_static : 1;
};

struct EngineObjectCreateInfo {
	enum PipelineType pltype;

	struct Vertex *vertices;
	size_t vertices_size;

	int *indices;
	size_t indices_size;

	bool is_static;
	char name[16];
};

struct EngineObjectAllocation {
	struct EngineObject *objects;
	size_t objects_size;
	pthread_mutex_t lock;
	struct EngineObjectAllocation *next;
};

struct EnginePipeline {
	enum PipelineType pltype;
	struct EngineObjectAllocation *allocations;
};

struct ObjectGroupQueue {
	struct EngineObjectCreateInfo *info;
	struct ObjectGroupQueue *next;
};

struct ObjectGroup {
	struct VulkanMemory *memory_pool;
	struct EnginePipeline pipelines[NUM_PIPELINES];
	struct HashTable *object_table;
	struct ObjectGroupQueue *queue[NUM_PIPELINES];
	size_t queue_size[NUM_PIPELINES];
};

#endif	// OBJECTS_H