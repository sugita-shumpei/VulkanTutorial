# FindPackage
find_package(Vulkan     REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(glfw3       REQUIRED)
add_executable( ${PROJECT_NAME}-week4-drawing-frames_in_flight)
target_compile_features(${PROJECT_NAME}-week4-drawing-frames_in_flight PRIVATE cxx_std_20)
target_compile_options (${PROJECT_NAME}-week4-drawing-frames_in_flight PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/Zc:__cplusplus /utf-8>)
target_sources ( ${PROJECT_NAME}-week4-drawing-frames_in_flight PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp )
target_link_libraries( ${PROJECT_NAME}-week4-drawing-frames_in_flight PRIVATE Vulkan::Vulkan glm::glm glfw  )
