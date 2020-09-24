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
 * @brief Goes through the entire object linked list and creates VkShaderModules for all items
 *
 * @param objects Object chain to compile shaders for
 * @param app Current application with a logical Vulkan device
 * @return Success boolean
 */
bool objectlink_createshadermodules(struct RenderObjectChain *objects, struct Application *app) {
	struct RenderObjectLink *curr = objects->link;

	while (curr != NULL) {
		if (object_processshaders(curr->render_object, app) == false) {
			return false;
		}
		curr = curr->next;
	}

	return true;
}

/**
 * @brief Retrieves the size of the object chain
 *
 * @param objects Object chain to determine size of
 * @return size_t Size of object chain
 */
size_t objectlist_getsize(struct RenderObjectChain *objects) {
	return objects->size;
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
	render_object->vertex_shader_path = ro_create_info->vertex_shader_path;
	render_object->fragment_shader_path = ro_create_info->fragment_shader_path;
	render_object->is_static = ro_create_info->is_static;
	render_object->retain_count = 1;
	if (object_populateshaders(render_object, app) == false) {
		fprintf(stderr, "Failure reading shaders.\n");
		return false;
	}
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
	object_destroyshaders(render_object, app);
	return true;
}

bool object_populateshaders(struct RenderObject *render_object, struct Application *app) {
	char filepath[EXECUTE_PATH_LEN];

	// Create filepath from app->execute_path for vertex shader
	strcpy(filepath, app->execute_path);
	strcat(filepath, render_object->vertex_shader_path);

	// Read shader file
	render_object->vertex_shader_data = vulkan_readshaderfile(filepath);

	// Create filepath from app->execute_path for fragment shader
	strcpy(filepath, app->execute_path);
	strcat(filepath, render_object->fragment_shader_path);

	// Read shader file
	render_object->fragment_shader_data = vulkan_readshaderfile(filepath);

	// Return success based on if files were read
	return render_object->vertex_shader_data.size != 0 &&
		   render_object->fragment_shader_data.size != 0;
}

bool object_processshaders(struct RenderObject *render_object, struct Application *app) {
	render_object->vertex_shader =
		vulkan_createshadermodule(app, render_object->vertex_shader_data);
	render_object->fragment_shader =
		vulkan_createshadermodule(app, render_object->fragment_shader_data);
	return render_object->vertex_shader != NULL && render_object->fragment_shader != NULL;
}

void object_destroyshaders(struct RenderObject *render_object, struct Application *app) {
	vkDestroyShaderModule(app->vulkan_data->device, render_object->vertex_shader, NULL);
	vkDestroyShaderModule(app->vulkan_data->device, render_object->fragment_shader, NULL);

	vulkan_destroyshaderfile(render_object->vertex_shader_data);
	vulkan_destroyshaderfile(render_object->fragment_shader_data);
}