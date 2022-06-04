#include "engine_object.h"

/*			Engine object group functions		*/
bool objgrp_init(struct ObjectGroup *obj_grp, struct VulkanMemory *vmem) {
	int i;
	for (i = 0; i < NUM_PIPELINES; i++) {
		obj_grp->pipelines[i].pltype = i;
		obj_grp->pipelines[i].allocations = NULL;
		obj_grp->queue[i] = NULL;
		obj_grp->queue_size[i] = 0;
	}

	obj_grp->object_table = hashtable_create(OBJECT_HASHTABLE_SIZE);
	obj_grp->memory_pool = vmem;
	return true;
}

bool objgrp_queue(struct ObjectGroup *obj_grp, struct EngineObjectCreateInfo *eo_create_info) {
	struct ObjectGroupQueue *link = malloc(sizeof(*link));
	if (link == NULL) {
		fprintf(stderr, "Failure to allocate link for object group.\n");
		return false;
	}

	link->info = eo_create_info;
	link->next = NULL;

	struct ObjectGroupQueue *curr = obj_grp->queue[eo_create_info->pltype];

	// Get to end of queue
	while (curr != NULL && curr->next != NULL) {
		curr = curr->next;
	}

	// Assign to end of queue
	if (curr == NULL) {
		obj_grp->queue[eo_create_info->pltype] = link;
	} else {
		curr->next = link;
	}

	// Increment queue size
	obj_grp->queue_size[eo_create_info->pltype]++;

	return true;
}

bool objgrp_processqueue(struct ObjectGroup *obj_grp, struct Application *app) {
	// Go through every queue for each pipeline
	enum PipelineType pltype;

	for (pltype = NO_PIPELINE; pltype < NUM_PIPELINES; pltype++) {
		// Move on if nothing to process
		if (obj_grp->queue_size[pltype] == 0) {
			continue;
		}

		// Allocate engine object block
		struct EngineObjectAllocation *allocation = malloc(sizeof(*allocation));
		if (allocation == NULL) {
			fprintf(stderr, "Failure to allocate engine object block.\n");
			return false;
		}

		// Set object block values
		allocation->objects_size = obj_grp->queue_size[pltype];
		allocation->next = NULL;

		// Allocate objects
		allocation->objects = malloc(sizeof(*allocation->objects) * allocation->objects_size);
		if (allocation->objects == NULL) {
			fprintf(stderr, "Failure to allocate objects.\n");
			return false;
		}

		// Clear all object data
		memset(allocation->objects, 0, sizeof(*allocation->objects) * allocation->objects_size);

		// Go through every object on queue and create them
		// Also count bytes for buffer allocation
		struct ObjectGroupQueue *prev, *curr = obj_grp->queue[pltype];
		VkDeviceSize buffer_size = 0;
		union HashTableValue val;
		size_t i = 0;

		while (curr != NULL) {
			// Create object & put on allocated array
			object_init(&allocation->objects[i], app, curr->info);

			buffer_size += allocation->objects[i].render_data.vertices_size *
						   sizeof(*allocation->objects[i].render_data.vertices);
			buffer_size += allocation->objects[i].render_data.indices_size *
						   sizeof(*allocation->objects[i].render_data.indices);

			// Store object in hashtable
			val.ptr = &allocation->objects[i];
			hashtable_store(obj_grp->object_table, curr->info->name, val, HASHTABLE_PTR);

			// Move to next item in queue
			prev = curr;
			curr = curr->next;
			i++;

			// Free queue item
			free(prev);
			prev = NULL;
		}

		// Create buffer for objects in allocation
		struct VulkanBuffer *obj_buffer;
		bool ret = vkmemory_createbuffer(obj_grp->memory_pool, buffer_size,
										 VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
											 VK_BUFFER_USAGE_TRANSFER_DST_BIT,
										 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &obj_buffer);
		if (ret == false) {
			fprintf(stderr, "Failure creating Vulkan buffer.\n");
			return false;
		}

		// Assign buffer data to each object, copy data
		struct VulkanBuffer *temp_buff;
		VkDeviceSize v_offset = 0;
		size_t j;

		for (j = 0; j < allocation->objects_size; j++) {
			printf("Setting object buffer to %p\n", obj_buffer->buffer);
			allocation->objects[j].render_data.vi_buffer = obj_buffer;
			allocation->objects[j].render_data.vertex_offset = v_offset;

			ret = vkmemory_createbuffer(obj_grp->memory_pool,
										sizeof(*allocation->objects[j].render_data.vertices) *
											allocation->objects[j].render_data.vertices_size,
										VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
										VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
											VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
										&temp_buff);
			if (ret == false) {
				fprintf(stderr, "Failure transfering vertex data to GPU.\n");
				return false;
			}

			void *data;
			vkmemory_mapbuffer(obj_grp->memory_pool, temp_buff, &data);
			memcpy(data, allocation->objects[j].render_data.vertices,
				   sizeof(*allocation->objects[j].render_data.vertices) *
					   allocation->objects[j].render_data.vertices_size);
			vkmemory_unmapbuffer(obj_grp->memory_pool, temp_buff);

			vulkan_copybuffer(app, obj_grp->memory_pool, temp_buff, obj_buffer,
							  sizeof(*allocation->objects[j].render_data.vertices) *
								  allocation->objects[j].render_data.vertices_size,
							  v_offset);

			vkmemory_destroybuffer(obj_grp->memory_pool, temp_buff);

			v_offset += sizeof(*allocation->objects[j].render_data.vertices) *
						allocation->objects[j].render_data.vertices_size;
		}

		assert(i == obj_grp->queue_size[pltype]);

		// Put allocation on pipeline list
		struct EngineObjectAllocation *plcurr = obj_grp->pipelines[pltype].allocations;

		while (plcurr != NULL && plcurr->next != NULL) {
			plcurr = plcurr->next;
		}

		if (plcurr == NULL) {
			obj_grp->pipelines[pltype].allocations = allocation;
		} else {
			plcurr->next = allocation;
		}

		// Empty current queue
		obj_grp->queue[pltype] = NULL;
		obj_grp->queue_size[pltype] = 0;
	}

	return true;
}

bool objgrp_destroy(struct ObjectGroup *objgrp) {
	enum PipelineType pltype;

	for (pltype = NO_PIPELINE; pltype < NUM_PIPELINES; pltype++) {
		struct EngineObjectAllocation *prev, *curr = objgrp->pipelines[pltype].allocations;
		while (curr != NULL) {
			prev = curr;
			curr = curr->next;

			size_t i;
			for (i = 0; i < prev->objects_size; i++) {
				object_destroy(&prev->objects[i]);
			}

			free(prev->objects);
			prev->objects = NULL;
			free(prev);
			prev = NULL;
		}
	}

	hashtable_destroy(objgrp->object_table);
	return true;
}

/*          Render object functions         */

bool object_init(struct EngineObject *engine_object, struct Application *app,
				 struct EngineObjectCreateInfo *eo_create_info) {
	// Clear data
	memset(engine_object, 0, sizeof(*engine_object));

	// Set render data
	engine_object->render_data.pltype = eo_create_info->pltype;

	if (eo_create_info->vertices_size > 0) {
		engine_object->render_data.vertices_size = eo_create_info->vertices_size;
		engine_object->render_data.vertices = malloc(sizeof(*engine_object->render_data.vertices) *
													 engine_object->render_data.vertices_size);
		if (engine_object->render_data.vertices == NULL) {
			fprintf(stderr, "Failure to allocate memory. Line: #%d.\n", __LINE__);
			return false;
		}

		memcpy(engine_object->render_data.vertices, eo_create_info->vertices,
			   sizeof(*engine_object->render_data.vertices) * eo_create_info->vertices_size);
	} else {
		engine_object->render_data.vertices_size = 0;
		engine_object->render_data.vertices = NULL;
	}

	if (eo_create_info->indices_size > 0) {
		engine_object->render_data.indices_size = eo_create_info->indices_size;
		engine_object->render_data.indices = malloc(sizeof(*engine_object->render_data.indices) *
													engine_object->render_data.indices_size);
		if (engine_object->render_data.indices == NULL) {
			fprintf(stderr, "Failure to allocate memory. Line: #%d.\n", __LINE__);
			return false;
		}

		memcpy(engine_object->render_data.indices, eo_create_info->indices,
			   sizeof(*engine_object->render_data.indices) * eo_create_info->indices_size);
	} else {
		engine_object->render_data.indices_size = 0;
		engine_object->render_data.indices = NULL;
	}

	// Set default struct data
	engine_object->owner = app;
	memset(engine_object->pos, 0, sizeof(engine_object->pos));
	memset(engine_object->rot, 0, sizeof(engine_object->rot));
	engine_object->is_static = eo_create_info->is_static;
	engine_object->retain_count = 1;
	return true;
}

bool object_retain(struct EngineObject *engine_object) {
	engine_object->retain_count++;
	return true;
}

bool object_release(struct EngineObject *engine_object) {
	engine_object->retain_count--;
	if (engine_object->retain_count == 0)
		object_destroy(engine_object);
	return true;
}

bool object_destroy(struct EngineObject *engine_object) {
	free(engine_object->render_data.vertices);
	engine_object->render_data.vertices = NULL;
	free(engine_object->render_data.indices);
	engine_object->render_data.indices = NULL;
	object_destroybuffers(engine_object);

	return true;
}

void object_destroybuffers(struct EngineObject *engine_object) {
	vkmemory_destroybuffer(&engine_object->owner->vulkan_data->vmemory,
						   engine_object->render_data.vi_buffer);
}