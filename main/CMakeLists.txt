cmake_minimum_required(VERSION 3.17)
add_compile_options(-fdiagnostics-color=always)

project(vlk_engine)

find_package(Python REQUIRED)

set(SOURCES main.c
			application.c
			application.h
			engine_vulkan.c
			engine_vulkan.h
			engine_object.c
			engine_object.h
			engine_vertex.c
			engine_vertex.h
			engine_vkmemory.c
			engine_vkmemory.h
			hashdata.c
			hashdata.h
			config.h)


# -mwindows on release build
if(CMAKE_BUILD_TYPE STREQUAL "Release")
	add_executable(vlkengine WIN32 ${SOURCES})
else()
	add_executable(vlkengine ${SOURCES})
endif()


set_property(TARGET vlkengine PROPERTY C_STANDARD 17)

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

# Shader compilation

add_subdirectory(shaders)

# Vulkan linking
add_compile_definitions(GLFW_INCLUDE_VULKAN)

find_package(Vulkan REQUIRED)
target_link_libraries(vlkengine ${Vulkan_LIBRARY})
include_directories(SYSTEM ${Vulkan_INCLUDE_DIR})

# pthread linking
find_package(Threads REQUIRED)
target_link_libraries(vlkengine Threads::Threads)

# Enable all warnings
target_compile_options(vlkengine PRIVATE
	-Wall
)

if (CMAKE_C_COMPILER_ID STREQUAL "GNU")
	target_compile_options(vlkengine PRIVATE
		-Wextra
	)
endif()

# CPack files
if (CMAKE_C_COMPILER_ID STREQUAL "GNU")
	find_file(PTHREAD_LIB libwinpthread-1.dll
		PATHS C:/msys64/mingw64/bin/)
	install(TARGETS vlkengine RUNTIME DESTINATION bin)
	install(FILES ${PTHREAD_LIB} DESTINATION bin)
	install(FILES 
		${CMAKE_CURRENT_BINARY_DIR}/shaders/shader2d.fs.spv 
		${CMAKE_CURRENT_BINARY_DIR}/shaders/shader2d.vs.spv 
		DESTINATION bin/shaders)
	set(CPACK_GENERATOR "NSIS")
	set(CPACK_PACKAGE_NAME "vlkengine")
	set(CPACK_PACKAGE_VENDOR "dylanweber.com")
	set(CPACK_PACKAGE_DESCRIPTION_SUMMARY
		"vlkengine - Vulkan Engine Example")

	set(CPACK_PACKAGE_VERSION "0.1.0")
	set(CPACK_PACKAGE_VERSION_MAJOR "0")
	set(CPACK_PACKAGE_VERSION_MINOR "1")
	set(CPACK_PACKAGE_VERSION_PATCH "0")
	set(CPACK_PACKAGE_INSTALL_DIRECTORY "Vulkan Example")

	include(CPack)
endif()