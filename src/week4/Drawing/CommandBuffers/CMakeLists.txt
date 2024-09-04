set(SHADER_ROOT_DIR ${CMAKE_CURRENT_BINARY_DIR})
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.h.in ${CMAKE_CURRENT_BINARY_DIR}/config.h)
# FindPackage
find_package(Vulkan     REQUIRED COMPONENTS glslc)
find_package(glm CONFIG REQUIRED)
find_package(glfw3      REQUIRED)
add_custom_command(
	OUTPUT  ${CMAKE_CURRENT_BINARY_DIR}/shader.vert.spv
	COMMAND ${Vulkan_GLSLC_EXECUTABLE} -c ${CMAKE_CURRENT_SOURCE_DIR}/shader.vert -o ${CMAKE_CURRENT_BINARY_DIR}/shader.vert.spv
	DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/shader.vert 
	COMMENT "Compiling shader.vert"
)
add_custom_command(
	OUTPUT  ${CMAKE_CURRENT_BINARY_DIR}/shader.frag.spv
	COMMAND ${Vulkan_GLSLC_EXECUTABLE} -c ${CMAKE_CURRENT_SOURCE_DIR}/shader.frag -o ${CMAKE_CURRENT_BINARY_DIR}/shader.frag.spv
	DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/shader.frag 
	COMMENT "Compiling shader.frag"
)
add_executable( ${PROJECT_NAME}-week4-Drawing-CommandBuffers)
target_compile_features(${PROJECT_NAME}-week4-Drawing-CommandBuffers PRIVATE cxx_std_20)
target_compile_options (${PROJECT_NAME}-week4-Drawing-CommandBuffers PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/Zc:__cplusplus /utf-8>)
target_sources ( ${PROJECT_NAME}-week4-Drawing-CommandBuffers        PRIVATE 
	${CMAKE_CURRENT_SOURCE_DIR}/main.cpp 
	${CMAKE_CURRENT_BINARY_DIR}/shader.vert.spv
	${CMAKE_CURRENT_BINARY_DIR}/shader.frag.spv
)
target_link_libraries( ${PROJECT_NAME}-week4-Drawing-CommandBuffers     PRIVATE Vulkan::Vulkan glm::glm glfw)
target_include_directories(${PROJECT_NAME}-week4-Drawing-CommandBuffers PRIVATE ${CMAKE_CURRENT_BINARY_DIR} )
