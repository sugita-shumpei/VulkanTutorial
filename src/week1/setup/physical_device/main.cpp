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
static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback2(
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageType,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData) {

	std::cerr << "validation layer2: " << pCallbackData->pMessage << std::endl;

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
	GLFWwindow*                window = nullptr;
	PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr = nullptr;
    VkInstance                instance = nullptr;
	PFN_vkDestroyInstance     vkDestroyInstance = nullptr;
#ifndef NDEBUG
	VkDebugUtilsMessengerEXT debugMessenger = nullptr;
	PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessengerEXT = nullptr;
#endif
	VkPhysicalDevice           physicalDevice = nullptr;

    void initWindow() {
        glfwInit();

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

        window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
    }

    void initVulkan() {
        initInstance();
		selectPhysicalDevice();
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
		vkGetInstanceProcAddr = (PFN_vkGetInstanceProcAddr)glfwGetInstanceProcAddress(nullptr, "vkGetInstanceProcAddr");
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
		//	"VK_LAYER_LUNARG_api_dump"
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
	void selectPhysicalDevice() {
		auto vkEnumeratePhysicalDevices              = (PFN_vkEnumeratePhysicalDevices)vkGetInstanceProcAddr(instance, "vkEnumeratePhysicalDevices");
		auto vkGetPhysicalDeviceProperties            = (PFN_vkGetPhysicalDeviceProperties)vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceProperties");
		auto vkGetPhysicalDeviceFeatures              = (PFN_vkGetPhysicalDeviceFeatures)vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceFeatures");
		auto vkGetPhysicalDeviceFeatures2             = (PFN_vkGetPhysicalDeviceFeatures2)vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceFeatures2");
		auto vkEnumerateDeviceExtensionProperties     = (PFN_vkEnumerateDeviceExtensionProperties)vkGetInstanceProcAddr(instance, "vkEnumerateDeviceExtensionProperties");
		auto vkGetPhysicalDeviceQueueFamilyProperteis = (PFN_vkGetPhysicalDeviceQueueFamilyProperties)vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceQueueFamilyProperties");
		auto physicalDeviceCount = 0u;
		auto result = vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, nullptr);
		if (result != VK_SUCCESS) {
			throw std::runtime_error("failed to enumerate physical devices");
		}
		std::vector<VkPhysicalDevice> physicalDevices(physicalDeviceCount);
		result = vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, physicalDevices.data());
		if (result != VK_SUCCESS) {
			throw std::runtime_error("failed to enumerate physical devices");
		}

		for (auto& physDev : physicalDevices) {
			VkPhysicalDeviceProperties physicalDeviceProperties;
			vkGetPhysicalDeviceProperties(physDev, &physicalDeviceProperties);
			std::cout << "Physical Device: "   << physicalDeviceProperties.deviceName << std::endl;
			std::cout << "API Version: "       << VK_VERSION_MAJOR(physicalDeviceProperties.apiVersion) << "." << VK_VERSION_MINOR(physicalDeviceProperties.apiVersion) << "." << VK_VERSION_PATCH(physicalDeviceProperties.apiVersion) << std::endl;
			std::cout << "Driver Version: "    << physicalDeviceProperties.driverVersion << std::endl;
			std::cout << "Vendor ID: "         << physicalDeviceProperties.vendorID << std::endl;
			std::cout << "Device ID: "         << physicalDeviceProperties.deviceID << std::endl;
			VkPhysicalDeviceFeatures  physicalDeviceFeatures;
			vkGetPhysicalDeviceFeatures(physDev, &physicalDeviceFeatures);
			std::cout << "GeometryShader    : " << physicalDeviceFeatures.geometryShader     << std::endl;
			std::cout << "TessellationShader: " << physicalDeviceFeatures.tessellationShader << std::endl;
			std::uint32_t extensionCount;
			vkEnumerateDeviceExtensionProperties(physDev, nullptr, &extensionCount, nullptr);
			std::vector<VkExtensionProperties> extensionProps(extensionCount);
			vkEnumerateDeviceExtensionProperties(physDev, nullptr, &extensionCount, extensionProps.data());
			std::cout << "ExtensionCount: " << extensionProps.size() << std::endl;
			size_t index = 0;
			for (auto& extensionProp : extensionProps) {
				std::cout << "Extensions[" << index << "]: " << extensionProp.extensionName << std::endl;
				index++;
			}
			{
				VkPhysicalDeviceFeatures2        physicalDeviceFeatures2 = {};
				VkPhysicalDeviceVulkan11Features physicalDeviceVulkan11Features = {};
				VkPhysicalDeviceVulkan12Features physicalDeviceVulkan12Features = {};
				VkPhysicalDeviceVulkan13Features physicalDeviceVulkan13Features = {};
				physicalDeviceFeatures2.sType        = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
				physicalDeviceVulkan11Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
				physicalDeviceVulkan12Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
				physicalDeviceVulkan13Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
				physicalDeviceFeatures2.pNext = &physicalDeviceVulkan11Features;
				physicalDeviceVulkan11Features.pNext = &physicalDeviceVulkan12Features;
				physicalDeviceVulkan12Features.pNext = &physicalDeviceVulkan13Features;
				physicalDeviceVulkan13Features.pNext = nullptr;
				vkGetPhysicalDeviceFeatures2(physDev, &physicalDeviceFeatures2);
				std::cout << "BufferDeviceAddress: " << physicalDeviceVulkan12Features.bufferDeviceAddress << std::endl;
				std::cout << "DynamicRendering   : " << physicalDeviceVulkan13Features.dynamicRendering << std::endl;
			}
			auto queueFamilyCount = 0u;
			vkGetPhysicalDeviceQueueFamilyProperteis(physDev, &queueFamilyCount, nullptr);
			std::vector<VkQueueFamilyProperties> queueFamilyProps(queueFamilyCount);
			vkGetPhysicalDeviceQueueFamilyProperteis(physDev, &queueFamilyCount, queueFamilyProps.data());
			std::cout << "QueueFamilyCount: " << queueFamilyProps.size() << std::endl;
			for (auto& queueFamilyProp : queueFamilyProps) {
				std::cout << "QueueFlags: ";
				if (queueFamilyProp.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
					std::cout << "GRAPHICS |";
				}
				if (queueFamilyProp.queueFlags & VK_QUEUE_COMPUTE_BIT) {
					std::cout << "COMPUTE |";
				}
				if (queueFamilyProp.queueFlags & VK_QUEUE_TRANSFER_BIT) {
					std::cout << "TRANSFER |";
				}
				if (queueFamilyProp.queueFlags & VK_QUEUE_SPARSE_BINDING_BIT) {
					std::cout << "SPARSE_BINDING |";
				}
				if (queueFamilyProp.queueFlags & VK_QUEUE_PROTECTED_BIT) {
					std::cout << "PROTECTED |";
				}
				std::cout << std::endl;
				std::cout << "QueueCount: " << queueFamilyProp.queueCount << std::endl;
				std::cout << "TimestampValidBits: " << queueFamilyProp.timestampValidBits << std::endl;
			}

		}

		if (physicalDevices.size() > 0) {
			physicalDevice = physicalDevices[0];
		}
		else {
			throw std::runtime_error("failed to find a physical device with Vulkan support");
		}


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