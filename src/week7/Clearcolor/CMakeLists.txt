# FindPackage
find_package(Vulkan     REQUIRED COMPONENTS glslc)
find_package(glm CONFIG REQUIRED)
find_package(glfw3      REQUIRED)
# STBÇåüçıÇ∑ÇÈ
add_executable( ${PROJECT_NAME}-week7-Clearcolor)

target_compile_features(${PROJECT_NAME}-week7-Clearcolor PRIVATE cxx_std_20)
target_compile_options (${PROJECT_NAME}-week7-Clearcolor PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/Zc:__cplusplus /utf-8>)

target_sources ( ${PROJECT_NAME}-week7-Clearcolor PRIVATE 
	${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
)
target_link_libraries( ${PROJECT_NAME}-week7-Clearcolor PRIVATE Vulkan::Vulkan glm::glm glfw  )
target_include_directories(${PROJECT_NAME}-week7-Clearcolor PRIVATE ${CMAKE_CURRENT_BINARY_DIR})