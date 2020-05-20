cmake_minimum_required(VERSION 3.17)
add_compile_options(-fdiagnostics-color=always)

project(vlkengine)

find_package(Python REQUIRED)

set(SOURCES main.c
			application.c
			application.h
			config.h)

add_executable(vlkengine ${SOURCES})

# Version and build number
add_custom_target(
	version_number ALL
	COMMAND ${Python_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/../genver.py ${CMAKE_CURRENT_SOURCE_DIR}/../
	BYPRODUCTS config.h
	COMMENT "Generating build and version number"
)

add_dependencies(vlkengine version_number)

# GLFW build
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

add_subdirectory(glfw)

target_link_libraries(vlkengine glfw)

# Vulkan linking
add_compile_definitions(GLFW_INCLUDE_VULKAN)

target_link_libraries(vlkengine vulkan)
