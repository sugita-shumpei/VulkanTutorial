#define GLFW_INCLUDE_VULKAN
#define VK_NO_PROTOTYPES
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan.hpp>

#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <optional>
#include <set>
#include <cstdint>
#include <limits>
#include <algorithm>

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

struct SwapChainSupportDetails {
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;
};

struct QueueFamilyIndices {
	std::optional<uint32_t> graphicsFamily;
	std::optional<uint32_t> presentFamily;

	bool isComplete()
	{
		return graphicsFamily.has_value() && presentFamily.has_value();
	}
};

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
	GLFWwindow* window = nullptr;
	VkInstance                 instance = nullptr;
	VkPhysicalDevice           physicalDevice = nullptr;
	VkDevice                   device = nullptr;
	VkSurfaceKHR surface;
	VkQueue graphicsQueue;
	VkQueue presentQueue;
	VkSwapchainKHR swapChain = nullptr;
	std::vector<VkImage> swapChainImages;
	VkFormat swapChainImageFormat;
	VkExtent2D swapChainExtent;
	// NOTE:
	std::vector<VkImageView> swapChainImageViews;

	PFN_vkGetInstanceProcAddr  vkGetInstanceProcAddr = nullptr;
	PFN_vkGetDeviceProcAddr    vkGetDeviceProcAddr = nullptr;
	PFN_vkDestroyInstance      vkDestroyInstance = nullptr;
	PFN_vkDestroyDevice        vkDestroyDevice = nullptr;
	PFN_vkDestroySurfaceKHR	   vkDestroySurfaceKHR = nullptr;
	PFN_vkDestroySwapchainKHR  vkDestroySwapchainKHR = nullptr;
	// NOTE:
	PFN_vkDestroyImageView	   vkDestroyImageView = nullptr;
#ifndef NDEBUG
	VkDebugUtilsMessengerEXT            debugMessenger = nullptr;
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
		createSurface();
		selectPhysicalDevice();
		initDevice();
		createSwapChain();
		// NOTE:
		createImageViews();
	}

	void mainLoop() {
		while (!glfwWindowShouldClose(window)) {
			glfwPollEvents();
		}
	}

	void cleanup() {
		// NOTE:
		for (auto imageView : swapChainImageViews) {
			vkDestroyImageView(device, imageView, nullptr);
		}
		vkDestroySwapchainKHR(device, swapChain, nullptr);
		if (vkDestroyDevice) {
			vkDestroyDevice(device, nullptr);

		}
		if (vkDestroySurfaceKHR) {
			vkDestroySurfaceKHR(instance, surface, nullptr);
		}
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
		VkResult result = vkEnumerateInstanceVersion(&supportedVersion);
		if (result == VK_SUCCESS) {
			std::cout << "Vulkan Version: " << VK_VERSION_MAJOR(supportedVersion) << "." << VK_VERSION_MINOR(supportedVersion) << "." << VK_VERSION_PATCH(supportedVersion) << std::endl;
		}
		else {
			throw std::runtime_error("failed to enumerate instance version");
		}

		auto requestInstanceVersion = 0u;
		if (supportedVersion >= VK_API_VERSION_1_3) {
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
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = "Hello Triangle";
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.pEngineName = "No Engine";
		appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.apiVersion = requestInstanceVersion;
		appInfo.pNext = nullptr;

		uint32_t        extensionCount = 0;
		auto ppExtensioNames = glfwGetRequiredInstanceExtensions(&extensionCount);

		std::vector<const char*> requestedInstanceExtensions = std::vector<const char*>(ppExtensioNames, ppExtensioNames + extensionCount);
#ifndef NDEBUG
		requestedInstanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif
		std::vector<const char*> requestedInstanceLayers = {
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
		enabledInstanceLayers = requestedInstanceLayers;

		VkInstanceCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &appInfo;
		createInfo.enabledExtensionCount = enabledInstanceExtensions.size();
		createInfo.ppEnabledExtensionNames = enabledInstanceExtensions.data();
		createInfo.enabledLayerCount = enabledInstanceLayers.size();
		createInfo.ppEnabledLayerNames = enabledInstanceLayers.data();

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
		debugCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		debugCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		debugCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT;
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

	void createSurface()
	{
		vkDestroySurfaceKHR = (PFN_vkDestroySurfaceKHR)vkGetInstanceProcAddr(instance, "vkDestroySurfaceKHR");
		if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS) {
			throw std::runtime_error("failed to create window surface!");
		}
	}

	SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice physDev)
	{
		SwapChainSupportDetails details;
		auto vkGetPhysicalDeviceSurfaceCapabilitiesKHR = (PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR)vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceSurfaceCapabilitiesKHR"); // notice that instance, not device
		auto vkGetPhysicalDeviceSurfaceFormatsKHR = (PFN_vkGetPhysicalDeviceSurfaceFormatsKHR)vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceSurfaceFormatsKHR");
		auto vkGetPhysicalDeviceSurfacePresentModesKHR = (PFN_vkGetPhysicalDeviceSurfacePresentModesKHR)vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceSurfacePresentModesKHR");

		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physDev, surface, &details.capabilities);

		uint32_t formatCount = 0;
		vkGetPhysicalDeviceSurfaceFormatsKHR(physDev, surface, &formatCount, nullptr);
		if (formatCount != 0) {
			details.formats.resize(formatCount);
			vkGetPhysicalDeviceSurfaceFormatsKHR(physDev, surface, &formatCount, details.formats.data());
		}

		uint32_t presentModeCount = 0;
		vkGetPhysicalDeviceSurfacePresentModesKHR(physDev, surface, &presentModeCount, nullptr);
		if (presentModeCount != 0) {
			details.presentModes.resize(presentModeCount);
			vkGetPhysicalDeviceSurfacePresentModesKHR(physDev, surface, &presentModeCount, details.presentModes.data());
		}

		return details;
	}

	bool isDeviceSuitable(VkPhysicalDevice physDev)
	{
		SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physDev);
		if (!swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty()) {
			return true;
		}
		else {
			return false;
		}
	}

	void selectPhysicalDevice() {
		auto vkEnumeratePhysicalDevices = (PFN_vkEnumeratePhysicalDevices)vkGetInstanceProcAddr(instance, "vkEnumeratePhysicalDevices");
		auto vkGetPhysicalDeviceProperties = (PFN_vkGetPhysicalDeviceProperties)vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceProperties");
		auto vkGetPhysicalDeviceFeatures = (PFN_vkGetPhysicalDeviceFeatures)vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceFeatures");
		auto vkGetPhysicalDeviceFeatures2 = (PFN_vkGetPhysicalDeviceFeatures2)vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceFeatures2");
		auto vkEnumerateDeviceExtensionProperties = (PFN_vkEnumerateDeviceExtensionProperties)vkGetInstanceProcAddr(instance, "vkEnumerateDeviceExtensionProperties");
		auto vkGetPhysicalDeviceQueueFamilyProperties = (PFN_vkGetPhysicalDeviceQueueFamilyProperties)vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceQueueFamilyProperties");

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
			std::cout << "Physical Device: " << physicalDeviceProperties.deviceName << std::endl;
			std::cout << "API Version: " << VK_VERSION_MAJOR(physicalDeviceProperties.apiVersion) << "." << VK_VERSION_MINOR(physicalDeviceProperties.apiVersion) << "." << VK_VERSION_PATCH(physicalDeviceProperties.apiVersion) << std::endl;
			std::cout << "Driver Version: " << physicalDeviceProperties.driverVersion << std::endl;
			std::cout << "Vendor ID: " << physicalDeviceProperties.vendorID << std::endl;
			std::cout << "Device ID: " << physicalDeviceProperties.deviceID << std::endl;
			VkPhysicalDeviceFeatures  physicalDeviceFeatures;
			vkGetPhysicalDeviceFeatures(physDev, &physicalDeviceFeatures);
			std::cout << "GeometryShader    : " << physicalDeviceFeatures.geometryShader << std::endl;
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
			if (vkGetPhysicalDeviceFeatures2) {
				// Query Vulkan Features
				VkPhysicalDeviceFeatures2        physicalDeviceFeatures2 = {};
				VkPhysicalDeviceVulkan11Features physicalDeviceVulkan11Features = {};
				VkPhysicalDeviceVulkan12Features physicalDeviceVulkan12Features = {};
				VkPhysicalDeviceVulkan13Features physicalDeviceVulkan13Features = {};
				physicalDeviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
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
			vkGetPhysicalDeviceQueueFamilyProperties(physDev, &queueFamilyCount, nullptr);
			std::vector<VkQueueFamilyProperties> queueFamilyProps(queueFamilyCount);
			vkGetPhysicalDeviceQueueFamilyProperties(physDev, &queueFamilyCount, queueFamilyProps.data());
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

		if (physicalDevices.size() > 0 && isDeviceSuitable(physicalDevices[0])) {
			physicalDevice = physicalDevices[0];
		}
		else {
			throw std::runtime_error("failed to find a physical device with Vulkan support");
		}


	}

	QueueFamilyIndices findQueueFamilies(VkPhysicalDevice physDev)
	{
		auto vkGetPhysicalDeviceQueueFamilyProperties = (PFN_vkGetPhysicalDeviceQueueFamilyProperties)vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceQueueFamilyProperties");
		auto vkGetPhysicalDeviceSurfaceSupportKHR = (PFN_vkGetPhysicalDeviceSurfaceSupportKHR)vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceSurfaceSupportKHR");

		QueueFamilyIndices indices;

		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(physDev, &queueFamilyCount, nullptr);

		std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(physDev, &queueFamilyCount, queueFamilies.data());

		int i = 0;
		for (const auto& queueFamily : queueFamilies) {
			if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
				indices.graphicsFamily = i;
			}
			VkBool32 presentSupport = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(physDev, i, surface, &presentSupport);

			if (presentSupport) {
				indices.presentFamily = i;
			}

			if (indices.isComplete()) {
				break;
			}
			i++;
		}

		return indices;
	}

	void initDevice() {
		auto vkEnumeratePhysicalDevices = (PFN_vkEnumeratePhysicalDevices)vkGetInstanceProcAddr(instance, "vkEnumeratePhysicalDevices");
		auto vkGetPhysicalDeviceProperties = (PFN_vkGetPhysicalDeviceProperties)vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceProperties");
		auto vkGetPhysicalDeviceFeatures = (PFN_vkGetPhysicalDeviceFeatures)vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceFeatures");
		auto vkGetPhysicalDeviceFeatures2 = (PFN_vkGetPhysicalDeviceFeatures2)vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceFeatures2");
		auto vkEnumerateDeviceExtensionProperties = (PFN_vkEnumerateDeviceExtensionProperties)vkGetInstanceProcAddr(instance, "vkEnumerateDeviceExtensionProperties");
		auto vkGetPhysicalDeviceSurfaceSupportKHR = (PFN_vkGetPhysicalDeviceSurfaceSupportKHR)vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceSurfaceSupportKHR");
		auto vkGetPhysicalDeviceQueueFamilyProperties = (PFN_vkGetPhysicalDeviceQueueFamilyProperties)vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceQueueFamilyProperties");

		std::uint32_t extensionCount;
		vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, nullptr);
		std::vector<VkExtensionProperties> extensionProps(extensionCount);
		vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, extensionProps.data());

		std::vector<const char*> requestedDeviceExtensions = std::vector<const char*>{
			VK_KHR_SWAPCHAIN_EXTENSION_NAME
		};
		std::vector<const char*> enabledDeviceExtensions;
		for (auto& requestedDeviceExtension : requestedDeviceExtensions) {
			if (!findExtensionProperties(extensionProps, requestedDeviceExtension)) {
				throw std::runtime_error("failed to find device extension: " + std::string(requestedDeviceExtension));
			}
		}

		enabledDeviceExtensions = requestedDeviceExtensions;

		VkDeviceCreateInfo deviceCreateInfo = {};
		deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		deviceCreateInfo.enabledExtensionCount = requestedDeviceExtensions.size();
		deviceCreateInfo.ppEnabledExtensionNames = requestedDeviceExtensions.data();

		VkPhysicalDeviceFeatures  physicalDeviceFeatures;
		vkGetPhysicalDeviceFeatures(physicalDevice, &physicalDeviceFeatures);
		deviceCreateInfo.pEnabledFeatures = &physicalDeviceFeatures;

		auto queueFamilyCount = 0u;
		vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);
		std::vector<VkQueueFamilyProperties> queueFamilyProps(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilyProps.data());

		QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice);
		std::set<uint32_t> uniqueQueueFamilyIndices = { queueFamilyIndices.graphicsFamily.value(), queueFamilyIndices.presentFamily.value() };

		std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
		float queuePriority = 1.0f;
		for (uint32_t uniqueQueueFamilyindex : uniqueQueueFamilyIndices) {
			VkDeviceQueueCreateInfo queueCreateInfo = {};
			queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueCreateInfo.queueFamilyIndex = uniqueQueueFamilyindex;
			queueCreateInfo.queueCount = 1;
			queueCreateInfo.pQueuePriorities = &queuePriority;
			queueCreateInfos.push_back(queueCreateInfo);
		}

		deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
		deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();

		auto vkCreateDevice = (PFN_vkCreateDevice)vkGetInstanceProcAddr(instance, "vkCreateDevice");
		auto result = vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &device);
		if (result == VK_SUCCESS) {
			std::cout << "Vulkan Device created successfully" << std::endl;
		}
		else {
			throw std::runtime_error("failed to create device");
		}

		vkGetDeviceProcAddr = (PFN_vkGetDeviceProcAddr)vkGetInstanceProcAddr(instance, "vkGetDeviceProcAddr");
		auto vkGetDeviceQueue = (PFN_vkGetDeviceQueue)vkGetDeviceProcAddr(device, "vkGetDeviceQueue");
		vkDestroyDevice = (PFN_vkDestroyDevice)vkGetDeviceProcAddr(device, "vkDestroyDevice");

		vkGetDeviceQueue(device, queueFamilyIndices.graphicsFamily.value(), 0, &graphicsQueue);
		vkGetDeviceQueue(device, queueFamilyIndices.presentFamily.value(), 0, &presentQueue);
	}

	VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
	{
		for (const auto& availableFormat : availableFormats) {
			if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
				return availableFormat;
			}
		}

		return availableFormats[0];
	}

	VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes)
	{
		for (const auto& availablePresentMode : availablePresentModes) {
			if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
				return availablePresentMode;
			}
		}

		return VK_PRESENT_MODE_FIFO_KHR;
	}

	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities)
	{
		if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
			return capabilities.currentExtent;
		}
		else {
			int width, height;
			glfwGetFramebufferSize(window, &width, &height);

			VkExtent2D actualExtent = { static_cast<uint32_t>(width), static_cast<uint32_t>(height) };

			actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
			actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

			return actualExtent;
		}
	}

	void createSwapChain()
	{
		auto vkCreateSwapchainKHR = (PFN_vkCreateSwapchainKHR)vkGetDeviceProcAddr(device, "vkCreateSwapchainKHR");
		auto vkGetSwapchainImagesKHR = (PFN_vkGetSwapchainImagesKHR)vkGetDeviceProcAddr(device, "vkGetSwapchainImagesKHR");

		SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice);

		VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
		VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
		VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

		uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;

		if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
			imageCount = swapChainSupport.capabilities.maxImageCount;
		}

		VkSwapchainCreateInfoKHR createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		createInfo.surface = surface;
		createInfo.minImageCount = imageCount;
		createInfo.imageFormat = surfaceFormat.format;
		createInfo.imageColorSpace = surfaceFormat.colorSpace;
		createInfo.imageExtent = extent;
		createInfo.imageArrayLayers = 1;
		createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

		QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
		uint32_t queueFamilyIndices[] = { indices.graphicsFamily.has_value(), indices.presentFamily.has_value() };

		if (indices.graphicsFamily != indices.graphicsFamily) {
			createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			createInfo.queueFamilyIndexCount = 2;
			createInfo.pQueueFamilyIndices = queueFamilyIndices;
		}
		else {
			createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
			/*createInfo.queueFamilyIndexCount = 0;
			createInfo.pQueueFamilyIndices = nullptr;*/
		}

		createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
		createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		createInfo.presentMode = presentMode;
		createInfo.clipped = VK_TRUE;

		createInfo.oldSwapchain = VK_NULL_HANDLE;

		if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain) != VK_SUCCESS) {
			throw std::runtime_error("failed to create swap chain!");
		}

		vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
		swapChainImages.resize(imageCount);
		vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());

		swapChainImageFormat = surfaceFormat.format;
		swapChainExtent = extent;

		vkDestroySwapchainKHR = (PFN_vkDestroySwapchainKHR)vkGetDeviceProcAddr(device, "vkDestroySwapchainKHR");
	}

	// NOTE:
	void createImageViews()
	{
		auto vkCreateImageView = (PFN_vkCreateImageView)vkGetDeviceProcAddr(device, "vkCreateImageView");

		swapChainImageViews.resize(swapChainImages.size());

		for (size_t i = 0; i < swapChainImages.size(); i++) {
			VkImageViewCreateInfo createInfo{};
			createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			createInfo.image = swapChainImages[i];
			createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			createInfo.format = swapChainImageFormat;
			createInfo.components.r = VK_COMPONENT_SWIZZLE_R;
			createInfo.components.g = VK_COMPONENT_SWIZZLE_G;
			createInfo.components.b = VK_COMPONENT_SWIZZLE_B;
			createInfo.components.a = VK_COMPONENT_SWIZZLE_A;
			createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			createInfo.subresourceRange.baseMipLevel = 0;
			createInfo.subresourceRange.levelCount = 1;
			createInfo.subresourceRange.baseArrayLayer = 0;
			createInfo.subresourceRange.layerCount = 1;

			if (vkCreateImageView(device, &createInfo, nullptr, &swapChainImageViews[i]) != VK_SUCCESS) {
				throw std::runtime_error("failed to create image views!");
			}
		}

		vkDestroyImageView = (PFN_vkDestroyImageView)vkGetDeviceProcAddr(device, "vkDestroyImageView");
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