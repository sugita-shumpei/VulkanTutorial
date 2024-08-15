# FindPackage
find_package(Vulkan     REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(glfw3       REQUIRED)
add_executable( ${PROJECT_NAME}-week1-setup-loader)
target_compile_features(${PROJECT_NAME}-week1-setup-loader PRIVATE cxx_std_20)
target_compile_options (${PROJECT_NAME}-week1-setup-loader PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/Zc:__cplusplus /utf-8>)
target_sources ( ${PROJECT_NAME}-week1-setup-loader PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp )
target_link_libraries( ${PROJECT_NAME}-week1-setup-loader PRIVATE Vulkan::Vulkan glm::glm glfw  )
