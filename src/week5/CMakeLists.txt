set (WEEK5_SHADER_ROOT ${CMAKE_CURRENT_BINARY_DIR}/shaders)
make_directory(${CMAKE_CURRENT_BINARY_DIR}/shaders)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/shaders/config.h.in  ${CMAKE_CURRENT_BINARY_DIR}/shaders/config.h)
find_package (Vulkan REQUIRED COMPONENTS glslc)
add_custom_target( ${PROJECT_NAME}-week5-compile-shaders 
	COMMAND ${Vulkan_GLSLC_EXECUTABLE} -c ${CMAKE_CURRENT_SOURCE_DIR}/shaders/shader.vert -o ${CMAKE_CURRENT_BINARY_DIR}/shaders/vert.spv
	COMMAND ${Vulkan_GLSLC_EXECUTABLE} -c ${CMAKE_CURRENT_SOURCE_DIR}/shaders/shader.frag -o ${CMAKE_CURRENT_BINARY_DIR}/shaders/frag.spv
	COMMENT "Compiling <CONFIG>_OUTPUT_NAME"
)
add_subdirectory(vertexbuffers)