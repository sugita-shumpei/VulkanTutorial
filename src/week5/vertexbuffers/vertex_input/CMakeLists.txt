# FindPackage
find_package(Vulkan     REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(glfw3       REQUIRED)
add_executable( ${PROJECT_NAME}-week5-vertexbuffers-vertex_input)
target_compile_features(${PROJECT_NAME}-week5-vertexbuffers-vertex_input PRIVATE cxx_std_20)
target_compile_options (${PROJECT_NAME}-week5-vertexbuffers-vertex_input PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/Zc:__cplusplus /utf-8>)
target_sources ( ${PROJECT_NAME}-week5-vertexbuffers-vertex_input PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp )
target_link_libraries( ${PROJECT_NAME}-week5-vertexbuffers-vertex_input PRIVATE Vulkan::Vulkan glm::glm glfw  )
