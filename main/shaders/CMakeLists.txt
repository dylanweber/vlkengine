cmake_minimum_required(VERSION 3.17)
add_compile_options(-fdiagnostics-color=always)

project(vlkengine-shaders LANGUAGES NONE)

find_program(
	GLSLC glslc 
	PATHS C:/VulkanSDK/*/Bin
	REQUIRED
)

file (GLOB SOURCES "*shader.*")

foreach(SOURCE ${SOURCES})
	get_filename_component(NAME ${SOURCE} NAME_WE)
	get_filename_component(FULL_NAME ${SOURCE} NAME)
	add_custom_command(
		OUTPUT ${NAME}.spv
		DEPENDS ${SOURCE}
		COMMAND
			${GLSLC}
			-MD -MF ${CMAKE_CURRENT_BINARY_DIR}/${FULL_NAME}.d
			-o ${CMAKE_CURRENT_BINARY_DIR}/${NAME}.spv -mfmt=num
			--target-env=vulkan1.1
			${SOURCE}
		DEPFILE  ${CMAKE_CURRENT_BINARY_DIR}/${FULL_NAME}.d
		COMMENT "Compiling ${FULL_NAME}"
	)
	add_custom_target(
		shaders ALL
		DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${NAME}.spv
	)
endforeach()