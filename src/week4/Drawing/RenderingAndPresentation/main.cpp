#define GLFW_INCLUDE_VULKAN
#define VK_NO_PROTOTYPES
#include "config.h"
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan.hpp>

#include <iostream>
#include <fstream>
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
	const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
	void *pUserData)
{

	std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

	return VK_FALSE;
}
static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback2(
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageType,
	const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
	void *pUserData)
{

	std::cerr << "validation layer2: " << pCallbackData->pMessage << std::endl;

	return VK_FALSE;
}

inline auto findExtensionProperties(const std::vector<VkExtensionProperties> &extensionProps, const char *name)
{
	for (auto &extensionProp : extensionProps)
	{
		if (strcmp(extensionProp.extensionName, name) == 0)
		{
			return true;
		}
	}
	return false;
}
inline auto findLayerProperties(const std::vector<VkLayerProperties> &layerProps, const char *name)
{
	for (auto &layerProp : layerProps)
	{
		if (strcmp(layerProp.layerName, name) == 0)
		{
			return true;
		}
	}
	return false;
}
inline auto findQueueFamilyIndices(const std::vector<VkQueueFamilyProperties> &queueFamilyProps, VkQueueFlags requiredFlags, VkQueueFlags disallowedFlags) -> std::vector<uint32_t>
{
	std::vector<uint32_t> indices;
	for (uint32_t i = 0; i < queueFamilyProps.size(); i++)
	{
		if ((queueFamilyProps[i].queueFlags & requiredFlags) == requiredFlags &&
			(queueFamilyProps[i].queueFlags & disallowedFlags) == 0)
		{
			indices.push_back(i);
		}
	}
	return indices;
}
inline auto findQueueFamilyIndices(
	VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, PFN_vkGetPhysicalDeviceSurfaceSupportKHR vkGetPhysicalDeviceSurfaceSupportKHR,
	const std::vector<VkQueueFamilyProperties> &queueFamilyProps, VkQueueFlags requiredFlags, VkQueueFlags disallowedFlags) -> std::vector<uint32_t>
{
	std::vector<uint32_t> indices;
	for (uint32_t i = 0; i < queueFamilyProps.size(); i++)
	{
		if ((queueFamilyProps[i].queueFlags & requiredFlags) == requiredFlags &&
			(queueFamilyProps[i].queueFlags & disallowedFlags) == 0)
		{
			if (!surface)
			{
				indices.push_back(i);
			}
			else
			{
				VkBool32 presentSupport = false;
				vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &presentSupport);
				if (presentSupport)
				{
					indices.push_back(i);
				}
			}
		}
	}
	return indices;
}

struct SwapChainSupportDetails
{
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;
};

struct QueueFamilyIndices
{
	std::optional<uint32_t> graphicsFamily;
	std::optional<uint32_t> presentFamily;

	bool isComplete()
	{
		return graphicsFamily.has_value() && presentFamily.has_value();
	}
};

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

struct CommandPool
{
	CommandPool() = default;
	CommandPool(
		VkDevice device_, VkCommandPool commandPool_,
		PFN_vkAllocateCommandBuffers vkAllocateCommandBuffers,
		PFN_vkResetCommandPool vkResetCommandPool,
		PFN_vkDestroyCommandPool vkDestroyCommandPool) : device(device_), commandPool(commandPool_),
														 vkAllocateCommandBuffers(vkAllocateCommandBuffers),
														 vkResetCommandPool(vkResetCommandPool),
														 vkDestroyCommandPool(vkDestroyCommandPool)
	{
	}

	~CommandPool()
	{
		clear();
	}

	CommandPool(const CommandPool &) = delete;
	CommandPool &operator=(const CommandPool &) = delete;
	CommandPool(CommandPool &&other) noexcept
	{
		device = other.device;
		commandPool = other.commandPool;
		usedCommandBuffersForPrimary = std::move(other.usedCommandBuffersForPrimary);
		freeCommandBuffersForPrimary = std::move(other.freeCommandBuffersForPrimary);
		usedCommandBuffersForSecondary = std::move(other.usedCommandBuffersForSecondary);
		freeCommandBuffersForSecondary = std::move(other.freeCommandBuffersForSecondary);
		vkAllocateCommandBuffers = other.vkAllocateCommandBuffers;
		vkResetCommandPool = other.vkResetCommandPool;
		vkDestroyCommandPool = other.vkDestroyCommandPool;
		other.device = nullptr;
		other.commandPool = nullptr;
		other.vkAllocateCommandBuffers = nullptr;
		other.vkResetCommandPool = nullptr;
		other.vkDestroyCommandPool = nullptr;
	}
	CommandPool &operator=(CommandPool &&other) noexcept
	{
		if (this != &other)
		{
			clear();
			device = other.device;
			commandPool = other.commandPool;
			usedCommandBuffersForPrimary = std::move(other.usedCommandBuffersForPrimary);
			freeCommandBuffersForPrimary = std::move(other.freeCommandBuffersForPrimary);
			usedCommandBuffersForSecondary = std::move(other.usedCommandBuffersForSecondary);
			freeCommandBuffersForSecondary = std::move(other.freeCommandBuffersForSecondary);
			vkAllocateCommandBuffers = other.vkAllocateCommandBuffers;
			vkResetCommandPool = other.vkResetCommandPool;
			vkDestroyCommandPool = other.vkDestroyCommandPool;
			other.device = nullptr;
			other.commandPool = nullptr;
			other.vkAllocateCommandBuffers = nullptr;
			other.vkResetCommandPool = nullptr;
			other.vkDestroyCommandPool = nullptr;
		}
		return *this;
	}
	//
	void clear()
	{
		if (vkDestroyCommandPool)
		{
			vkDestroyCommandPool(device, commandPool, nullptr);
		}
		usedCommandBuffersForPrimary.clear();
		freeCommandBuffersForPrimary.clear();
		usedCommandBuffersForSecondary.clear();
		freeCommandBuffersForSecondary.clear();
		device = nullptr;
		commandPool = nullptr;
		vkAllocateCommandBuffers = nullptr;
		vkResetCommandPool = nullptr;
		vkDestroyCommandPool = nullptr;
	}
	// 毎フレームの最初にResetを呼び出すこと
	void reset(VkFlags flags = 0)
	{
		if (vkResetCommandPool)
		{
			vkResetCommandPool(device, commandPool, flags);
		}
		// usedCommandBuffersForPrimaryをfreeCommandBuffersForPrimaryに移動
		freeCommandBuffersForPrimary.insert(freeCommandBuffersForPrimary.end(), usedCommandBuffersForPrimary.begin(), usedCommandBuffersForPrimary.end());
		usedCommandBuffersForPrimary.clear();
		// usedCommandBuffersForSecondaryをfreeCommandBuffersForSecondaryに移動
		freeCommandBuffersForSecondary.insert(freeCommandBuffersForSecondary.end(), usedCommandBuffersForSecondary.begin(), usedCommandBuffersForSecondary.end());
		usedCommandBuffersForSecondary.clear();
	}
	// すでに確保されたコマンドバッファがあれば, それを再利用し, 足りなければ新たに確保する
	auto acquireCommandBuffers(VkCommandBufferLevel level, uint32_t count) -> std::vector<VkCommandBuffer>
	{
		std::vector<VkCommandBuffer> commandBuffers;
		commandBuffers.reserve(count);
		auto &freeCommandBuffers = (level == VK_COMMAND_BUFFER_LEVEL_PRIMARY) ? freeCommandBuffersForPrimary : freeCommandBuffersForSecondary;
		auto &usedCommandBuffers = (level == VK_COMMAND_BUFFER_LEVEL_PRIMARY) ? usedCommandBuffersForPrimary : usedCommandBuffersForSecondary;
		auto reuseCount = std::min(count, static_cast<uint32_t>(freeCommandBuffers.size()));
		auto allocCount = count - reuseCount;
		if (reuseCount > 0)
		{
			commandBuffers = std::vector<VkCommandBuffer>(freeCommandBuffers.end() - reuseCount, freeCommandBuffers.end());
			freeCommandBuffers.resize(freeCommandBuffers.size() - reuseCount);
		}
		if (allocCount > 0)
		{
			VkCommandBufferAllocateInfo allocInfo = {};
			allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
			allocInfo.commandPool = commandPool;
			allocInfo.level = level;
			allocInfo.commandBufferCount = allocCount;
			auto newCommandBuffers = std::vector<VkCommandBuffer>(allocCount);
			if (vkAllocateCommandBuffers(device, &allocInfo, newCommandBuffers.data()) != VK_SUCCESS)
			{
				throw std::runtime_error("failed to allocate command buffers!");
			}
			commandBuffers.insert(commandBuffers.end(), newCommandBuffers.begin(), newCommandBuffers.end());
		}
		usedCommandBuffers.reserve(usedCommandBuffers.size() + commandBuffers.size());
		usedCommandBuffers.insert(usedCommandBuffers.end(), commandBuffers.begin(), commandBuffers.end());
		return commandBuffers;
	}
	auto acquireCommandBuffer(VkCommandBufferLevel level) -> VkCommandBuffer
	{
		auto commandBuffers = acquireCommandBuffers(level, 1);
		return commandBuffers[0];
	}

	VkDevice device = nullptr;
	VkCommandPool commandPool = nullptr;
	std::vector<VkCommandBuffer> freeCommandBuffersForPrimary = {};
	std::vector<VkCommandBuffer> usedCommandBuffersForPrimary = {};
	std::vector<VkCommandBuffer> freeCommandBuffersForSecondary = {};
	std::vector<VkCommandBuffer> usedCommandBuffersForSecondary = {};
	PFN_vkAllocateCommandBuffers vkAllocateCommandBuffers = nullptr;
	PFN_vkResetCommandPool vkResetCommandPool = nullptr;
	PFN_vkDestroyCommandPool vkDestroyCommandPool = nullptr;
};

class HelloTriangleApplication
{
public:
	void run()
	{
		initWindow();
		initVulkan();
		mainLoop();
		cleanup();
	}

private:
	GLFWwindow *window = nullptr;
	VkInstance instance = nullptr;
	VkPhysicalDevice physicalDevice = nullptr;
	VkDevice device = nullptr;
	VkSurfaceKHR surface = nullptr;
	VkQueue graphicsQueue = nullptr;
	VkQueue presentQueue = nullptr;
	VkSwapchainKHR swapChain = nullptr;
	std::vector<VkImage> swapChainImages = {};
	VkFormat swapChainImageFormat = VK_FORMAT_UNDEFINED;
	VkExtent2D swapChainExtent = VkExtent2D{};
	std::vector<VkImageView> swapChainImageViews = {};
	std::vector<VkFramebuffer> swapChainFramebuffers = {};

	VkShaderModule vertShaderModule = nullptr;
	VkShaderModule fragShaderModule = nullptr;
	VkPipelineLayout pipelineLayout = nullptr;
	VkRenderPass renderPass = nullptr;
	VkPipeline graphicsPipeline = nullptr;
	CommandPool commandPool = {};
	VkFence inFlightFence = nullptr;
	VkSemaphore imageAvailableSemaphore = nullptr;
	VkSemaphore renderFinishedSemaphore = nullptr;

	PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr = nullptr;
	PFN_vkGetDeviceProcAddr vkGetDeviceProcAddr = nullptr;
	PFN_vkDestroyInstance vkDestroyInstance = nullptr;
	PFN_vkDestroyDevice vkDestroyDevice = nullptr;
	PFN_vkDestroySurfaceKHR vkDestroySurfaceKHR = nullptr;
	PFN_vkDestroySwapchainKHR vkDestroySwapchainKHR = nullptr;
	PFN_vkDestroyImageView vkDestroyImageView = nullptr;
	PFN_vkDestroyShaderModule vkDestroyShaderModule = nullptr;
	PFN_vkDestroyPipelineLayout vkDestroyPipelineLayout = nullptr;
	PFN_vkDestroyRenderPass vkDestroyRenderPass = nullptr;
	PFN_vkDestroyPipeline vkDestroyPipeline = nullptr;
	PFN_vkDestroyFramebuffer vkDestroyFramebuffer = nullptr;
	PFN_vkDestroyFence vkDestroyFence = nullptr;
	PFN_vkDestroySemaphore vkDestroySemaphore = nullptr;

#ifndef NDEBUG
	VkDebugUtilsMessengerEXT debugMessenger = nullptr;
	PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessengerEXT = nullptr;
#endif

	void initWindow()
	{
		glfwInit();

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

		window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
	}

	void initVulkan()
	{
		initInstance();
		createSurface();
		selectPhysicalDevice();
		initDevice();
		createSwapChain();
		createImageViews();
		createRenderPass();
		createGraphicsPipeline();
		createFramebuffers();
		createCommandPools();
		createSyncObjects();
	}

	void mainLoop()
	{
		while (!glfwWindowShouldClose(window))
		{
			glfwPollEvents();
			drawFrame();
		}
	}

	void cleanup()
	{
		PFN_vkDeviceWaitIdle vkDeviceWaitIdle = (PFN_vkDeviceWaitIdle)vkGetDeviceProcAddr(device, "vkDeviceWaitIdle");
		vkDeviceWaitIdle(device);

		if (vkDestroyFence)
		{
			vkDestroyFence(device, inFlightFence, nullptr);
		}
		if (vkDestroySemaphore)
		{
			vkDestroySemaphore(device, renderFinishedSemaphore, nullptr);
			vkDestroySemaphore(device, imageAvailableSemaphore, nullptr);
		}

		commandPool.clear();
		if (vkDestroyFramebuffer)
		{
			for (auto framebuffer : swapChainFramebuffers)
			{
				vkDestroyFramebuffer(device, framebuffer, nullptr);
			}
		}
		if (vkDestroyPipeline)
		{
			vkDestroyPipeline(device, graphicsPipeline, nullptr);
		}
		if (vkDestroyPipelineLayout)
		{
			vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
		}
		if (vkDestroyRenderPass)
		{
			vkDestroyRenderPass(device, renderPass, nullptr);
		}
		if (vkDestroyShaderModule)
		{
			vkDestroyShaderModule(device, vertShaderModule, nullptr);
			vkDestroyShaderModule(device, fragShaderModule, nullptr);
		}
		for (auto imageView : swapChainImageViews)
		{
			vkDestroyImageView(device, imageView, nullptr);
		}
		if (vkDestroySwapchainKHR)
		{
			vkDestroySwapchainKHR(device, swapChain, nullptr);
		}
		if (vkDestroyDevice)
		{
			vkDestroyDevice(device, nullptr);
		}
#ifndef NDEBUG
		if (vkDestroyDebugUtilsMessengerEXT)
		{
			vkDestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
		}
#endif
		if (vkDestroyInstance)
		{
			vkDestroySurfaceKHR(instance, surface, nullptr);
			vkDestroyInstance(instance, nullptr);
		}
		glfwDestroyWindow(window);
		glfwTerminate();
	}

	void initInstance()
	{
		vkGetInstanceProcAddr = (PFN_vkGetInstanceProcAddr)glfwGetInstanceProcAddress(nullptr, "vkGetInstanceProcAddr");
		auto vkEnumerateInstanceVersion = (PFN_vkEnumerateInstanceVersion)vkGetInstanceProcAddr(nullptr, "vkEnumerateInstanceVersion");
		auto vkEnumerateInstanceExtensionProperties = (PFN_vkEnumerateInstanceExtensionProperties)vkGetInstanceProcAddr(nullptr, "vkEnumerateInstanceExtensionProperties");
		auto vkEnumerateInstanceLayerProperties = (PFN_vkEnumerateInstanceLayerProperties)vkGetInstanceProcAddr(nullptr, "vkEnumerateInstanceLayerProperties");
		auto vkCreateInstance = (PFN_vkCreateInstance)vkGetInstanceProcAddr(nullptr, "vkCreateInstance");

		uint32_t supportedVersion = 0u;
		VkResult result = vkEnumerateInstanceVersion(&supportedVersion);
		if (result == VK_SUCCESS)
		{
			std::cout << "Vulkan Version: " << VK_VERSION_MAJOR(supportedVersion) << "." << VK_VERSION_MINOR(supportedVersion) << "." << VK_VERSION_PATCH(supportedVersion) << std::endl;
		}
		else
		{
			throw std::runtime_error("failed to enumerate instance version");
		}

		auto requestInstanceVersion = 0u;
		if (supportedVersion >= VK_API_VERSION_1_3)
		{
			requestInstanceVersion = VK_API_VERSION_1_3;
		}
		else if (supportedVersion >= VK_API_VERSION_1_2)
		{
			requestInstanceVersion = VK_API_VERSION_1_2;
		}
		else if (supportedVersion >= VK_API_VERSION_1_1)
		{
			requestInstanceVersion = VK_API_VERSION_1_1;
		}
		else
		{
			requestInstanceVersion = VK_API_VERSION_1_0;
		}

		VkApplicationInfo appInfo = {};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = "Hello Triangle";
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.pEngineName = "No Engine";
		appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.apiVersion = requestInstanceVersion;
		appInfo.pNext = nullptr;

		uint32_t extensionCount = 0;
		auto ppExtensioNames = glfwGetRequiredInstanceExtensions(&extensionCount);

		std::vector<const char *> requestedInstanceExtensions = std::vector<const char *>(ppExtensioNames, ppExtensioNames + extensionCount);
#ifndef NDEBUG
		requestedInstanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif
		std::vector<const char *> requestedInstanceLayers = {
			//	"VK_LAYER_LUNARG_api_dump"
		};
#ifndef NDEBUG
		requestedInstanceLayers.push_back("VK_LAYER_KHRONOS_validation");
#endif

		std::vector<const char *> enabledInstanceExtensions;
		std::vector<const char *> enabledInstanceLayers;

		auto instanceExtensionPropCount = 0u;
		result = vkEnumerateInstanceExtensionProperties(nullptr, &instanceExtensionPropCount, nullptr);
		std::vector<VkExtensionProperties> extensionProps(instanceExtensionPropCount);
		result = vkEnumerateInstanceExtensionProperties(nullptr, &instanceExtensionPropCount, extensionProps.data());

		auto instanceLayerPropCount = 0u;
		result = vkEnumerateInstanceLayerProperties(&instanceLayerPropCount, nullptr);
		std::vector<VkLayerProperties> layerProps(instanceLayerPropCount);
		result = vkEnumerateInstanceLayerProperties(&instanceLayerPropCount, layerProps.data());

		for (auto &requestedInstanceExtension : requestedInstanceExtensions)
		{
			if (!findExtensionProperties(extensionProps, requestedInstanceExtension))
			{
				throw std::runtime_error("failed to find instance extension: " + std::string(requestedInstanceExtension));
			}
		}
		for (auto &requestedInstanceLayer : requestedInstanceLayers)
		{
			if (!findLayerProperties(layerProps, requestedInstanceLayer))
			{
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
		if (result == VK_SUCCESS)
		{
			std::cout << "Vulkan Instance created successfully" << std::endl;
		}
		else
		{
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
		if (result == VK_SUCCESS)
		{
			std::cout << "Debug Messenger created successfully" << std::endl;
		}
		else
		{
			throw std::runtime_error("failed to create debug messenger");
		}
		vkDestroyDebugUtilsMessengerEXT = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
#endif
	}

	void createSurface()
	{
		vkDestroySurfaceKHR = (PFN_vkDestroySurfaceKHR)vkGetInstanceProcAddr(instance, "vkDestroySurfaceKHR");
		if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS)
		{
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
		if (formatCount != 0)
		{
			details.formats.resize(formatCount);
			vkGetPhysicalDeviceSurfaceFormatsKHR(physDev, surface, &formatCount, details.formats.data());
		}

		uint32_t presentModeCount = 0;
		vkGetPhysicalDeviceSurfacePresentModesKHR(physDev, surface, &presentModeCount, nullptr);
		if (presentModeCount != 0)
		{
			details.presentModes.resize(presentModeCount);
			vkGetPhysicalDeviceSurfacePresentModesKHR(physDev, surface, &presentModeCount, details.presentModes.data());
		}

		return details;
	}

	bool isDeviceSuitable(VkPhysicalDevice physDev)
	{
		SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physDev);
		if (!swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty())
		{
			return true;
		}
		else
		{
			return false;
		}
	}

	void selectPhysicalDevice()
	{
		auto vkEnumeratePhysicalDevices = (PFN_vkEnumeratePhysicalDevices)vkGetInstanceProcAddr(instance, "vkEnumeratePhysicalDevices");
		auto vkGetPhysicalDeviceProperties = (PFN_vkGetPhysicalDeviceProperties)vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceProperties");
		auto vkGetPhysicalDeviceFeatures = (PFN_vkGetPhysicalDeviceFeatures)vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceFeatures");
		auto vkGetPhysicalDeviceFeatures2 = (PFN_vkGetPhysicalDeviceFeatures2)vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceFeatures2");
		auto vkEnumerateDeviceExtensionProperties = (PFN_vkEnumerateDeviceExtensionProperties)vkGetInstanceProcAddr(instance, "vkEnumerateDeviceExtensionProperties");
		auto vkGetPhysicalDeviceQueueFamilyProperties = (PFN_vkGetPhysicalDeviceQueueFamilyProperties)vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceQueueFamilyProperties");

		auto physicalDeviceCount = 0u;
		auto result = vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, nullptr);
		if (result != VK_SUCCESS)
		{
			throw std::runtime_error("failed to enumerate physical devices");
		}
		std::vector<VkPhysicalDevice> physicalDevices(physicalDeviceCount);
		result = vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, physicalDevices.data());
		if (result != VK_SUCCESS)
		{
			throw std::runtime_error("failed to enumerate physical devices");
		}

		for (auto &physDev : physicalDevices)
		{
			VkPhysicalDeviceProperties physicalDeviceProperties;
			vkGetPhysicalDeviceProperties(physDev, &physicalDeviceProperties);
			std::cout << "Physical Device: " << physicalDeviceProperties.deviceName << std::endl;
			std::cout << "API Version: " << VK_VERSION_MAJOR(physicalDeviceProperties.apiVersion) << "." << VK_VERSION_MINOR(physicalDeviceProperties.apiVersion) << "." << VK_VERSION_PATCH(physicalDeviceProperties.apiVersion) << std::endl;
			std::cout << "Driver Version: " << physicalDeviceProperties.driverVersion << std::endl;
			std::cout << "Vendor ID: " << physicalDeviceProperties.vendorID << std::endl;
			std::cout << "Device ID: " << physicalDeviceProperties.deviceID << std::endl;
			VkPhysicalDeviceFeatures physicalDeviceFeatures;
			vkGetPhysicalDeviceFeatures(physDev, &physicalDeviceFeatures);
			std::cout << "GeometryShader    : " << physicalDeviceFeatures.geometryShader << std::endl;
			std::cout << "TessellationShader: " << physicalDeviceFeatures.tessellationShader << std::endl;
			std::uint32_t extensionCount;
			vkEnumerateDeviceExtensionProperties(physDev, nullptr, &extensionCount, nullptr);
			std::vector<VkExtensionProperties> extensionProps(extensionCount);
			vkEnumerateDeviceExtensionProperties(physDev, nullptr, &extensionCount, extensionProps.data());
			std::cout << "ExtensionCount: " << extensionProps.size() << std::endl;
			size_t index = 0;
			for (auto &extensionProp : extensionProps)
			{
				std::cout << "Extensions[" << index << "]: " << extensionProp.extensionName << std::endl;
				index++;
			}
			if (vkGetPhysicalDeviceFeatures2)
			{
				// Query Vulkan Features
				VkPhysicalDeviceFeatures2 physicalDeviceFeatures2 = {};
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
			for (auto &queueFamilyProp : queueFamilyProps)
			{
				std::cout << "QueueFlags: ";
				if (queueFamilyProp.queueFlags & VK_QUEUE_GRAPHICS_BIT)
				{
					std::cout << "GRAPHICS |";
				}
				if (queueFamilyProp.queueFlags & VK_QUEUE_COMPUTE_BIT)
				{
					std::cout << "COMPUTE |";
				}
				if (queueFamilyProp.queueFlags & VK_QUEUE_TRANSFER_BIT)
				{
					std::cout << "TRANSFER |";
				}
				if (queueFamilyProp.queueFlags & VK_QUEUE_SPARSE_BINDING_BIT)
				{
					std::cout << "SPARSE_BINDING |";
				}
				if (queueFamilyProp.queueFlags & VK_QUEUE_PROTECTED_BIT)
				{
					std::cout << "PROTECTED |";
				}
				std::cout << std::endl;
				std::cout << "QueueCount: " << queueFamilyProp.queueCount << std::endl;
				std::cout << "TimestampValidBits: " << queueFamilyProp.timestampValidBits << std::endl;
			}
		}

		if (physicalDevices.size() > 0 && isDeviceSuitable(physicalDevices[0]))
		{
			physicalDevice = physicalDevices[0];
		}
		else
		{
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
		for (const auto &queueFamily : queueFamilies)
		{
			if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
			{
				indices.graphicsFamily = i;
			}
			VkBool32 presentSupport = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(physDev, i, surface, &presentSupport);

			if (presentSupport)
			{
				indices.presentFamily = i;
			}

			if (indices.isComplete())
			{
				break;
			}
			i++;
		}

		return indices;
	}

	void initDevice()
	{
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

		std::vector<const char *> requestedDeviceExtensions = std::vector<const char *>{
			VK_KHR_SWAPCHAIN_EXTENSION_NAME};
		std::vector<const char *> enabledDeviceExtensions;
		for (auto &requestedDeviceExtension : requestedDeviceExtensions)
		{
			if (!findExtensionProperties(extensionProps, requestedDeviceExtension))
			{
				throw std::runtime_error("failed to find device extension: " + std::string(requestedDeviceExtension));
			}
		}

		enabledDeviceExtensions = requestedDeviceExtensions;

		VkDeviceCreateInfo deviceCreateInfo = {};
		deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		deviceCreateInfo.enabledExtensionCount = requestedDeviceExtensions.size();
		deviceCreateInfo.ppEnabledExtensionNames = requestedDeviceExtensions.data();

		VkPhysicalDeviceFeatures physicalDeviceFeatures;
		vkGetPhysicalDeviceFeatures(physicalDevice, &physicalDeviceFeatures);
		deviceCreateInfo.pEnabledFeatures = &physicalDeviceFeatures;

		auto queueFamilyCount = 0u;
		vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);
		std::vector<VkQueueFamilyProperties> queueFamilyProps(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilyProps.data());

		QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice);
		std::set<uint32_t> uniqueQueueFamilyIndices = {queueFamilyIndices.graphicsFamily.value(), queueFamilyIndices.presentFamily.value()};

		std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
		float queuePriority = 1.0f;
		for (uint32_t uniqueQueueFamilyindex : uniqueQueueFamilyIndices)
		{
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
		if (result == VK_SUCCESS)
		{
			std::cout << "Vulkan Device created successfully" << std::endl;
		}
		else
		{
			throw std::runtime_error("failed to create device");
		}

		vkGetDeviceProcAddr = (PFN_vkGetDeviceProcAddr)vkGetInstanceProcAddr(instance, "vkGetDeviceProcAddr");
		auto vkGetDeviceQueue = (PFN_vkGetDeviceQueue)vkGetDeviceProcAddr(device, "vkGetDeviceQueue");
		vkDestroyDevice = (PFN_vkDestroyDevice)vkGetDeviceProcAddr(device, "vkDestroyDevice");

		vkGetDeviceQueue(device, queueFamilyIndices.graphicsFamily.value(), 0, &graphicsQueue);
		vkGetDeviceQueue(device, queueFamilyIndices.presentFamily.value(), 0, &presentQueue);
	}

	VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats)
	{
		for (const auto &availableFormat : availableFormats)
		{
			if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
			{
				return availableFormat;
			}
		}

		return availableFormats[0];
	}

	VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes)
	{
		for (const auto &availablePresentMode : availablePresentModes)
		{
			if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
			{
				return availablePresentMode;
			}
		}

		return VK_PRESENT_MODE_FIFO_KHR;
	}

	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities)
	{
		if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
		{
			return capabilities.currentExtent;
		}
		else
		{
			int width, height;
			glfwGetFramebufferSize(window, &width, &height);

			VkExtent2D actualExtent = {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};

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

		if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount)
		{
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
		uint32_t queueFamilyIndices[] = {indices.graphicsFamily.has_value(), indices.presentFamily.has_value()};

		if (indices.graphicsFamily != indices.presentFamily)
		{
			createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			createInfo.queueFamilyIndexCount = 2;
			createInfo.pQueueFamilyIndices = queueFamilyIndices;
		}
		else
		{
			createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
			/*createInfo.queueFamilyIndexCount = 0;
			createInfo.pQueueFamilyIndices = nullptr;*/
		}

		createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
		createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		createInfo.presentMode = presentMode;
		createInfo.clipped = VK_TRUE;

		createInfo.oldSwapchain = VK_NULL_HANDLE;

		if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create swap chain!");
		}

		vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
		swapChainImages.resize(imageCount);
		vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());

		swapChainImageFormat = surfaceFormat.format;
		swapChainExtent = extent;

		vkDestroySwapchainKHR = (PFN_vkDestroySwapchainKHR)vkGetDeviceProcAddr(device, "vkDestroySwapchainKHR");
	}

	void createImageViews()
	{
		auto vkCreateImageView = (PFN_vkCreateImageView)vkGetDeviceProcAddr(device, "vkCreateImageView");

		swapChainImageViews.resize(swapChainImages.size());

		for (size_t i = 0; i < swapChainImages.size(); i++)
		{
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

			if (vkCreateImageView(device, &createInfo, nullptr, &swapChainImageViews[i]) != VK_SUCCESS)
			{
				throw std::runtime_error("failed to create image views!");
			}
		}

		vkDestroyImageView = (PFN_vkDestroyImageView)vkGetDeviceProcAddr(device, "vkDestroyImageView");
	}

	// note
	void createRenderPass()
	{
		VkAttachmentDescription colorAttachment{};
		colorAttachment.format = swapChainImageFormat;
		colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		VkAttachmentReference colorAttachmentRef{};
		colorAttachmentRef.attachment = 0;
		colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpass{};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &colorAttachmentRef;

		VkRenderPassCreateInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.attachmentCount = 1;
		renderPassInfo.pAttachments = &colorAttachment;
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpass;

		auto vkCreateRenderPass = (PFN_vkCreateRenderPass)vkGetInstanceProcAddr(instance, "vkCreateRenderPass");
		if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create render pass");
		}

		vkDestroyRenderPass = (PFN_vkDestroyRenderPass)vkGetDeviceProcAddr(device, "vkDestroyRenderPass");
	}

	void createGraphicsPipeline()
	{
		auto vertShaderCode = readFile(SHADER_ROOT_DIR "/shader.vert.spv");
		auto fragShaderCode = readFile(SHADER_ROOT_DIR "/shader.frag.spv");

		vertShaderModule = createShaderModule(vertShaderCode);
		fragShaderModule = createShaderModule(fragShaderCode);

		VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
		vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vertShaderStageInfo.module = vertShaderModule;
		vertShaderStageInfo.pName = "main";

		VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
		fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		fragShaderStageInfo.module = fragShaderModule;
		fragShaderStageInfo.pName = "main";

		VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

		// note
		VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
		vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputInfo.vertexBindingDescriptionCount = 0;
		vertexInputInfo.vertexAttributeDescriptionCount = 0;

		VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
		inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		inputAssembly.primitiveRestartEnable = VK_FALSE;

		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = (float)swapChainExtent.width;
		viewport.height = (float)swapChainExtent.height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		VkRect2D scissor{};
		scissor.offset = {0, 0};
		scissor.extent = swapChainExtent;

		VkPipelineViewportStateCreateInfo viewportState{};
		viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportState.viewportCount = 1;
		viewportState.pViewports = &viewport;
		viewportState.scissorCount = 1;
		viewportState.pScissors = &scissor;

		VkPipelineRasterizationStateCreateInfo rasterizer{};
		rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizer.depthClampEnable = VK_FALSE;
		rasterizer.rasterizerDiscardEnable = VK_FALSE;
		rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
		rasterizer.lineWidth = 1.0f;
		rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
		rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
		rasterizer.depthBiasEnable = VK_FALSE;

		VkPipelineMultisampleStateCreateInfo multisampling{};
		multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampling.sampleShadingEnable = VK_FALSE;
		multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

		VkPipelineColorBlendAttachmentState colorBlendAttachment{};
		colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		colorBlendAttachment.blendEnable = VK_FALSE;
		colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;	 // Optional
		colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
		colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;			 // Optional
		colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;	 // Optional
		colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
		colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;			 // Optional

		VkPipelineColorBlendStateCreateInfo colorBlending{};
		colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlending.logicOpEnable = VK_FALSE;
		colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
		colorBlending.attachmentCount = 1;
		colorBlending.pAttachments = &colorBlendAttachment;
		colorBlending.blendConstants[0] = 0.0f; // Optional
		colorBlending.blendConstants[1] = 0.0f; // Optional
		colorBlending.blendConstants[2] = 0.0f; // Optional
		colorBlending.blendConstants[3] = 0.0f; // Optional

		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = 0;
		pipelineLayoutInfo.pSetLayouts = nullptr;
		pipelineLayoutInfo.pushConstantRangeCount = 0;
		pipelineLayoutInfo.pPushConstantRanges = nullptr;

		auto vkCreatePipelineLayout = (PFN_vkCreatePipelineLayout)vkGetInstanceProcAddr(instance, "vkCreatePipelineLayout");
		if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create pipeline layout");
		}

		// note
		VkGraphicsPipelineCreateInfo pipelineInfo{};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineInfo.stageCount = 2;
		pipelineInfo.pStages = shaderStages;
		pipelineInfo.pVertexInputState = &vertexInputInfo;
		pipelineInfo.pInputAssemblyState = &inputAssembly;
		pipelineInfo.pViewportState = &viewportState;
		pipelineInfo.pRasterizationState = &rasterizer;
		pipelineInfo.pMultisampleState = &multisampling;
		pipelineInfo.pDepthStencilState = nullptr;
		pipelineInfo.pColorBlendState = &colorBlending;
		pipelineInfo.pDynamicState = nullptr;
		pipelineInfo.layout = pipelineLayout;
		pipelineInfo.renderPass = renderPass;
		pipelineInfo.subpass = 0;

		auto vkCreateGraphicsPipelines = (PFN_vkCreateGraphicsPipelines)vkGetInstanceProcAddr(instance, "vkCreateGraphicsPipelines");
		if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create graphics pipeline");
		}

		vkDestroyPipeline = (PFN_vkDestroyPipeline)vkGetDeviceProcAddr(device, "vkDestroyPipeline");
		vkDestroyPipelineLayout = (PFN_vkDestroyPipelineLayout)vkGetDeviceProcAddr(device, "vkDestroyPipelineLayout");
		vkDestroyShaderModule = (PFN_vkDestroyShaderModule)vkGetDeviceProcAddr(device, "vkDestroyShaderModule");
	}

	void createFramebuffers()
	{
		PFN_vkCreateFramebuffer vkCreateFramebuffer = (PFN_vkCreateFramebuffer)vkGetDeviceProcAddr(device, "vkCreateFramebuffer");
		swapChainFramebuffers.resize(swapChainImageViews.size());
		for (size_t i = 0; i < swapChainImageViews.size(); i++)
		{
			VkImageView attachments[1] = {
				swapChainImageViews[i]};

			VkFramebufferCreateInfo framebufferInfo{};
			framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferInfo.renderPass = renderPass;
			framebufferInfo.width = swapChainExtent.width;
			framebufferInfo.height = swapChainExtent.height;
			framebufferInfo.layers = 1;
			framebufferInfo.attachmentCount = 1;
			framebufferInfo.pAttachments = attachments;
			attachments[0] = swapChainImageViews[i];
			if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS)
			{
				throw std::runtime_error("failed to create framebuffer!");
			}
		}
		vkDestroyFramebuffer = (PFN_vkDestroyFramebuffer)vkGetDeviceProcAddr(device, "vkDestroyFramebuffer");
	}
	void createCommandPools()
	{
		QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice);
		PFN_vkCreateCommandPool vkCreateCommandPool = (PFN_vkCreateCommandPool)vkGetDeviceProcAddr(device, "vkCreateCommandPool");
		PFN_vkResetCommandPool vkResetCommandPool = (PFN_vkResetCommandPool)vkGetDeviceProcAddr(device, "vkResetCommandPool");
		PFN_vkAllocateCommandBuffers vkAllocateCommandBuffers = (PFN_vkAllocateCommandBuffers)vkGetDeviceProcAddr(device, "vkAllocateCommandBuffers");
		PFN_vkDestroyCommandPool vkDestroyCommandPool = (PFN_vkDestroyCommandPool)vkGetDeviceProcAddr(device, "vkDestroyCommandPool");
		{
			VkCommandPoolCreateInfo poolInfo{};
			poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
			poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();
			poolInfo.flags = 0; // Optional
			VkCommandPool vkCommandPool;
			if (vkCreateCommandPool(device, &poolInfo, nullptr, &vkCommandPool) != VK_SUCCESS)
			{
				throw std::runtime_error("failed to create command pool!");
			}
			commandPool = CommandPool(device, vkCommandPool, vkAllocateCommandBuffers, vkResetCommandPool, vkDestroyCommandPool);
		}
	}
	void createSyncObjects()
	{
		PFN_vkCreateSemaphore vkCreateSemaphore = (PFN_vkCreateSemaphore)vkGetDeviceProcAddr(device, "vkCreateSemaphore");
		PFN_vkCreateFence vkCreateFence = (PFN_vkCreateFence)vkGetDeviceProcAddr(device, "vkCreateFence");
		if (vkCreateSemaphore == nullptr || vkCreateFence == nullptr)
		{
			throw std::runtime_error("failed to create sync objects");
		}
		VkSemaphoreCreateInfo semaphoreInfo{};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		VkFenceCreateInfo fenceInfo{};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
		if (vkCreateFence(device, &fenceInfo, nullptr, &inFlightFence) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create synchronization objects for a frame!");
		}
		if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphore) != VK_SUCCESS ||
			vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphore) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create synchronization objects for a frame!");
		}
		vkDestroyFence = (PFN_vkDestroyFence)vkGetDeviceProcAddr(device, "vkDestroyFence");
		vkDestroySemaphore = (PFN_vkDestroySemaphore)vkGetDeviceProcAddr(device, "vkDestroySemaphore");
	}

	VkShaderModule createShaderModule(const std::vector<char> &code)
	{
		VkShaderModuleCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.codeSize = code.size();
		createInfo.pCode = reinterpret_cast<const uint32_t *>(code.data());

		auto vkCreateShaderModule = (PFN_vkCreateShaderModule)vkGetInstanceProcAddr(instance, "vkCreateShaderModule");
		VkShaderModule shaderModule;
		if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create shader module");
		}

		return shaderModule;
	}

	static std::vector<char> readFile(const std::string &filename)
	{
		std::ifstream file(filename, std::ios::ate | std::ios::binary);

		if (!file.is_open())
		{
			throw std::runtime_error("failed to open file");
		}

		size_t fileSize = (size_t)file.tellg();
		std::vector<char> buffer(fileSize);

		file.seekg(0);
		file.read(buffer.data(), fileSize);

		file.close();
		return buffer;
	}

	void drawFrame()
	{
		auto vkWaitForFences = (PFN_vkWaitForFences)vkGetDeviceProcAddr(device, "vkWaitForFences");
		auto vkResetFences = (PFN_vkResetFences)vkGetDeviceProcAddr(device, "vkResetFences");
		auto vkGetFenceStatus = (PFN_vkGetFenceStatus)vkGetDeviceProcAddr(device, "vkGetFenceStatus");
		auto vkAcquireNextImageKHR = (PFN_vkAcquireNextImageKHR)vkGetDeviceProcAddr(device, "vkAcquireNextImageKHR");
		auto vkQueueSubmit = (PFN_vkQueueSubmit)vkGetDeviceProcAddr(device, "vkQueueSubmit");
		auto vkQueuePresentKHR = (PFN_vkQueuePresentKHR)vkGetDeviceProcAddr(device, "vkQueuePresentKHR");
		auto vkQueueWaitIdle = (PFN_vkQueueWaitIdle)vkGetDeviceProcAddr(device, "vkQueueWaitIdle");

		if (vkGetFenceStatus(device, inFlightFence) == VK_NOT_READY)
		{
			vkWaitForFences(device, 1, &inFlightFence, VK_TRUE, std::numeric_limits<uint64_t>::max());
		}
		vkResetFences(device, 1, &inFlightFence);

		uint32_t imageIndex;
		VkResult result = vkAcquireNextImageKHR(device, swapChain, std::numeric_limits<uint64_t>::max(), imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);

		if (result != VK_SUCCESS)
		{
			throw std::runtime_error("failed to acquire swap chain image!");
		}

		commandPool.reset();
		auto commandBuffer = commandPool.acquireCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY);
		recordCommandBuffer(commandBuffer, imageIndex);

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

		VkSemaphore waitSemaphores[] = {imageAvailableSemaphore};
		VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = waitSemaphores;
		submitInfo.pWaitDstStageMask = waitStages;

		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;

		VkSemaphore signalSemaphores[] = {renderFinishedSemaphore};
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = signalSemaphores;

		vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFence);

		VkPresentInfoKHR presentInfo{};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = signalSemaphores;
		VkSwapchainKHR swapChains[] = {swapChain};
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = swapChains;
		presentInfo.pImageIndices = &imageIndex;

		result = vkQueuePresentKHR(presentQueue, &presentInfo);
		if (result != VK_SUCCESS)
		{
			throw std::runtime_error("failed to present swap chain image!");
		}
	}
	void recordCommandBuffer(VkCommandBuffer commandBuffer, std::uint32_t imageIndex)
	{
		PFN_vkBeginCommandBuffer vkBeginCommandBuffer = (PFN_vkBeginCommandBuffer)vkGetDeviceProcAddr(device, "vkBeginCommandBuffer");
		PFN_vkEndCommandBuffer vkEndCommandBuffer = (PFN_vkEndCommandBuffer)vkGetDeviceProcAddr(device, "vkEndCommandBuffer");
		PFN_vkCmdBeginRenderPass vkCmdBeginRenderPass = (PFN_vkCmdBeginRenderPass)vkGetDeviceProcAddr(device, "vkCmdBeginRenderPass");
		PFN_vkCmdEndRenderPass vkCmdEndRenderPass = (PFN_vkCmdEndRenderPass)vkGetDeviceProcAddr(device, "vkCmdEndRenderPass");
		PFN_vkCmdBindPipeline vkCmdBindPipeline = (PFN_vkCmdBindPipeline)vkGetDeviceProcAddr(device, "vkCmdBindPipeline");
		PFN_vkCmdDraw vkCmdDraw = (PFN_vkCmdDraw)vkGetDeviceProcAddr(device, "vkCmdDraw");

		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = 0;
		beginInfo.pInheritanceInfo = nullptr;

		vkBeginCommandBuffer(commandBuffer, &beginInfo);

		VkRenderPassBeginInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = renderPass;
		renderPassInfo.framebuffer = swapChainFramebuffers[imageIndex];
		renderPassInfo.renderArea.offset = {0, 0};
		renderPassInfo.renderArea.extent = swapChainExtent;

		VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
		renderPassInfo.clearValueCount = 1;
		renderPassInfo.pClearValues = &clearColor;

		vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
		vkCmdDraw(commandBuffer, 3, 1, 0, 0);
		vkCmdEndRenderPass(commandBuffer);
		vkEndCommandBuffer(commandBuffer);
	}
};

int main()
{
	HelloTriangleApplication app;

	try
	{
		app.run();
	}
	catch (const std::exception &e)
	{
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}