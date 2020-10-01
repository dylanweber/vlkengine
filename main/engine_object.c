#include "engine_object.h"

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

bool object_init(struct Application *app, struct RenderObjectCreateInfo *ro_create_info,
				 struct RenderObject *render_object) {
	render_object->pltype = ro_create_info->pltype;
	render_object->vertices = ro_create_info->vertices;
	render_object->vertices_size = ro_create_info->vertices_size;
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
	vkDestroyBuffer(app->vulkan_data->device, render_object->vertex_buffer, NULL);
}