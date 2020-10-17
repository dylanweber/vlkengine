#include "engine_object.h"

/*          Object link functions         */

/**
 * @brief Initializes object link
 *
 * @param objects Object chain to initialize
 * @return Success boolean
 */
bool objectlink_init(struct RenderObjectChain *objects) {
	objects->link = NULL;
	objects->size = 0;
	return true;
}

/**
 * @brief Adds an object to the application's object link
 *
 * @param objects Object chain to add object to
 * @param render_object Object to add to application
 * @return Success boolean
 */
bool objectlink_add(struct RenderObjectChain *objects, struct RenderObject *render_object) {
	struct RenderObjectLink *curr = objects->link;

	struct RenderObjectLink *new_link = malloc(sizeof(*new_link));
	if (new_link == NULL) {
		fprintf(stderr, "Failure to allocate memory.\n");
		return false;
	}

	new_link->render_object = render_object;
	new_link->next = NULL;

	while (curr != NULL && curr->next != NULL) {
		curr = curr->next;
	}

	if (curr == NULL) {
		objects->link = new_link;
	} else {
		curr->next = new_link;
	}

	objects->size++;
	return true;
}

/**
 * @brief Retrieves the size of the object chain
 *
 * @param objects Object chain to determine size of
 * @return size_t Size of object chain
 */
size_t objectlist_getsize(struct RenderObjectChain *objects) {
	if (objects == NULL) {
		fprintf(stderr, "Invalid use of objectlist_getsize(struct RenderObjectChain *)\n");
		return 0;
	}
	if (objects->link == NULL && objects->size != 0) {
		fprintf(stderr, "RenderObjectChain is corrupted.\n");
		return 0;
	}
	return objects->size;
}

/**
 * @brief Gets the head of the linked list
 *
 * @param objects Render object chain to get head of
 * @return struct RenderObjectLink* Head of linked list
 */
struct RenderObjectLink *objectlist_gethead(struct RenderObjectChain *objects) {
	if (objects == NULL) {
		fprintf(stderr, "Invalid use of objectlist_gethead(struct RenderObjectChain *)\n");
		return NULL;
	}
	if (objects->link == NULL && objects->size != 0) {
		fprintf(stderr, "RenderObjectChain is corrupted.\n");
		return NULL;
	}
	return objects->link;
}

/**
 * @brief Destroys every object in the link and the chain itself
 *
 * @param objects Render chain to destroy
 * @param app Application the chain belongs to
 * @return Success boolean
 */
bool objectlink_destroy(struct RenderObjectChain *objects, struct Application *app) {
	struct RenderObjectLink *curr = objects->link, *next;

	while (curr != NULL) {
		object_destroy(curr->render_object, app);
		if (curr->render_object->is_static == false)
			free(curr->render_object);
		next = curr->next;
		free(curr);
		curr = next;
	}

	objects->size = 0;
	return true;
}

/*          Pipeline link functions         */

bool pipelinelink_init(struct RenderPipelineChain *pipeline_chain) {
	pipeline_chain->pl_sizes = malloc(sizeof(*pipeline_chain->pl_sizes) * NUM_PIPELINES);
	if (pipeline_chain->pl_sizes == NULL) {
		fprintf(stderr, "Failed to allocate memory.\n");
		return false;
	}

	pipeline_chain->links = malloc(sizeof(*pipeline_chain->links) * NUM_PIPELINES);
	if (pipeline_chain->links == NULL) {
		fprintf(stderr, "Failed to allocate memory.\n");
		return false;
	}

	pipeline_chain->size = NUM_PIPELINES;

	uint32_t i;
	for (i = 0; i < NUM_PIPELINES; i++) {
		pipeline_chain->links[i] = NULL;
		pipeline_chain->pl_sizes[i] = 0;
	}

	return true;
}

bool pipelinelink_add(struct RenderPipelineChain *pipeline_chain,
					  struct RenderObject *render_object) {
	struct RenderPipelineLink *curr = pipeline_chain->links[render_object->render_data.pltype];

	struct RenderPipelineLink *new_link = malloc(sizeof(*new_link));
	if (new_link == NULL) {
		fprintf(stderr, "Failed to allocate memory.\n");
		return false;
	}

	new_link->render_object = render_object;
	new_link->next = NULL;

	while (curr != NULL && curr->next != NULL) {
		curr = curr->next;
	}

	if (curr == NULL) {
		pipeline_chain->links[render_object->render_data.pltype] = new_link;
	} else {
		curr->next = new_link;
	}

	pipeline_chain->pl_sizes[render_object->render_data.pltype]++;
	return true;
}

struct RenderPipelineLink *pipelinelink_gethead(struct RenderPipelineChain *pipeline_chain,
												enum PipelineType pltype) {
	return pipeline_chain->links[pltype];
}

size_t pipelinelink_getsize(struct RenderPipelineChain *pipeline_chain, enum PipelineType pltype) {
	return pipeline_chain->pl_sizes[pltype];
}

/**
 * @brief Deletes all links in the pipeline chains for all pipelines
 *
 * @param pipeline_chain Pipeline chain to clean
 * @return Success boolean
 */
bool pipelinelink_destroy(struct RenderPipelineChain *pipeline_chain) {
	struct RenderPipelineLink *curr = NULL, *prev;
	uint32_t i;
	for (i = 0; i < NUM_PIPELINES; i++) {
		curr = pipeline_chain->links[i];
		if (curr == NULL) {
			continue;
		}

		while (curr != NULL) {
			prev = curr;
			curr = curr->next;
			free(prev);
		}
	}

	free(pipeline_chain->links);
	free(pipeline_chain->pl_sizes);
	return true;
}

/*          Render group functions         */

bool rendergroup_init(struct RenderGroup *render_group) {
	render_group->objects = malloc(sizeof(*render_group->objects));
	render_group->pipelines = malloc(sizeof(*render_group->pipelines));

	return objectlink_init(render_group->objects) && pipelinelink_init(render_group->pipelines);
}

bool rendergroup_add(struct RenderGroup *render_group, struct RenderObject *render_object) {
	return objectlink_add(render_group->objects, render_object) &&
		   pipelinelink_add(render_group->pipelines, render_object);
}

bool rendergroup_destroy(struct RenderGroup *render_group, struct Application *app) {
	bool ret = objectlink_destroy(render_group->objects, app) &&
			   pipelinelink_destroy(render_group->pipelines);
	if (ret == false)
		return ret;

	free(render_group->objects);
	free(render_group->pipelines);
	return ret;
}

/*          Render object functions         */

bool object_init(struct Application *app, struct RenderObjectCreateInfo *ro_create_info,
				 struct RenderObject *render_object) {
	render_object->render_data.pltype = ro_create_info->pltype;
	render_object->render_data.vertices_size = ro_create_info->vertices_size;
	render_object->render_data.vertices = malloc(sizeof(*render_object->render_data.vertices) *
												 render_object->render_data.vertices_size);
	if (render_object->render_data.vertices == NULL) {
		fprintf(stderr, "Failure to allocate memory.\n");
		return false;
	}

	memcpy(render_object->render_data.vertices, ro_create_info->vertices,
		   sizeof(*render_object->render_data.vertices) * ro_create_info->vertices_size);

	render_object->is_static = ro_create_info->is_static;
	render_object->retain_count = 1;
	return true;
}

bool object_retain(struct RenderObject *render_object) {
	if (render_object->is_static == true)
		return true;
	render_object->retain_count++;
	return true;
}

bool object_release(struct RenderObject *render_object) {
	free(render_object->render_data.vertices);
	if (render_object->is_static == true)
		return true;
	render_object->retain_count--;
	if (render_object->retain_count == 0)
		free(render_object);
	return true;
}

bool object_destroy(struct RenderObject *render_object, struct Application *app) {
	object_destroybuffers(render_object, app);
	return true;
}

void object_destroybuffers(struct RenderObject *render_object, struct Application *app) {
	vkmemory_destroybuffer(&app->vulkan_data->vmemory, render_object->render_data.vi_buffer);
}