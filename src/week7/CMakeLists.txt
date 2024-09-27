set (ASSETS_ROOT_DIR ${CMAKE_CURRENT_BINARY_DIR}/assets)
make_directory(${CMAKE_CURRENT_BINARY_DIR}/assets)
file(DOWNLOAD "https://vulkan-tutorial.com/images/texture.jpg" "${CMAKE_CURRENT_BINARY_DIR}/assets/texture.jpg")
add_subdirectory(Texturemapping)