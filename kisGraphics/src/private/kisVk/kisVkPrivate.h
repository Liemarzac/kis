#pragma once

#include "kisGraphicsShared.h"

#include <kisCore/kisArray.h>
#include <kisCore/kisCore.h>
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
    kisMatrix4 m_modelViewProj;
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
struct kisVkImage
{
    VkImage m_image;
    VmaAllocation m_alloc;
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
    kisArray<VkDescriptorSet> m_descriptorSets;
    VkBuffer m_uniformBuffer;
    VmaAllocation m_uniformBufferAlloc;
    kisByte* m_uniformBufferMapped;
    uint32_t m_uniformBufferOffset;
    uint32_t m_uniformBufferSize;
};

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
struct kisVk
{
    kisStaticArray<kisVkFrame, k_kisVkMaxNumImages> m_frames;
    kisFixedArray<kisVkMesh, 1024> m_meshes;
    kisFixedArray<kisVkImage, 1024> m_images;
};

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
extern kisVk* g_kisVk;
extern kisVkInfo g_kisVkInfo;
extern VkAllocationCallbacks g_kisVkAllocCallbacks;

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
VkIndexType kisVkIndexType(kisIndexBufferType type);

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
size_t kisVkImageSize2D(uint32_t width, uint32_t height, VkFormat format);

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
VkFormat kisVkImageFormat(kisTextureFormat format);
