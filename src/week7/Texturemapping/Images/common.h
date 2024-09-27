#pragma once
#define GLFW_INCLUDE_VULKAN
#define VK_NO_PROTOTYPES
#define STB_IMAGE_IMPLEMENTATION
#include "config.h"
#include <GLFW/glfw3.h>
#include <stb_image.h>

//note
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#include <glm/glm.hpp>

#include <vulkan/vulkan.h>
#include <vulkan/vulkan.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <optional>
#include <set>
#include <cstdint>
#include <limits>
#include <algorithm>
#include <fstream>
#include <array>
#include <chrono>

#define CONCAT(X,Y)      CONCAT_IPML(X,Y)
#define CONCAT_IPML(X,Y) X##Y
#define VK_INSTANCE_FUNCTION_DECL(NAME) PFN_##NAME NAME = nullptr
#define VK_INSTANCE_FUNCTION_INIT(NAME) NAME = (PFN_##NAME)vkGetInstanceProcAddr(instance, #NAME)
#define VK_INSTANCE_FUNCTION(OPER,NAME) CONCAT(VK_INSTANCE_FUNCTION_,OPER)(NAME)
#define VK_DEVICE_FUNCTION_DECL(NAME) PFN_##NAME NAME = nullptr
#define VK_DEVICE_FUNCTION_INIT(NAME) NAME = (PFN_##NAME)vkGetDeviceProcAddr(device, #NAME)
#define VK_DEVICE_FUNCTION(OPER,NAME) CONCAT(VK_DEVICE_FUNCTION_,OPER)(NAME)
#define VK_INSTANCE_BEG_IF_PRE_INIT_DECL  
#define VK_INSTANCE_BEG_IF_PRE_INIT_INIT  if (!instance){
#define VK_INSTANCE_END_IF_PRE_INIT_DECL  
#define VK_INSTANCE_END_IF_PRE_INIT_INIT  }
#define VK_INSTANCE_BEG_IF_PRE_INIT(OPER) CONCAT(VK_INSTANCE_BEG_IF_PRE_INIT_,OPER)
#define VK_INSTANCE_END_IF_PRE_INIT(OPER) CONCAT(VK_INSTANCE_END_IF_PRE_INIT_,OPER)
#define VK_INSTANCE_BEG_IF_POST_INIT_DECL
#define VK_INSTANCE_BEG_IF_POST_INIT_INIT if (instance){
#define VK_INSTANCE_END_IF_POST_INIT_DECL
#define VK_INSTANCE_END_IF_POST_INIT_INIT }
#define VK_INSTANCE_BEG_IF_POST_INIT(OPER) CONCAT(VK_INSTANCE_BEG_IF_POST_INIT_,OPER)
#define VK_INSTANCE_END_IF_POST_INIT(OPER) CONCAT(VK_INSTANCE_END_IF_POST_INIT_,OPER)

#ifndef NDEBUG 
#define VK_INSTANCE_DEBUG_FUNCTIONS(OPER) \
	VK_INSTANCE_FUNCTION(OPER, vkCreateDebugUtilsMessengerEXT); \
	VK_INSTANCE_FUNCTION(OPER, vkDestroyDebugUtilsMessengerEXT)
#else
#define VK_INSTANCE_DEBUG_FUNCTIONS(OPER)
#endif

#define VK_INSTANCE_FUNCTIONS(OPER) \
	/* LOAD BEFORE INITIALIZATION **/ \
	VK_INSTANCE_BEG_IF_PRE_INIT(OPER) \
	VK_INSTANCE_FUNCTION(OPER, vkCreateInstance); \
	VK_INSTANCE_FUNCTION(OPER, vkEnumerateInstanceVersion); \
	VK_INSTANCE_FUNCTION(OPER, vkEnumerateInstanceExtensionProperties); \
	VK_INSTANCE_FUNCTION(OPER, vkEnumerateInstanceLayerProperties); \
	VK_INSTANCE_END_IF_PRE_INIT(OPER) \
	/** LOAD AFTER INITIALIZATION **/ \
	VK_INSTANCE_BEG_IF_POST_INIT(OPER) \
	VK_INSTANCE_DEBUG_FUNCTIONS(OPER); \
	VK_INSTANCE_FUNCTION(OPER, vkDestroyInstance); \
	VK_INSTANCE_FUNCTION(OPER, vkDestroySurfaceKHR); \
	VK_INSTANCE_FUNCTION(OPER, vkGetPhysicalDeviceSurfaceCapabilitiesKHR); \
	VK_INSTANCE_FUNCTION(OPER, vkGetPhysicalDeviceSurfaceFormatsKHR); \
	VK_INSTANCE_FUNCTION(OPER, vkGetPhysicalDeviceSurfacePresentModesKHR); \
	VK_INSTANCE_FUNCTION(OPER, vkGetPhysicalDeviceSurfaceSupportKHR); \
	VK_INSTANCE_FUNCTION(OPER, vkGetDeviceProcAddr); \
	VK_INSTANCE_FUNCTION(OPER, vkGetPhysicalDeviceProperties);	\
	VK_INSTANCE_FUNCTION(OPER, vkGetPhysicalDeviceFeatures);  \
	VK_INSTANCE_FUNCTION(OPER, vkEnumeratePhysicalDevices); \
	VK_INSTANCE_FUNCTION(OPER, vkCreateDevice);		 \
	VK_INSTANCE_FUNCTION(OPER, vkGetPhysicalDeviceQueueFamilyProperties); \
	VK_INSTANCE_FUNCTION(OPER, vkEnumerateDeviceExtensionProperties);	\
	VK_INSTANCE_FUNCTION(OPER, vkGetPhysicalDeviceFeatures2); \
	VK_INSTANCE_FUNCTION(OPER, vkGetPhysicalDeviceProperties2); \
	VK_INSTANCE_FUNCTION(OPER, vkGetPhysicalDeviceMemoryProperties);	\
	VK_INSTANCE_END_IF_POST_INIT(OPER)

#define VK_DEVICE_FUNCTIONS(OPER) \
	/*create*/ \
	VK_DEVICE_FUNCTION(OPER, vkCreateSwapchainKHR); \
	VK_DEVICE_FUNCTION(OPER, vkCreateImageView); \
	VK_DEVICE_FUNCTION(OPER, vkCreateRenderPass); \
	VK_DEVICE_FUNCTION(OPER, vkCreateShaderModule); \
	VK_DEVICE_FUNCTION(OPER, vkCreatePipelineLayout); \
	VK_DEVICE_FUNCTION(OPER, vkCreateGraphicsPipelines); \
	VK_DEVICE_FUNCTION(OPER, vkCreateFramebuffer); \
	VK_DEVICE_FUNCTION(OPER, vkCreateCommandPool); \
	VK_DEVICE_FUNCTION(OPER, vkAllocateCommandBuffers); \
	VK_DEVICE_FUNCTION(OPER, vkAllocateMemory); \
	VK_DEVICE_FUNCTION(OPER, vkCreateBuffer); \
	VK_DEVICE_FUNCTION(OPER, vkCreateImage); \
	VK_DEVICE_FUNCTION(OPER, vkCreateSampler); \
	VK_DEVICE_FUNCTION(OPER, vkCreateDescriptorSetLayout); \
	VK_DEVICE_FUNCTION(OPER, vkCreateDescriptorPool); \
	VK_DEVICE_FUNCTION(OPER, vkAllocateDescriptorSets); \
	VK_DEVICE_FUNCTION(OPER, vkCreateSemaphore); \
	VK_DEVICE_FUNCTION(OPER, vkCreateFence); \
	/*destroy*/ \
	VK_DEVICE_FUNCTION(OPER, vkDestroyDevice); \
	VK_DEVICE_FUNCTION(OPER, vkDestroySwapchainKHR); \
	VK_DEVICE_FUNCTION(OPER, vkDestroyRenderPass); \
	VK_DEVICE_FUNCTION(OPER, vkDestroyShaderModule); \
	VK_DEVICE_FUNCTION(OPER, vkDestroyPipelineLayout); \
	VK_DEVICE_FUNCTION(OPER, vkDestroyPipeline); \
	VK_DEVICE_FUNCTION(OPER, vkDestroyFramebuffer); \
	VK_DEVICE_FUNCTION(OPER, vkDestroyCommandPool); \
	VK_DEVICE_FUNCTION(OPER, vkFreeMemory); \
	VK_DEVICE_FUNCTION(OPER, vkDestroyBuffer); \
	VK_DEVICE_FUNCTION(OPER, vkDestroyImage); \
	VK_DEVICE_FUNCTION(OPER, vkDestroyImageView); \
	VK_DEVICE_FUNCTION(OPER, vkDestroySampler); \
	VK_DEVICE_FUNCTION(OPER, vkDestroyDescriptorSetLayout); \
	VK_DEVICE_FUNCTION(OPER, vkDestroyDescriptorPool); \
	VK_DEVICE_FUNCTION(OPER, vkFreeDescriptorSets); \
	VK_DEVICE_FUNCTION(OPER, vkDestroySemaphore); \
	VK_DEVICE_FUNCTION(OPER, vkDestroyFence); \
	/*memory*/ \
	VK_DEVICE_FUNCTION(OPER, vkMapMemory); \
	VK_DEVICE_FUNCTION(OPER, vkUnmapMemory); \
	VK_DEVICE_FUNCTION(OPER, vkBindBufferMemory); \
	VK_DEVICE_FUNCTION(OPER, vkBindImageMemory); \
	VK_DEVICE_FUNCTION(OPER, vkGetBufferMemoryRequirements); \
	VK_DEVICE_FUNCTION(OPER, vkGetImageMemoryRequirements); \
	/*command*/ \
	VK_DEVICE_FUNCTION(OPER, vkBeginCommandBuffer); \
	VK_DEVICE_FUNCTION(OPER, vkEndCommandBuffer); \
	VK_DEVICE_FUNCTION(OPER, vkResetCommandBuffer); \
	VK_DEVICE_FUNCTION(OPER, vkFreeCommandBuffers); \
	VK_DEVICE_FUNCTION(OPER, vkCmdPipelineBarrier); \
	VK_DEVICE_FUNCTION(OPER, vkCmdClearColorImage); \
	VK_DEVICE_FUNCTION(OPER, vkCmdCopyBufferToImage); \
	VK_DEVICE_FUNCTION(OPER, vkCmdCopyBuffer); \
	VK_DEVICE_FUNCTION(OPER, vkCmdCopyImageToBuffer); \
	VK_DEVICE_FUNCTION(OPER, vkCmdBeginRenderPass); \
	VK_DEVICE_FUNCTION(OPER, vkCmdEndRenderPass); \
	VK_DEVICE_FUNCTION(OPER, vkCmdBindPipeline); \
	VK_DEVICE_FUNCTION(OPER, vkCmdBindVertexBuffers); \
	VK_DEVICE_FUNCTION(OPER, vkCmdBindIndexBuffer); \
	VK_DEVICE_FUNCTION(OPER, vkCmdBindDescriptorSets); \
	VK_DEVICE_FUNCTION(OPER, vkCmdDraw); \
	VK_DEVICE_FUNCTION(OPER, vkCmdDrawIndexed); \
	VK_DEVICE_FUNCTION(OPER, vkCmdSetViewport); \
	VK_DEVICE_FUNCTION(OPER, vkCmdSetScissor); \
	/*queue operations*/ \
	VK_DEVICE_FUNCTION(OPER, vkGetDeviceQueue); \
	VK_DEVICE_FUNCTION(OPER, vkQueueSubmit); \
	VK_DEVICE_FUNCTION(OPER, vkQueuePresentKHR); \
	/*wait*/ \
	VK_DEVICE_FUNCTION(OPER, vkDeviceWaitIdle); \
	VK_DEVICE_FUNCTION(OPER, vkQueueWaitIdle); \
	/*fence*/ \
	VK_DEVICE_FUNCTION(OPER, vkWaitForFences); \
	VK_DEVICE_FUNCTION(OPER, vkResetFences); \
	/*swapchain*/ \
	VK_DEVICE_FUNCTION(OPER, vkAcquireNextImageKHR); \
	VK_DEVICE_FUNCTION(OPER, vkGetSwapchainImagesKHR); \
	/*descriptor*/ \
	VK_DEVICE_FUNCTION(OPER, vkUpdateDescriptorSets)

