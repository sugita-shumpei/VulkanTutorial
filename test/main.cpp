#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/string_cast.hpp>
#include <iostream>
int main(int argc, const char** argv) {
	glm::vec4 vec(1.0f, 0.0f, 0.0f, 1.0f);
	std::cout << "Hello, World!" << std::endl;
	std::cout << glm::to_string(vec) << std::endl;
	return 0;
}
