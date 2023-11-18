#pragma once

#include <kisCore/kisArray.h>
#include <kisCore/kisFixedArray.h>
#include <kisCore/kisStaticArray.h>
#include <kisCore/kisMath.h>

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
const uint32_t k_kisVkMaxNumImages = 3;

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
struct kisVkInfo
{
    VkInstance m_instance;
    VkPhysicalDevice m_physicalDevice;
    VkPhysicalDeviceProperties m_physicalDeviceProperties;
    VkPhysicalDeviceMemoryProperties m_memoryProperties;
    VkDevice m_device;
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
    VkDescriptorPool m_descriptorPool;
    kisStaticArray<VkBuffer, k_kisVkMaxNumImages> m_uniformBuffers;
    kisStaticArray<VmaAllocation, k_kisVkMaxNumImages> m_uniformBufferAllocs;
    kisStaticArray<void*, k_kisVkMaxNumImages> m_uniformBufferMapped;
    uint32_t m_uniformBufferOffset;
    VmaAllocator m_vmaAllocator;
    uint32_t m_iGraphicsQueueFamily;
    uint32_t m_iPresentQueueFamily;
    uint32_t m_nImages;
    bool m_bValidate;
    kisFixedArray<const char*, 64> m_instanceExtensionNames;
    kisFixedArray<const char*, 64> m_deviceExtensionNames;
    kisFixedArray<const char*, 16> m_layerNames;
    kisFixedArray<VkImage, k_kisVkMaxNumImages> m_swapchainImages;
    kisFixedArray<VkImageView, k_kisVkMaxNumImages> m_swapchainImageViews;
    kisFixedArray<VkFramebuffer, k_kisVkMaxNumImages> m_frameBuffers;
    kisFixedArray<VkSemaphore, k_kisVkMaxNumImages> m_imageAvailSemaphore;
};

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
struct kisVkUBOObjectVertexBuffer
{
    kisMat4 m_modelViewProj;
};

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
struct kisVkMesh
{
    VkBuffer m_vertexBuffer;
    VmaAllocation m_vertexBufferAlloc;
    VkBuffer m_indexBuffer;
    VmaAllocation m_indexBufferAlloc;
    uint32_t m_nVertices;
    uint32_t m_nIndices;
    VkIndexType m_indexType;
};

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
struct kisVkDraw
{
    VkDescriptorSet m_descriptorSet;
    uint32_t m_uniformBufferOffset;
    uint32_t m_iMesh;
};

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
struct kisVkFrame
{
    kisArray<kisVkDraw> m_draws;
};

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
extern kisVkInfo g_kisVkInfo;
extern VkAllocationCallbacks g_kisVkAllocCallbacks;
extern kisStaticArray<kisVkFrame, k_kisVkMaxNumImages> g_kisVkFrames;
extern kisFixedArray<kisVkMesh, 1024> g_kisVkMeshes;

