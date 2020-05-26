cmake_minimum_required(VERSION 3.17)
add_compile_options(-fdiagnostics-color=always)

project(vlkengine-shaders LANGUAGES NONE)

find_program(
	GLSLC glslc 
	PATHS C:/VulkanSDK/*/Bin
	REQUIRED
)

set(
	SOURCES
	shader.vert	
)

foreach(SOURCE ${SOURCES})
	get_filename_component(NAME ${SOURCE} NAME_WE)
	add_custom_command(
		OUTPUT ${NAME}.spv
		DEPENDS ${SOURCE}
		COMMAND
			${GLSLC}
			-MD -MF ${SOURCE}.d
			-o ${CMAKE_CURRENT_BINARY_DIR}/${NAME}.spv -mfmt=num
			--target-env=vulkan1.1
			${CMAKE_CURRENT_SOURCE_DIR}/${SOURCE}
		DEPFILE ${SOURCE}.d
	)
	add_custom_target(
		shaders ALL
		DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${NAME}.spv
	)
endforeach()