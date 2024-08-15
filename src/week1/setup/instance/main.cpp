#define GLFW_INCLUDE_VULKAN
#define VK_NO_PROTOTYPES
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan.hpp>

#include <iostream>
#include <stdexcept>
#include <cstdlib>

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageType,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData) {

	std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

	return VK_FALSE;
}

inline auto findExtensionProperties(const std::vector<VkExtensionProperties>& extensionProps, const char* name) {
	for (auto& extensionProp : extensionProps) {
		if (strcmp(extensionProp.extensionName, name) == 0) {
			return true;
		}
	}
	return false;
}
inline auto findLayerProperties(const std::vector<VkLayerProperties>& layerProps, const char* name) {
	for (auto& layerProp : layerProps) {
		if (strcmp(layerProp.layerName, name) == 0) {
			return true;
		}
	}
	return false;
}

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

class HelloTriangleApplication {
public:
    void run() {
        initWindow();
        initVulkan();
        mainLoop();
        cleanup();
    }

private:
	GLFWwindow*           window = nullptr;
    VkInstance            instance = nullptr;
	PFN_vkDestroyInstance vkDestroyInstance = nullptr;
#ifndef NDEBUG
	VkDebugUtilsMessengerEXT debugMessenger = nullptr;
	PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessengerEXT = nullptr;
#endif

    void initWindow() {
        glfwInit();

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

        window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
    }

    void initVulkan() {
        initInstance();
    }

    void mainLoop() {
        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();
        }
    }

    void cleanup() {

#ifndef NDEBUG
		if (vkDestroyDebugUtilsMessengerEXT) {
			vkDestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
		}
#endif
        if (vkDestroyInstance) {
			vkDestroyInstance(instance, nullptr);
		}
        glfwDestroyWindow(window);

        glfwTerminate();
    }
    void initInstance() {
        auto vkGetInstanceProcAddr = (PFN_vkGetInstanceProcAddr)glfwGetInstanceProcAddress(nullptr, "vkGetInstanceProcAddr");
        auto vkEnumerateInstanceVersion = (PFN_vkEnumerateInstanceVersion)vkGetInstanceProcAddr(nullptr, "vkEnumerateInstanceVersion");
        auto vkEnumerateInstanceExtensionProperties = (PFN_vkEnumerateInstanceExtensionProperties)vkGetInstanceProcAddr(nullptr, "vkEnumerateInstanceExtensionProperties");
        auto vkEnumerateInstanceLayerProperties = (PFN_vkEnumerateInstanceLayerProperties)vkGetInstanceProcAddr(nullptr, "vkEnumerateInstanceLayerProperties");
		auto vkCreateInstance = (PFN_vkCreateInstance)vkGetInstanceProcAddr(nullptr, "vkCreateInstance");
        uint32_t supportedVersion = 0u;
        VkResult result  = vkEnumerateInstanceVersion(&supportedVersion);
        if (result == VK_SUCCESS) {
            std::cout << "Vulkan Version: " << VK_VERSION_MAJOR(supportedVersion) << "." << VK_VERSION_MINOR(supportedVersion) << "." << VK_VERSION_PATCH(supportedVersion) << std::endl;
		}
		else {
			throw std::runtime_error("failed to enumerate instance version");
		}

        auto requestInstanceVersion = 0u;
        if (supportedVersion      >= VK_API_VERSION_1_3) {
			requestInstanceVersion = VK_API_VERSION_1_3;
        }
		else if (supportedVersion >= VK_API_VERSION_1_2) {
			requestInstanceVersion = VK_API_VERSION_1_2;
		}
		else if (supportedVersion >= VK_API_VERSION_1_1) {
			requestInstanceVersion = VK_API_VERSION_1_1;
		}
		else {
			requestInstanceVersion = VK_API_VERSION_1_0;
		}

		VkApplicationInfo  appInfo = {};
		appInfo.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName   = "Hello Triangle";
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.pEngineName        = "No Engine";
		appInfo.engineVersion      = VK_MAKE_VERSION(1, 0, 0);
		appInfo.apiVersion         = requestInstanceVersion;
        appInfo.pNext              = nullptr;

        uint32_t        extensionCount = 0;
		auto ppExtensioNames = glfwGetRequiredInstanceExtensions(&extensionCount);

		std::vector<const char*> requestedInstanceExtensions = std::vector<const char*>(ppExtensioNames, ppExtensioNames + extensionCount);
#ifndef NDEBUG
		requestedInstanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif
		std::vector<const char*> requestedInstanceLayers     = {
			"VK_LAYER_LUNARG_api_dump"
		};
#ifndef NDEBUG
		requestedInstanceLayers.push_back("VK_LAYER_KHRONOS_validation");
#endif		

		std::vector<const char*> enabledInstanceExtensions;
		std::vector<const char*> enabledInstanceLayers;

		auto instanceExtensionPropCount = 0u;
		result = vkEnumerateInstanceExtensionProperties(nullptr, &instanceExtensionPropCount, nullptr);
		std::vector<VkExtensionProperties> extensionProps(instanceExtensionPropCount);
		result = vkEnumerateInstanceExtensionProperties(nullptr, &instanceExtensionPropCount, extensionProps.data());

		auto instanceLayerPropCount = 0u;
		result = vkEnumerateInstanceLayerProperties(&instanceLayerPropCount, nullptr);
		std::vector<VkLayerProperties> layerProps(instanceLayerPropCount);
		result = vkEnumerateInstanceLayerProperties(&instanceLayerPropCount, layerProps.data());

		for (auto& requestedInstanceExtension : requestedInstanceExtensions) {
			if (!findExtensionProperties(extensionProps, requestedInstanceExtension)) {
				throw std::runtime_error("failed to find instance extension: " + std::string(requestedInstanceExtension));
			}
		}
		for (auto& requestedInstanceLayer : requestedInstanceLayers) {
			if (!findLayerProperties(layerProps, requestedInstanceLayer)) {
				throw std::runtime_error("failed to find instance layer: " + std::string(requestedInstanceLayer));
			}
		}

		enabledInstanceExtensions = requestedInstanceExtensions;
		enabledInstanceLayers     = requestedInstanceLayers;

		VkInstanceCreateInfo createInfo    = {};
		createInfo.sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo        = &appInfo;
		createInfo.enabledExtensionCount   = enabledInstanceExtensions.size();
		createInfo.ppEnabledExtensionNames = enabledInstanceExtensions.data();
		createInfo.enabledLayerCount       = enabledInstanceLayers.size();
		createInfo.ppEnabledLayerNames     = enabledInstanceLayers.data();

		result = vkCreateInstance(&createInfo, nullptr, &instance);
		if (result == VK_SUCCESS) {
			std::cout << "Vulkan Instance created successfully" << std::endl;
		}
		else {
			throw std::runtime_error("failed to create instance");
		}
		vkDestroyInstance = (PFN_vkDestroyInstance)vkGetInstanceProcAddr(instance, "vkDestroyInstance");

#ifndef NDEBUG
		auto vkCreateDebugUtilsMessengerEXT = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
		VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo = {};
		debugCreateInfo.sType           = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		debugCreateInfo.messageType     = VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT  | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		debugCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT  | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT;
		debugCreateInfo.pfnUserCallback = debugCallback;
		result = vkCreateDebugUtilsMessengerEXT(instance, &debugCreateInfo, nullptr, &debugMessenger);
		if (result == VK_SUCCESS) {
			std::cout << "Debug Messenger created successfully" << std::endl;
		}
		else {
			throw std::runtime_error("failed to create debug messenger");
		}
		vkDestroyDebugUtilsMessengerEXT = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
#endif
    }
};

int main() {
    HelloTriangleApplication app;

    try {
        app.run();
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}