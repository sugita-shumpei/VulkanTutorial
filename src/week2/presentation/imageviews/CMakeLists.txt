# FindPackage
find_package(Vulkan     REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(glfw3       REQUIRED)
add_executable( ${PROJECT_NAME}-week2-presentation-imageviews)
target_compile_features(${PROJECT_NAME}-week2-presentation-imageviews PRIVATE cxx_std_20)
target_compile_options (${PROJECT_NAME}-week2-presentation-imageviews PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/Zc:__cplusplus /utf-8>)
target_sources ( ${PROJECT_NAME}-week2-presentation-imageviews PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp )
target_link_libraries( ${PROJECT_NAME}-week2-presentation-imageviews PRIVATE Vulkan::Vulkan glm::glm glfw  )
