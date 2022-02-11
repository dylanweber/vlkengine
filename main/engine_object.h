#include "application.h"
#include "engine_vertex.h"
#include "engine_vulkan.h"
#include "glfw/glfw3.h"
#include "object_struct.h"

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef ENGINE_OBJECT_H
#define ENGINE_OBJECT_H

#define OBJECT_HASHTABLE_SIZE 256

// Engine object group functions
bool objgrp_init(struct ObjectGroup *, struct VulkanMemory *);
bool objgrp_queue(struct ObjectGroup *, struct EngineObjectCreateInfo *);
bool objgrp_processqueue(struct ObjectGroup *, struct Application *);
bool objgrp_createallocationbuffers(struct ObjectGroup *, struct EngineObjectAllocation *);
bool objgrp_destroy(struct ObjectGroup *);

// Object functions
bool object_init(struct EngineObject *, struct Application *, struct EngineObjectCreateInfo *);
bool object_retain(struct EngineObject *);
bool object_release(struct EngineObject *);
bool object_destroy(struct EngineObject *);
void object_destroybuffers(struct EngineObject *);

#endif