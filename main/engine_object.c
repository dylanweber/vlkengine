#include "engine_object.h"

bool objectlink_init(struct Application *app) {
	app->objects = NULL;
	return true;
}

bool objectlink_add(struct Application *app, struct RenderObject *render_object) {
	struct RenderObjectLink *curr = app->objects;

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
		app->objects = new_link;
	} else {
		curr->next = new_link;
	}

	return true;
}

/**
 * @brief Goes through the entire object linked list and creates VkShaderModules for all items
 *
 * @param app Current application with a logical Vulkan device
 * @return true Successfully created all shader modules
 * @return false Failed to create shader modules
 */
bool objectlink_createshadermodules(struct Application *app) {
	struct RenderObjectLink *curr = app->objects;

	while (curr != NULL) {
		if (object_processshaders(app, curr->render_object) == false) {
			return false;
		}
		curr = curr->next;
	}

	return true;
}

bool objectlink_destroy(struct Application *app) {
	struct RenderObjectLink *curr = app->objects, *next;

	while (curr != NULL) {
		object_destroy(app, curr->render_object);
		if (curr->render_object->is_static == false)
			free(curr->render_object);
		next = curr->next;
		free(curr);
		curr = next;
	}

	return true;
}

bool object_init(struct Application *app, struct RenderObjectCreateInfo *ro_create_info,
				 struct RenderObject *render_object) {
	render_object->vertex_shader_path = ro_create_info->vertex_shader_path;
	render_object->fragment_shader_path = ro_create_info->fragment_shader_path;
	render_object->is_static = ro_create_info->is_static;
	if (object_populateshaders(app, render_object) == false) {
		fprintf(stderr, "Failure reading shaders.\n");
		return false;
	}
	return true;
}

bool object_destroy(struct Application *app, struct RenderObject *render_object) {
	object_destroyshaders(app, render_object);
	return true;
}

bool object_populateshaders(struct Application *app, struct RenderObject *render_object) {
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

bool object_processshaders(struct Application *app, struct RenderObject *render_object) {
	render_object->vertex_shader =
		vulkan_createshadermodule(app, render_object->fragment_shader_data);
	render_object->fragment_shader =
		vulkan_createshadermodule(app, render_object->fragment_shader_data);
	return render_object->vertex_shader != NULL && render_object->fragment_shader != NULL;
}

void object_destroyshaders(struct Application *app, struct RenderObject *render_object) {
	vkDestroyShaderModule(app->vulkan_data->device, render_object->vertex_shader, NULL);
	vkDestroyShaderModule(app->vulkan_data->device, render_object->fragment_shader, NULL);

	vulkan_destroyshaderfile(render_object->vertex_shader_data);
	vulkan_destroyshaderfile(render_object->fragment_shader_data);
}