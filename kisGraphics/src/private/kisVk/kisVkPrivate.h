#pragma once

#include <kisCore/kisArrayStatic.h>

#include <MoltenVK/mvk_vulkan.h>
#pragma clang system_header
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdocumentation"
#pragma clang diagnostic ignored "-Wnullability-completeness"
#pragma clang diagnostic ignored "-Wall"
#include <vk_mem_alloc.h>
#pragma clang diagnostic pop

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
#define KIS_VK_CHECK(X) if(X != VK_SUCCESS){KIS_ASSERT(false)}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
struct kisVkInfo
{
    VkInstance m_instance;
    VkPhysicalDevice m_physicalDevice;
    VkDevice m_device;
    VkPhysicalDeviceProperties m_deviceProperties;
    VkPhysicalDeviceMemoryProperties m_memoryProperties;
    VkSurfaceKHR m_surface;
    VkCommandPool m_cmdPool;
    VkSwapchainKHR m_swapchain;
    VkSurfaceFormatKHR m_surfaceFormat;
    VkExtent2D m_swapchainSize;
    VkCommandBuffer m_cmdBuffer;
    VkFormat m_depthFormat;
    VkImage m_depthImage;
    VmaAllocation m_depthAlloc;
    VkImageView m_depthImageView;
    VkMemoryAllocateInfo m_depthMemAllocInfo;
    VkQueue m_graphicsQueue;
    VkQueue m_presentQueue;
    VmaAllocator m_vmaAllocator;
    uint32_t m_iGraphicsQueueFamily;
    uint32_t m_iPresentQueueFamily;
    bool m_bValidate;
    kisArrayStatic<const char*, 64> m_instanceExtensionNames;
    kisArrayStatic<const char*, 64> m_deviceExtensionNames;
    kisArrayStatic<const char*, 16> m_layerNames;
    kisArrayStatic<VkImage, 3> m_swapchainImages;
    kisArrayStatic<VkImageView, 3> m_swapchainImageViews;
    kisArrayStatic<VkFramebuffer, 3> m_frameBuffers;
    kisArrayStatic<VkSemaphore, 3> m_imageAvailSemaphore;
};

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
extern kisVkInfo g_kisVkInfo;
extern VkAllocationCallbacks g_kisVkAllocCallbacks;

