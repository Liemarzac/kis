#include "kisVk.h"

#include "kisVkPrivate.h"
#include "kisVkResources.h"

#include <kisCore/kisFile.h>
#include <kisCore/kisFixedHashMap.h>
#include <kisCore/kisMath.h>
#include <kisCore/kisStringANSIStatic.h>

#include <stdlib.h>

#ifndef WIN32
#include <signal.h>
#endif

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR g_vulkanFuncPtrGetPhysicalDeviceSurfaceCapabilitiesKHR = nullptr;
PFN_vkGetPhysicalDeviceSurfaceFormatsKHR g_vulkanFuncPtrGetPhysicalDeviceSurfaceFormatsKHR = nullptr;
PFN_vkCreateSwapchainKHR g_vulkanFuncPtrCreateSwapchainKHR = nullptr;

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
#define KIS_VK_GET_PROC_ADDR(inst, entrypoint)                                                                \
    {                                                                                                         \
        g_vulkanFuncPtr##entrypoint = (PFN_vk##entrypoint)vkGetInstanceProcAddr(inst, "vk" #entrypoint);      \
        KIS_ASSERT(g_vulkanFuncPtr##entrypoint != nullptr);                                                   \
    }

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
struct kisVkRenderContext
{
    VkRenderPass m_renderPass;
    VkPipeline m_pipeline;
    VkPipelineLayout m_pipelineLayout;
    VkDescriptorSetLayout m_objectDescriptorSetLayout;
    VkSemaphore m_imageAvailableSemaphore;
    VkSemaphore m_queueExecutedSemaphore;
    VkFence m_queueExecutedFence;
};

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
struct kisVkMemBlock
{
    size_t m_size;
};

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
static const uint32_t s_kisVkMaxNumAllocs = 8192;

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
struct kisVertexFormatPos2Color3
{
    kisVec2 m_pos;
    kisVec3 m_color;
};

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
uint32_t kisVkHashPointer(void* p)
{
    KIS_ASSERT(kisIsPowerOf2(s_kisVkMaxNumAllocs));
    return ((uintptr_t)p >> 4) & (s_kisVkMaxNumAllocs - 1);
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
kisVkRenderContext g_kisVkRenderContext;
kisFixedHashMap<void*, kisVkMemBlock, s_kisVkMaxNumAllocs, kisVkHashPointer> g_vulkanMemBlocks;

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
static bool s_bBreakOnValidationCallback = true;

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
void* kisVkAlloc(
    void*                       pUserData,
    size_t                      size,
    size_t                      alignment,
    VkSystemAllocationScope     allocationScope)
{
    void* pointer = aligned_alloc(alignment, size);

    auto itr = g_vulkanMemBlocks.add(pointer);
    (*itr).m_size = size;

#if defined(KIS_DEBUG)
    if(g_vulkanMemBlocks.getNumKeys() > 100)
    {
        const float collisionRatio = g_vulkanMemBlocks.getCollisionRatio();
        KIS_PERF_ASSERT(collisionRatio < 0.1f);
    }
#endif

    return pointer;
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
void* kisVkRealloc(
    void*                       pUserData,
    void*                       pOriginal,
    size_t                      size,
    size_t                      alignment,
    VkSystemAllocationScope     allocationScope)
{
    auto itrOldPointer = g_vulkanMemBlocks.find(pOriginal);
    KIS_ASSERT(itrOldPointer.isValid());
    const size_t oldSizeSize = (*itrOldPointer).m_size;
    g_vulkanMemBlocks.remove(itrOldPointer);
    
    void* newPointer = aligned_alloc(alignment, size);
    auto itrNewPointer = g_vulkanMemBlocks.add(newPointer);
    (*itrNewPointer).m_size = size;

    size_t cpySize = oldSizeSize;
    if(oldSizeSize > size)
    {
        cpySize = size;
    }

    if(cpySize > 0)
    {
        memcpy(newPointer, pOriginal, cpySize);
    }

    free(pOriginal);

    return newPointer;
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
void kisVkFree(void* pUserData, void* pointer)
{
    if(pointer == nullptr)
    {
        return;
    }

    g_vulkanMemBlocks.remove(pointer);
    free(pointer);
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
VkBool32 kisVkDebugMessengerCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
    void* userData
)
{
    kisStringANSIStatic<1024> message;

    if((messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT) != 0)
    {
        message.concat("VERBOSE : ");
    }

    if((messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) != 0)
    {
        message.concat("INFO : ");
    }

    if((messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) != 0)
    {
        message.concat("WARNING : ");
    }

    if((messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) != 0)
    {
        message.concat("ERROR : ");
    }

    if((messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT) != 0)
    {
        message.concat("GENERAL");
    }
    else
    {
        if((messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT) != 0)
        {
            message.concat("VALIDATION");
        }

        if((messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT) != 0)
        {
            if((messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT) != 0)
            {
                message.concat("|");
            }
            message.concat("PERFORMANCE");
        }
    }

    message.concat(" - Message Id Number: %d | Message Id Name: %s\n\t%s\n", pCallbackData->messageIdNumber, pCallbackData->pMessageIdName == NULL ? "" : pCallbackData->pMessageIdName, pCallbackData->pMessage);

    kisLog("%s", message.c_str());

    if(s_bBreakOnValidationCallback)
    {
#ifndef WIN32
        raise(SIGTRAP);
#else
        DebugBreak();
#endif
    }

    return false;
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
void kisVkNameObject(VkObjectType objectType, uint64_t vkHandle, const char* format, ...)
{
//    if(!g_kisVkInfo.m_bValidate)
//    {
//        return;
//    }
    char name[1024];
    va_list argptr;
    va_start(argptr, format);
    vsnprintf(name, sizeof(name), format, argptr);
    va_end(argptr);
    name[sizeof(name) - 1] = '\0';

    VkDebugUtilsObjectNameInfoEXT objNameInfo = {
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
        .pNext = nullptr,
        .objectType = objectType,
        .objectHandle = vkHandle,
        .pObjectName = name,
    };

    KIS_VK_CHECK(vkSetDebugUtilsObjectNameEXT(g_kisVkInfo.m_device, &objNameInfo));
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
void kisVkInitInstanceLayers()
{
    kisFixedArray<VkLayerProperties, 100> propertiesArray;

    bool bValidationLayerActive = false;

    vkEnumerateInstanceLayerProperties(propertiesArray.numExternal(), propertiesArray.dataPointer());

    const char* k_validationLayerName = "VK_LAYER_KHRONOS_validation";

    kisLogTableHeader("Instance layers");
    for(const VkLayerProperties& properties : propertiesArray)
    {
        kisLogTableEntry(properties.layerName);
        if(strcmp(properties.layerName, k_validationLayerName) == 0)
        {
            if(g_kisVkInfo.m_bValidate)
            {
                g_kisVkInfo.m_layerNames.add(k_validationLayerName);
                bValidationLayerActive = true;
            }
        }
    }
    kisLogTableFooter();

    if(!bValidationLayerActive)
    {
        g_kisVkInfo.m_bValidate = false;
    }
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
void kisVkInitInstanceExtensions(bool& bPortabilityEnumerationActive)
{
    kisFixedArray<VkExtensionProperties, 100> propertiesArray;

    vkEnumerateInstanceExtensionProperties(VK_NULL_HANDLE, propertiesArray.numExternal(), propertiesArray.dataPointer());

    bPortabilityEnumerationActive = false;

    bool bSurfaceExtensionFound = false;
    bool bMetalSurfaceExtensionFound = false;

    kisLogTableHeader("Instance extensions");
    for(const VkExtensionProperties& properties : propertiesArray)
    {
        kisLogTableEntry(properties.extensionName);
        if(strcmp(properties.extensionName, VK_KHR_SURFACE_EXTENSION_NAME) == 0)
        {
            g_kisVkInfo.m_instanceExtensionNames.add(VK_KHR_SURFACE_EXTENSION_NAME);
            bSurfaceExtensionFound = true;
        }
        else if(strcmp(properties.extensionName, VK_EXT_METAL_SURFACE_EXTENSION_NAME) == 0)
        {
            g_kisVkInfo.m_instanceExtensionNames.add(VK_EXT_METAL_SURFACE_EXTENSION_NAME);
            bMetalSurfaceExtensionFound = true;
        }
        else if(strcmp(properties.extensionName, VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME) == 0)
        {
            g_kisVkInfo.m_instanceExtensionNames.add(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
        }
        else if(strcmp(properties.extensionName, VK_EXT_DEBUG_UTILS_EXTENSION_NAME) == 0)
        {
            if(g_kisVkInfo.m_bValidate)
            {
                g_kisVkInfo.m_instanceExtensionNames.add(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
            }
        }
        else if(strcmp(properties.extensionName, VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME) == 0)
        {
            g_kisVkInfo.m_instanceExtensionNames.add(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
            bPortabilityEnumerationActive = true;
        }
    }
    kisLogTableFooter();

    KIS_ASSERT(bSurfaceExtensionFound);
    KIS_ASSERT(bMetalSurfaceExtensionFound);
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
void kisVkCreateInstance(bool bPortabilityEnumerationActive)
{
    const VkApplicationInfo app = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pNext = VK_NULL_HANDLE,
        .pApplicationName = "Kis",
        .applicationVersion = 0,
        .pEngineName = "Kis",
        .engineVersion = 0,
        .apiVersion = VK_API_VERSION_1_0,
    };

    VkInstanceCreateFlags instanceCreateFlags = 0;
    if(bPortabilityEnumerationActive)
    {
        instanceCreateFlags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
    }

    VkInstanceCreateInfo instanceCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pNext = VK_NULL_HANDLE,
        .flags = instanceCreateFlags,
        .pApplicationInfo = &app,
        .enabledLayerCount = g_kisVkInfo.m_layerNames.num(),
        .ppEnabledLayerNames = g_kisVkInfo.m_layerNames.dataPointer(),
        .enabledExtensionCount = g_kisVkInfo.m_instanceExtensionNames.num(),
        .ppEnabledExtensionNames = g_kisVkInfo.m_instanceExtensionNames.dataPointer(),
    };
    
    VkDebugUtilsMessengerCreateInfoEXT dbgMessengerCreateInfo;
    if(g_kisVkInfo.m_bValidate)
    {
        // VK_EXT_debug_utils style
        dbgMessengerCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        dbgMessengerCreateInfo.pNext = VK_NULL_HANDLE;
        dbgMessengerCreateInfo.flags = 0;
        dbgMessengerCreateInfo.messageSeverity =
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        dbgMessengerCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                                                VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                                                VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        dbgMessengerCreateInfo.pfnUserCallback = kisVkDebugMessengerCallback;
        dbgMessengerCreateInfo.pUserData = &g_kisVkInfo;
        instanceCreateInfo.pNext = &dbgMessengerCreateInfo;
    }

    KIS_VK_CHECK(vkCreateInstance(&instanceCreateInfo, &g_kisVkAllocCallbacks, &g_kisVkInfo.m_instance));

    KIS_VK_GET_PROC_ADDR(g_kisVkInfo.m_instance, GetPhysicalDeviceSurfaceCapabilitiesKHR);
    KIS_VK_GET_PROC_ADDR(g_kisVkInfo.m_instance, GetPhysicalDeviceSurfaceFormatsKHR);
    KIS_VK_GET_PROC_ADDR(g_kisVkInfo.m_instance, CreateSwapchainKHR);
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
void kisVkCreateSurface(const void* metalLayer)
{
    // Create surface.
    VkMetalSurfaceCreateInfoEXT surfaceCreateInfo;
    surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_METAL_SURFACE_CREATE_INFO_EXT;
    surfaceCreateInfo.pNext = VK_NULL_HANDLE;
    surfaceCreateInfo.flags = 0;
    surfaceCreateInfo.pLayer = metalLayer;
    KIS_VK_CHECK(vkCreateMetalSurfaceEXT(g_kisVkInfo.m_instance, &surfaceCreateInfo, &g_kisVkAllocCallbacks, &g_kisVkInfo.m_surface));
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
void kisVkPickPhysicalDevice()
{
    g_kisVkInfo.m_physicalDevice = nullptr;
    g_kisVkInfo.m_iGraphicsQueueFamily = k_kisCoreInvalidIndex;
    g_kisVkInfo.m_iPresentQueueFamily = k_kisCoreInvalidIndex;

    kisFixedArray<VkPhysicalDevice, 16> physicalDevices;

    KIS_VK_CHECK(vkEnumeratePhysicalDevices(g_kisVkInfo.m_instance, physicalDevices.numExternal(), physicalDevices.dataPointer()));

    kisFixedArray<VkQueueFamilyProperties, 32> queueFamilyProperties;

    int iBestDevice = k_kisCoreInvalidIndex;
    uint32_t bestImageDimension = 0;
    bool bFoundDiscreteGPU = false;
    VkPhysicalDeviceProperties properties;

    for(uint32_t iDevice = 0; iDevice < physicalDevices.num(); ++iDevice)
    {
        VkPhysicalDevice& device = physicalDevices[iDevice];

        vkGetPhysicalDeviceProperties(device, &properties);
        vkGetPhysicalDeviceQueueFamilyProperties(device, queueFamilyProperties.numExternal(), queueFamilyProperties.dataPointer());

        // Make sure that the physical device supports both graphics and present.
        int iGraphicsQueueFamily = k_kisCoreInvalidIndex;
        int iPresentQueueFamily = k_kisCoreInvalidIndex;

        for(uint32_t iQueueFamilyProperty = 0; iQueueFamilyProperty < queueFamilyProperties.num(); ++iQueueFamilyProperty)
        {
            if(iGraphicsQueueFamily == k_kisCoreInvalidIndex)
            {
                if((queueFamilyProperties[iQueueFamilyProperty].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0)
                {
                    iGraphicsQueueFamily = iQueueFamilyProperty;
                }
            }

            if(iPresentQueueFamily == k_kisCoreInvalidIndex)
            {
                VkBool32 bSupportPresent = false;
                vkGetPhysicalDeviceSurfaceSupportKHR(device, iQueueFamilyProperty, g_kisVkInfo.m_surface, &bSupportPresent);
                if(bSupportPresent)
                {
                    iPresentQueueFamily = iQueueFamilyProperty;
                }
            }

            if(iGraphicsQueueFamily != k_kisCoreInvalidIndex && iPresentQueueFamily != k_kisCoreInvalidIndex)
            {
                // We have found both queue families we were looing for.
                break;
            }
        }

        if(iGraphicsQueueFamily == k_kisCoreInvalidIndex || iPresentQueueFamily == k_kisCoreInvalidIndex)
        {
            // The physical device does not present or graphics queues.
            continue;
        }

        g_kisVkInfo.m_iPresentQueueFamily = iPresentQueueFamily;
        g_kisVkInfo.m_iGraphicsQueueFamily = iGraphicsQueueFamily;

        if(!bFoundDiscreteGPU || properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
        {
            if(properties.limits.maxImageDimension2D > bestImageDimension)
            {
                iBestDevice = iDevice;
                bestImageDimension = properties.limits.maxImageDimension2D;
                g_kisVkInfo.m_physicalDevice = device;
            }

            if(properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
            {
                bFoundDiscreteGPU = true;
            }
        }
    }

    KIS_ASSERT(g_kisVkInfo.m_physicalDevice != nullptr);
    KIS_ASSERT(g_kisVkInfo.m_iPresentQueueFamily != k_kisCoreInvalidIndex);
    KIS_ASSERT(g_kisVkInfo.m_iPresentQueueFamily != k_kisCoreInvalidIndex);

    g_kisVkInfo.m_physicalDeviceProperties = properties;

    kisFixedArray<VkExtensionProperties, 256> deviceExtensionProperties;
    vkEnumerateDeviceExtensionProperties(g_kisVkInfo.m_physicalDevice, nullptr, deviceExtensionProperties.numExternal(), deviceExtensionProperties.dataPointer());
    bool bSwapchainFound = false;
    const char* k_portabilitySubsetExtensioName = "VK_KHR_portability_subset";
 
    kisLogTableHeader("Device properties");
    for(const VkExtensionProperties& properties : deviceExtensionProperties)
    {
        kisLogTableEntry(properties.extensionName);
        if (strcmp(VK_KHR_SWAPCHAIN_EXTENSION_NAME, properties.extensionName) == 0)
        {
            bSwapchainFound = true;
            g_kisVkInfo.m_deviceExtensionNames.add(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
        }
        else if(strcmp(k_portabilitySubsetExtensioName, properties.extensionName) == 0)
        {
            g_kisVkInfo.m_deviceExtensionNames.add(k_portabilitySubsetExtensioName);
        }
    }
    kisLogTableFooter();

    kisFixedArray<VkSurfaceFormatKHR, 64> surfaceFormats;
    g_vulkanFuncPtrGetPhysicalDeviceSurfaceFormatsKHR(g_kisVkInfo.m_physicalDevice, g_kisVkInfo.m_surface, surfaceFormats.numExternal(), surfaceFormats.dataPointer());

    for(const VkSurfaceFormatKHR& surfaceFormat : surfaceFormats)
    {
        const VkFormat format = surfaceFormat.format;

        if (format == VK_FORMAT_R8G8B8A8_UNORM || format == VK_FORMAT_B8G8R8A8_UNORM ||
            format == VK_FORMAT_A2B10G10R10_UNORM_PACK32 || format == VK_FORMAT_A2R10G10B10_UNORM_PACK32 ||
            format == VK_FORMAT_A1R5G5B5_UNORM_PACK16 || format == VK_FORMAT_R5G6B5_UNORM_PACK16 ||
            format == VK_FORMAT_R16G16B16A16_SFLOAT)
        {
            g_kisVkInfo.m_surfaceFormat = surfaceFormat;
            break;
        }
    }

    vkGetPhysicalDeviceMemoryProperties(g_kisVkInfo.m_physicalDevice, &g_kisVkInfo.m_memoryProperties);
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
void kisVkCreateDevice()
{
    float queuePriorities[1] = {0.0};
    VkDeviceQueueCreateInfo queues[2];
    queues[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queues[0].pNext = nullptr;
    queues[0].queueFamilyIndex = g_kisVkInfo.m_iGraphicsQueueFamily;
    queues[0].queueCount = 1;
    queues[0].pQueuePriorities = queuePriorities;
    queues[0].flags = 0;

    VkDeviceCreateInfo deviceCreateInfos = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = nullptr,
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = queues,
        .enabledLayerCount = 0,
        .ppEnabledLayerNames = nullptr,
        .enabledExtensionCount = g_kisVkInfo.m_deviceExtensionNames.num(),
        .ppEnabledExtensionNames = g_kisVkInfo.m_deviceExtensionNames.dataPointer(),
        .pEnabledFeatures = nullptr,  // If specific features are required, pass them in here
    };

    if(g_kisVkInfo.m_iGraphicsQueueFamily != g_kisVkInfo.m_iPresentQueueFamily)
    {
        queues[1].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queues[1].pNext = nullptr;
        queues[1].queueFamilyIndex = g_kisVkInfo.m_iPresentQueueFamily;
        queues[1].queueCount = 1;
        queues[1].pQueuePriorities = queuePriorities;
        queues[1].flags = 0;
        deviceCreateInfos.queueCreateInfoCount = 2;
    }

    KIS_VK_CHECK(vkCreateDevice(g_kisVkInfo.m_physicalDevice, &deviceCreateInfos, &g_kisVkAllocCallbacks, &g_kisVkInfo.m_device));

    vkGetDeviceQueue(g_kisVkInfo.m_device, g_kisVkInfo.m_iGraphicsQueueFamily, 0, &g_kisVkInfo.m_graphicsQueue);
    vkGetDeviceQueue(g_kisVkInfo.m_device, g_kisVkInfo.m_iPresentQueueFamily, 0, &g_kisVkInfo.m_presentQueue);

    // Initialize Vulkan Memory Allocator
    const VmaAllocatorCreateInfo vmaCreateInfo = {
        .flags = VMA_ALLOCATOR_CREATE_EXTERNALLY_SYNCHRONIZED_BIT,
        .instance = g_kisVkInfo.m_instance,
        .physicalDevice = g_kisVkInfo.m_physicalDevice,
        .device = g_kisVkInfo.m_device,
        .pAllocationCallbacks = &g_kisVkAllocCallbacks,
    };

    KIS_VK_CHECK(vmaCreateAllocator(&vmaCreateInfo, &g_kisVkInfo.m_vmaAllocator));

    // Create command pool.
    const VkCommandPoolCreateInfo cmdPoolCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .pNext = nullptr,
        .queueFamilyIndex = g_kisVkInfo.m_iGraphicsQueueFamily,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
    };

    KIS_VK_CHECK(vkCreateCommandPool(g_kisVkInfo.m_device, &cmdPoolCreateInfo, &g_kisVkAllocCallbacks, &g_kisVkInfo.m_cmdPool));

    const VkCommandBufferAllocateInfo cmdBufferAllocInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .pNext = nullptr,
        .commandPool = g_kisVkInfo.m_cmdPool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1,
    };

    KIS_VK_CHECK(vkAllocateCommandBuffers(g_kisVkInfo.m_device, &cmdBufferAllocInfo, &g_kisVkInfo.m_cmdBuffer));

    // Create desciptor pool.
    const VkDescriptorPoolSize uniformBuffersDescriptorPoolSize = {
        .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount = k_kisVkMaxNumImages,
    };

    const VkDescriptorPoolCreateInfo descriptorPoolCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .poolSizeCount = 1,
        .pPoolSizes = &uniformBuffersDescriptorPoolSize,
        .maxSets = k_kisVkMaxNumImages,
    };

    KIS_VK_CHECK(vkCreateDescriptorPool(g_kisVkInfo.m_device, &descriptorPoolCreateInfo, &g_kisVkAllocCallbacks, &g_kisVkInfo.m_descriptorPool));

    const VkSemaphoreCreateInfo semaphoreCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
    };

    vkCreateSemaphore(g_kisVkInfo.m_device, &semaphoreCreateInfo, &g_kisVkAllocCallbacks, &g_kisVkRenderContext.m_imageAvailableSemaphore);

    vkCreateSemaphore(g_kisVkInfo.m_device, &semaphoreCreateInfo, &g_kisVkAllocCallbacks, &g_kisVkRenderContext.m_queueExecutedSemaphore);

    const VkFenceCreateInfo fenceCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT,
    };

    vkCreateFence(g_kisVkInfo.m_device, &fenceCreateInfo, &g_kisVkAllocCallbacks, &g_kisVkRenderContext.m_queueExecutedFence);

    // Create render pass.
    const VkAttachmentDescription colorAttachmentDesc = {
        .format = g_kisVkInfo.m_surfaceFormat.format,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
    };

    const VkAttachmentReference colorAttachmentRef = {
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    };

    const VkSubpassDescription subpassDesc = {
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount = 1,
        .pColorAttachments = &colorAttachmentRef
    };

    const VkSubpassDependency dependency = {
        .srcSubpass = VK_SUBPASS_EXTERNAL,
        .dstSubpass = 0,
        .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .srcAccessMask = 0,
        .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
    };

    const VkRenderPassCreateInfo renderPassCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .pNext = nullptr,
        .attachmentCount = 1,
        .pAttachments = &colorAttachmentDesc,
        .subpassCount = 1,
        .pSubpasses = &subpassDesc,
        .dependencyCount = 1,
        .pDependencies = &dependency,
    };

    KIS_VK_CHECK(vkCreateRenderPass(g_kisVkInfo.m_device, &renderPassCreateInfo, &g_kisVkAllocCallbacks, &g_kisVkRenderContext.m_renderPass));
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
VkShaderModule kisVkCreateShader(const kisFileBuffer& spirVCode)
{
    VkShaderModuleCreateInfo createInfo;
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = spirVCode.m_size;
    createInfo.pCode = (uint32_t*)spirVCode.m_data;
    VkShaderModule shader;
    KIS_VK_CHECK(vkCreateShaderModule(g_kisVkInfo.m_device, &createInfo, &g_kisVkAllocCallbacks, &shader));
    return shader;
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
void kisVKLoadAssets()
{

    const uint32_t uniformBufferSize = kisMin(g_kisVkInfo.m_physicalDeviceProperties.limits.maxUniformBufferRange, 64U * 1024U);
    VkBufferCreateInfo uniformBufferCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .flags = 0,
        .size = uniformBufferSize,
        .usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE
    };

    VmaAllocationCreateInfo uniformBufferAllocationInfo = {
        .flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
        .usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE
    };

    for(uint32_t i = 0 ; i < g_kisVkInfo.m_nImages; ++ i)
    {
        VkBuffer buffer;
        VmaAllocation alloc;
        VmaAllocationInfo allocInfo;
        vmaCreateBuffer(g_kisVkInfo.m_vmaAllocator, &uniformBufferCreateInfo, &uniformBufferAllocationInfo, &buffer, &alloc, &allocInfo);
        KIS_ASSERT(allocInfo.pMappedData != nullptr);
        g_kisVkFrames[i].m_uniformBuffer = buffer;
        g_kisVkFrames[i].m_uniformBufferAlloc = alloc;
        g_kisVkFrames[i].m_uniformBufferMapped = (kisByte*)allocInfo.pMappedData;
        g_kisVkFrames[i].m_uniformBufferOffset = 0;
        g_kisVkFrames[i].m_uniformBufferSize = uniformBufferSize;
    }

    kisFileBuffer vsBuffer = kisFileBufferCreate("data/triangle/triangle_vert.spv");
    VkShaderModule vsShader = kisVkCreateShader(vsBuffer);
    kisFileBufferDestroy(vsBuffer);

    kisFileBuffer psBuffer = kisFileBufferCreate("data/triangle/triangle_frag.spv");
    VkShaderModule psShader = kisVkCreateShader(psBuffer);
    kisFileBufferDestroy(psBuffer);

    const VkVertexInputBindingDescription vertexInputBindingDescription = {
        .binding = 0,
        .stride = sizeof(kisVertex_XYZ_UV_Color_Normal_Tangent_Bitangent),
        .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
    };

    const VkVertexInputAttributeDescription vertexInputAttributeDescription[] = {
        {
            .binding = 0,
            .location = 0,
            .format = VK_FORMAT_R32G32B32_SFLOAT,
            .offset = offsetof(kisVertex_XYZ_UV_Color_Normal_Tangent_Bitangent, m_position)
        },
        {
           .binding = 0,
           .location = 1,
           .format = VK_FORMAT_R32G32_SFLOAT,
           .offset = offsetof(kisVertex_XYZ_UV_Color_Normal_Tangent_Bitangent, m_uv)
        },
        {
           .binding = 0,
           .location = 2,
           .format = VK_FORMAT_R32G32B32_SFLOAT,
           .offset = offsetof(kisVertex_XYZ_UV_Color_Normal_Tangent_Bitangent, m_color)
        },
        {
           .binding = 0,
           .location = 3,
           .format = VK_FORMAT_R32G32B32_SFLOAT,
           .offset = offsetof(kisVertex_XYZ_UV_Color_Normal_Tangent_Bitangent, m_normal)
        },
        {
           .binding = 0,
           .location = 4,
           .format = VK_FORMAT_R32G32B32_SFLOAT,
           .offset = offsetof(kisVertex_XYZ_UV_Color_Normal_Tangent_Bitangent, m_tangent)
        },
        {
           .binding = 0,
           .location = 5,
           .format = VK_FORMAT_R32G32B32_SFLOAT,
           .offset = offsetof(kisVertex_XYZ_UV_Color_Normal_Tangent_Bitangent, m_bitangent)
        }
    };

    const VkPipelineShaderStageCreateInfo shaderStages[] = {
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .pNext = nullptr,
            .stage = VK_SHADER_STAGE_VERTEX_BIT,
            .module = vsShader,
            .pName = "main",
        },
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .pNext = nullptr,
            .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
            .module = psShader,
            .pName = "main",
        }
    };

    kisStaticArray<VkDynamicState, 2> dynamicStates;
    dynamicStates[0] = VK_DYNAMIC_STATE_VIEWPORT;
    dynamicStates[1] = VK_DYNAMIC_STATE_SCISSOR;

    const VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .pNext = nullptr,
        .dynamicStateCount = dynamicStates.num(),
        .pDynamicStates = dynamicStates.dataPointer(),
    };

    const VkPipelineVertexInputStateCreateInfo vertexInfoStateCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .pNext = nullptr,
        .vertexBindingDescriptionCount = 1,
        .pVertexBindingDescriptions = &vertexInputBindingDescription,
        .vertexAttributeDescriptionCount = KIS_ARRAY_COUNT(vertexInputAttributeDescription),
        .pVertexAttributeDescriptions = vertexInputAttributeDescription,
    };

    const VkPipelineInputAssemblyStateCreateInfo inputAssemblyCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .primitiveRestartEnable = VK_FALSE,
    };

    const VkPipelineViewportStateCreateInfo viewportStateCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .pNext = nullptr,
        .viewportCount = 1,
        .scissorCount = 1,
    };

    const VkPipelineRasterizationStateCreateInfo rasterizationStateCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .pNext = nullptr,
        .depthClampEnable = VK_FALSE,
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .lineWidth = 1.0f,
        .cullMode = VK_CULL_MODE_BACK_BIT,
        .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
        .depthBiasEnable = VK_FALSE,
        .depthBiasConstantFactor = 0.0f,
        .depthBiasClamp = 0.0f,
        .depthBiasSlopeFactor = 0.0f
    };

    const VkPipelineMultisampleStateCreateInfo multisamplingStateCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .pNext = nullptr,
        .sampleShadingEnable = VK_FALSE,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
        .minSampleShading = 1.0f,
        .pSampleMask = nullptr,
        .alphaToCoverageEnable = VK_FALSE,
        .alphaToOneEnable = VK_FALSE,
    };

    const VkPipelineColorBlendAttachmentState colorBlendAttachmentState = {
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
        .blendEnable = VK_FALSE,
        .srcColorBlendFactor = VK_BLEND_FACTOR_ONE,
        .dstColorBlendFactor = VK_BLEND_FACTOR_ZERO,
        .colorBlendOp = VK_BLEND_OP_ADD,
        .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
        .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
        .alphaBlendOp = VK_BLEND_OP_ADD
    };

    const VkPipelineColorBlendStateCreateInfo colorBlendStateCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .pNext = nullptr,
        .logicOpEnable = VK_FALSE,
        .logicOp = VK_LOGIC_OP_COPY,
        .attachmentCount = 1,
        .pAttachments = &colorBlendAttachmentState,
        .blendConstants[0] = 0.0f,
        .blendConstants[1] = 0.0f,
        .blendConstants[2] = 0.0f,
        .blendConstants[3] = 0.0f,
    };

    const VkDescriptorSetLayoutBinding objectDescriptorSetLayoutBindings[] = {
        {
            .binding = 0,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
            .pImmutableSamplers = nullptr
        },
    };

    const VkDescriptorSetLayoutCreateInfo objectDescriptorSetLayoutCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = KIS_ARRAY_COUNT(objectDescriptorSetLayoutBindings),
        .pBindings = objectDescriptorSetLayoutBindings,
    };

    KIS_VK_CHECK(vkCreateDescriptorSetLayout(g_kisVkInfo.m_device, &objectDescriptorSetLayoutCreateInfo, &g_kisVkAllocCallbacks, &g_kisVkRenderContext.m_objectDescriptorSetLayout));

    const VkDescriptorSetLayout pipelineDescriptorSetLayouts[] = {
        g_kisVkRenderContext.m_objectDescriptorSetLayout,
    };

    const VkPipelineLayoutCreateInfo layoutCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .pNext = nullptr,
        .setLayoutCount = KIS_ARRAY_COUNT(pipelineDescriptorSetLayouts),
        .pSetLayouts = pipelineDescriptorSetLayouts,
        .pushConstantRangeCount = 0,
        .pPushConstantRanges = nullptr,
    };

    KIS_VK_CHECK(vkCreatePipelineLayout(g_kisVkInfo.m_device, &layoutCreateInfo, &g_kisVkAllocCallbacks, &g_kisVkRenderContext.m_pipelineLayout));

    VkGraphicsPipelineCreateInfo pipelineCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext = nullptr,
        .stageCount = 2,
        .pStages = shaderStages,
        .pVertexInputState = &vertexInfoStateCreateInfo,
        .pInputAssemblyState = &inputAssemblyCreateInfo,
        .pViewportState = &viewportStateCreateInfo,
        .pRasterizationState = &rasterizationStateCreateInfo,
        .pMultisampleState = &multisamplingStateCreateInfo,
        .pDepthStencilState = nullptr,
        .pColorBlendState = &colorBlendStateCreateInfo,
        .pDynamicState = &dynamicStateCreateInfo,
        .layout = g_kisVkRenderContext.m_pipelineLayout,
        .renderPass = g_kisVkRenderContext.m_renderPass,
        .subpass = 0,
        .basePipelineHandle = VK_NULL_HANDLE,
        .basePipelineIndex = -1,
    };

    KIS_VK_CHECK(vkCreateGraphicsPipelines(g_kisVkInfo.m_device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, &g_kisVkAllocCallbacks, &g_kisVkRenderContext.m_pipeline));
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
void kisVkPrepareResolutionSwapchain(uint32_t width, uint32_t height)
{
    // Surface capabilities.
    VkSurfaceCapabilitiesKHR surfaceCapabilities;
    KIS_VK_CHECK(g_vulkanFuncPtrGetPhysicalDeviceSurfaceCapabilitiesKHR(g_kisVkInfo.m_physicalDevice, g_kisVkInfo.m_surface, &surfaceCapabilities));

    g_kisVkInfo.m_swapchainSize.width = width;
    if(g_kisVkInfo.m_swapchainSize.width == 0 || g_kisVkInfo.m_swapchainSize.width > surfaceCapabilities.currentExtent.width)
    {
        g_kisVkInfo.m_swapchainSize.width = surfaceCapabilities.currentExtent.width;
    }

    g_kisVkInfo.m_swapchainSize.height = height;
    if(g_kisVkInfo.m_swapchainSize.height == 0 || g_kisVkInfo.m_swapchainSize.height > surfaceCapabilities.currentExtent.height)
    {
        g_kisVkInfo.m_swapchainSize.height = surfaceCapabilities.currentExtent.height;
    }

    VkPresentModeKHR swapchainPresentMode = VK_PRESENT_MODE_FIFO_KHR;

    uint32_t nSwapchainImages = g_kisVkInfo.m_swapchainImageViews.capacity();
    if(surfaceCapabilities.maxImageCount > 0 && nSwapchainImages > surfaceCapabilities.maxImageCount)
    {
        nSwapchainImages = surfaceCapabilities.maxImageCount;
    }

    KIS_ASSERT(nSwapchainImages >= surfaceCapabilities.minImageCount);

    VkSurfaceTransformFlagBitsKHR preTransform;
    if (surfaceCapabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR)
    {
        preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    }
    else
    {
        preTransform = surfaceCapabilities.currentTransform;
    }

    // Find a supported composite alpha mode - one of these is guaranteed to be set
    VkCompositeAlphaFlagBitsKHR compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    const uint32_t k_nCompositeAlphaFlags = 4;
    VkCompositeAlphaFlagBitsKHR compositeAlphaFlags[k_nCompositeAlphaFlags] = {
        VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
        VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR,
        VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR,
    };

    for (uint32_t i = 0; i < k_nCompositeAlphaFlags; i++)
    {
        if ((surfaceCapabilities.supportedCompositeAlpha & compositeAlphaFlags[i]) != 0)
        {
            compositeAlpha = compositeAlphaFlags[i];
            break;
        }
    }

    // Create swapchain.
    VkSwapchainKHR oldSwapchain = g_kisVkInfo.m_swapchain;

    VkSwapchainCreateInfoKHR swapchainCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .pNext = nullptr,
        .surface = g_kisVkInfo.m_surface,
        .minImageCount = nSwapchainImages,
        .imageFormat = g_kisVkInfo.m_surfaceFormat.format,
        .imageColorSpace = g_kisVkInfo.m_surfaceFormat.colorSpace,
        .imageExtent = g_kisVkInfo.m_swapchainSize,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .preTransform = preTransform,
        .compositeAlpha = compositeAlpha,
        .imageArrayLayers = 1,
        .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices = NULL,
        .presentMode = swapchainPresentMode,
        .oldSwapchain = oldSwapchain,
        .clipped = true,
    };

    KIS_VK_CHECK(vkCreateSwapchainKHR(g_kisVkInfo.m_device, &swapchainCreateInfo, &g_kisVkAllocCallbacks, &g_kisVkInfo.m_swapchain));

    g_kisVkInfo.m_nImages = nSwapchainImages;

    // Destroy old image views.
    for(int i = 0 ; i < g_kisVkInfo.m_swapchainImageViews.num(); ++i)
    {
        vkDestroyImageView(g_kisVkInfo.m_device, g_kisVkInfo.m_swapchainImageViews[i], &g_kisVkAllocCallbacks);
    }
    g_kisVkInfo.m_swapchainImageViews.empty();
    
    // Destroy old swapchain.
    if(oldSwapchain != VK_NULL_HANDLE)
    {
        vkDestroySwapchainKHR(g_kisVkInfo.m_device, oldSwapchain, &g_kisVkAllocCallbacks);
    }

    // Get swapchain images.
    KIS_VK_CHECK(vkGetSwapchainImagesKHR(g_kisVkInfo.m_device, g_kisVkInfo.m_swapchain, g_kisVkInfo.m_swapchainImages.numExternal(), g_kisVkInfo.m_swapchainImages.dataPointer()));

    // Create image views.
    for(int i = 0 ; i < nSwapchainImages; ++i)
    {
        VkImageViewCreateInfo imageViewCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .pNext = nullptr,
            .format = g_kisVkInfo.m_surfaceFormat.format,
            .components =
                {
                    .r = VK_COMPONENT_SWIZZLE_IDENTITY,
                    .g = VK_COMPONENT_SWIZZLE_IDENTITY,
                    .b = VK_COMPONENT_SWIZZLE_IDENTITY,
                    .a = VK_COMPONENT_SWIZZLE_IDENTITY,
                },
            .subresourceRange =
                {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .baseMipLevel = 0, .levelCount = 1, .baseArrayLayer = 0, .layerCount = 1},
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .flags = 0,
            .image = g_kisVkInfo.m_swapchainImages[i]
        };

        VkImageView imageView = VK_NULL_HANDLE;
        KIS_VK_CHECK(vkCreateImageView(g_kisVkInfo.m_device, &imageViewCreateInfo, &g_kisVkAllocCallbacks, &imageView));
        g_kisVkInfo.m_swapchainImageViews.add(imageView);
    }
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
void kisVkPrepareResolutionDepth()
{
    // Destroy old depth.
    if(g_kisVkInfo.m_depthImage != VK_NULL_HANDLE)
    {
        vkDestroyImageView(g_kisVkInfo.m_device, g_kisVkInfo.m_depthImageView, &g_kisVkAllocCallbacks);
        vmaDestroyImage(g_kisVkInfo.m_vmaAllocator, g_kisVkInfo.m_depthImage, g_kisVkInfo.m_depthAlloc);
    }

    const VkFormat depthFormat = VK_FORMAT_D16_UNORM;
    const VkImageCreateInfo imageCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .pNext = NULL,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = depthFormat,
        .extent = {g_kisVkInfo.m_swapchainSize.width, g_kisVkInfo.m_swapchainSize.height, 1},
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        .flags = 0,
    };

    g_kisVkInfo.m_depthFormat = depthFormat;

    const VmaAllocationCreateInfo allocCreateInfo = {
        .usage = VMA_MEMORY_USAGE_GPU_ONLY,
        .flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
    };

    vmaCreateImage(g_kisVkInfo.m_vmaAllocator, &imageCreateInfo, &allocCreateInfo, &g_kisVkInfo.m_depthImage, &g_kisVkInfo.m_depthAlloc, nullptr);

    kisVkNameObject(VK_OBJECT_TYPE_IMAGE, (uint64_t)g_kisVkInfo.m_depthImage, "depth_image");

    VkImageViewCreateInfo imageViewCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .pNext = nullptr,
        .image = g_kisVkInfo.m_depthImage,
        .format = g_kisVkInfo.m_depthFormat,
        .subresourceRange =
            {.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT, .baseMipLevel = 0, .levelCount = 1, .baseArrayLayer = 0, .layerCount = 1},
        .flags = 0,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
    };

    KIS_VK_CHECK(vkCreateImageView(g_kisVkInfo.m_device, &imageViewCreateInfo, &g_kisVkAllocCallbacks, &g_kisVkInfo.m_depthImageView));
    kisVkNameObject(VK_OBJECT_TYPE_IMAGE_VIEW, (uint64_t)g_kisVkInfo.m_depthImageView, "depth_view");
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
void kisVkCreateResolutionFramebuffer()
{
    for(uint32_t iSwapchainImageView = 0; iSwapchainImageView < g_kisVkInfo.m_swapchainImageViews.num(); iSwapchainImageView++)
    {
        const VkFramebufferCreateInfo frameBufferCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .renderPass = g_kisVkRenderContext.m_renderPass,
            .attachmentCount = 1,
            .pAttachments = &g_kisVkInfo.m_swapchainImageViews[iSwapchainImageView],
            .width = g_kisVkInfo.m_swapchainSize.width,
            .height = g_kisVkInfo.m_swapchainSize.height,
            .layers = 1,
        };

        VkFramebuffer frameBuffer;
        KIS_VK_CHECK(vkCreateFramebuffer(g_kisVkInfo.m_device, &frameBufferCreateInfo, &g_kisVkAllocCallbacks, &frameBuffer));
        g_kisVkInfo.m_frameBuffers.add(frameBuffer);
    }
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
void kisVkDestroyFramebuffers()
{
    for(VkFramebuffer frameBuffer : g_kisVkInfo.m_frameBuffers)
    {
        vkDestroyFramebuffer(g_kisVkInfo.m_device, frameBuffer, &g_kisVkAllocCallbacks);
    }

    g_kisVkInfo.m_frameBuffers.empty();
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
void kisVkPrepareResolution(uint32_t width, uint32_t height)
{
    kisVkDestroyFramebuffers();

    kisVkPrepareResolutionSwapchain(width, height);
    kisVkPrepareResolutionDepth();

    kisVkCreateResolutionFramebuffer();
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
void kisVkInit(const void* metalLayer)
{
    kisLog("Initializing Vulkan");

    memset(&g_kisVkInfo, 0, sizeof(g_kisVkInfo));
    g_kisVkInfo.m_bValidate = true;

    memset(&g_kisVkAllocCallbacks, 0, sizeof(g_kisVkAllocCallbacks));
    g_kisVkAllocCallbacks.pUserData = (void*)&g_kisVkInfo;
    g_kisVkAllocCallbacks.pfnAllocation = kisVkAlloc;
    g_kisVkAllocCallbacks.pfnReallocation = kisVkRealloc;
    g_kisVkAllocCallbacks.pfnFree = kisVkFree;

    kisVkInitInstanceLayers();

    bool bPortabilityEnumerationActive = false;
    kisVkInitInstanceExtensions(bPortabilityEnumerationActive);
    kisVkCreateInstance(bPortabilityEnumerationActive);
    kisVkCreateSurface(metalLayer);
    kisVkPickPhysicalDevice();
    kisVkCreateDevice();
    kisVkPrepareResolution(0, 0);
    kisVKLoadAssets();
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
void kisVkShutdown()
{
    for(uint32_t i = 0; i < g_kisVkInfo.m_nImages; i++)
    {
        vkDestroyBuffer(g_kisVkInfo.m_device, g_kisVkFrames[i].m_uniformBuffer, &g_kisVkAllocCallbacks);
        vmaFreeMemory(g_kisVkInfo.m_vmaAllocator, g_kisVkFrames[i].m_uniformBufferAlloc);
    }

    for(VkSemaphore semaphore : g_kisVkInfo.m_imageAvailSemaphore)
    {
        vkDestroySemaphore(g_kisVkInfo.m_device, semaphore, &g_kisVkAllocCallbacks);
    }

    if(g_kisVkInfo.m_descriptorPool != VK_NULL_HANDLE)
    {
        vkDestroyDescriptorPool(g_kisVkInfo.m_device, g_kisVkInfo.m_descriptorPool, &g_kisVkAllocCallbacks);
    }

    if(g_kisVkInfo.m_cmdPool != VK_NULL_HANDLE)
    {
        vkDestroyCommandPool(g_kisVkInfo.m_device, g_kisVkInfo.m_cmdPool, &g_kisVkAllocCallbacks);
    }

    for(VkFramebuffer framebuffer : g_kisVkInfo.m_frameBuffers)
    {
        vkDestroyFramebuffer(g_kisVkInfo.m_device, framebuffer, &g_kisVkAllocCallbacks);
    }

    if(g_kisVkInfo.m_depthImageView != VK_NULL_HANDLE)
    {
        vkDestroyImageView(g_kisVkInfo.m_device, g_kisVkInfo.m_depthImageView, &g_kisVkAllocCallbacks);
    }

    if(g_kisVkInfo.m_depthImage != VK_NULL_HANDLE)
    {
        vkDestroyImage(g_kisVkInfo.m_device, g_kisVkInfo.m_depthImage, &g_kisVkAllocCallbacks);
    }

    if(g_kisVkInfo.m_depthAlloc)
    {
        vmaFreeMemory(g_kisVkInfo.m_vmaAllocator, g_kisVkInfo.m_depthAlloc);
    }

    for(VkImageView imageView : g_kisVkInfo.m_swapchainImageViews)
    {
        vkDestroyImageView(g_kisVkInfo.m_device, imageView, &g_kisVkAllocCallbacks);
    }

    if(g_kisVkInfo.m_swapchain != VK_NULL_HANDLE)
    {
        vkDestroySwapchainKHR(g_kisVkInfo.m_device, g_kisVkInfo.m_swapchain, &g_kisVkAllocCallbacks);
    }

    if(g_kisVkInfo.m_vmaAllocator)
    {
        vmaDestroyAllocator(g_kisVkInfo.m_vmaAllocator);
    }

    if(g_kisVkInfo.m_device != VK_NULL_HANDLE)
    {
        vkDestroyDevice(g_kisVkInfo.m_device, &g_kisVkAllocCallbacks);
    }

    if(g_kisVkInfo.m_surface != VK_NULL_HANDLE)
    {
        vkDestroySurfaceKHR(g_kisVkInfo.m_instance, g_kisVkInfo.m_surface, &g_kisVkAllocCallbacks);
    }

    if(g_kisVkInfo.m_instance != VK_NULL_HANDLE)
    {
        vkDestroyInstance(g_kisVkInfo.m_instance, nullptr);
    }
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
void kisVkRender(const kisRenderParams& renderParams)
{
    const VkCommandBufferBeginInfo cmdBufferBeginInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext = nullptr,
        .flags = 0,
        .pInheritanceInfo = nullptr,
    };

    const VkClearValue clearColor = {
        .color = {0.0f, 0.0f, 0.0f, 1.0f},
    };

    vkQueueWaitIdle(g_kisVkInfo.m_graphicsQueue);
    vkQueueWaitIdle(g_kisVkInfo.m_presentQueue);

    vkResetCommandBuffer(g_kisVkInfo.m_cmdBuffer, 0);

    uint32_t iImage;
    vkAcquireNextImageKHR(g_kisVkInfo.m_device, g_kisVkInfo.m_swapchain, UINT64_MAX, g_kisVkRenderContext.m_imageAvailableSemaphore, VK_NULL_HANDLE, &iImage);

    kisVkFrame& frame = g_kisVkFrames[iImage];

    //
    // Cleanup previous use.

    frame.m_draws.empty();

    vkFreeDescriptorSets(g_kisVkInfo.m_device, g_kisVkInfo.m_descriptorPool, frame.m_descriptorSets.num(), frame.m_descriptorSets.dataPointer());
    frame.m_descriptorSets.empty();

    frame.m_uniformBufferOffset = 0;

    // Setup view and projection matrix.
    kisMatrix4 view = glm::lookAt(glm::vec3(0.0f, 1.0f, 1.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    kisMatrix4 projection = glm::perspective(kisDegToRad(45.0f), (float)g_kisVkInfo.m_swapchainSize.width / (float)g_kisVkInfo.m_swapchainSize.height, 0.1f, 10.0f);
    projection[1][1] = -projection[1][1];

    
    //
    // Create draw calls.

    for(uint32_t i = 0; i < renderParams.m_nInstances; i++)
    {
        VkDescriptorSet descriptorSet;

        // Create descriptor sets.
        const VkDescriptorSetAllocateInfo descriptorSetAllocInfo = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
            .descriptorPool = g_kisVkInfo.m_descriptorPool,
            .descriptorSetCount = 1,
            .pSetLayouts = &g_kisVkRenderContext.m_objectDescriptorSetLayout,
        };

        vkAllocateDescriptorSets(g_kisVkInfo.m_device, &descriptorSetAllocInfo, &descriptorSet);
        frame.m_descriptorSets.add(descriptorSet);

        const VkDescriptorBufferInfo descriptorBufferInfo = {
            .buffer = frame.m_uniformBuffer,
            .offset = frame.m_uniformBufferOffset,
            .range = sizeof(kisVkUBOObjectVertexBuffer)
        };

        const VkWriteDescriptorSet writeDescriptorSet = {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = descriptorSet,
            .dstBinding = 0,
            .dstArrayElement = 0,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = 1,
            .pBufferInfo = &descriptorBufferInfo,
            .pImageInfo = nullptr,
            .pTexelBufferView = nullptr,
        };

        vkUpdateDescriptorSets(g_kisVkInfo.m_device, 1, &writeDescriptorSet, 0, nullptr);

        // Update uniform buffer.
        kisByte* writePtr = frame.m_uniformBufferMapped + frame.m_uniformBufferOffset;
        kisVkUBOObjectVertexBuffer* ubo = (kisVkUBOObjectVertexBuffer*)writePtr;

        kisMatrix4 instanceTransform = renderParams.m_instances[i].m_orientation;
        ubo->m_modelViewProj = projection * view * instanceTransform;

        frame.m_uniformBufferOffset += sizeof(kisVkUBOObjectVertexBuffer);
        frame.m_uniformBufferOffset = kisAlignPowerOf2(g_kisVkFrames[iImage].m_uniformBufferOffset, 16);

        kisVkDraw& draw = frame.m_draws.add();
        draw.m_descriptorSet = descriptorSet;
        draw.m_iMesh = renderParams.m_instances[i].m_iMesh;
    }

    KIS_VK_CHECK(vkBeginCommandBuffer(g_kisVkInfo.m_cmdBuffer, &cmdBufferBeginInfo));

    const VkRenderPassBeginInfo renderPassBeginInfo = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .pNext = nullptr,
        .renderPass = g_kisVkRenderContext.m_renderPass,
        .framebuffer = g_kisVkInfo.m_frameBuffers[iImage],
        .renderArea.offset = {0, 0},
        .renderArea.extent = g_kisVkInfo.m_swapchainSize,
        .clearValueCount = 1,
        .pClearValues = &clearColor,
    };

    vkCmdBeginRenderPass(g_kisVkInfo.m_cmdBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(g_kisVkInfo.m_cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, g_kisVkRenderContext.m_pipeline);

    const VkViewport viewport = {
        .x = 0.0f,
        .y = 0.0f,
        .width = (float)g_kisVkInfo.m_swapchainSize.width,
        .height = (float)g_kisVkInfo.m_swapchainSize.height,
        .minDepth = 0.0f,
        .maxDepth = 1.0f,
    };
    vkCmdSetViewport(g_kisVkInfo.m_cmdBuffer, 0, 1, &viewport);

    const VkRect2D scissor = {
        .offset = {0, 0},
        .extent = g_kisVkInfo.m_swapchainSize,
    };
    vkCmdSetScissor(g_kisVkInfo.m_cmdBuffer, 0, 1, &scissor);

    for(const kisVkDraw& draw : g_kisVkFrames[iImage].m_draws)
    {
        vkCmdBindDescriptorSets(g_kisVkInfo.m_cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, g_kisVkRenderContext.m_pipelineLayout, 0, 1, &draw.m_descriptorSet, 0, nullptr);

        const kisVkMesh& mesh = g_kisVkMeshes[draw.m_iMesh];

        VkBuffer vertexBuffers[] = {mesh.m_vertexBuffer};
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(g_kisVkInfo.m_cmdBuffer, 0, 1, vertexBuffers, offsets);

        if(mesh.m_indexBuffer != VK_NULL_HANDLE)
        {
            vkCmdBindIndexBuffer(g_kisVkInfo.m_cmdBuffer, mesh.m_indexBuffer, 0, mesh.m_indexType);
            vkCmdDrawIndexed(g_kisVkInfo.m_cmdBuffer, mesh.m_nIndices, 1, 0, 0, 0);
        }
        else
        {
            vkCmdDraw(g_kisVkInfo.m_cmdBuffer, mesh.m_nVertices, 1, 0, 0);
        }
    }

    vkCmdEndRenderPass(g_kisVkInfo.m_cmdBuffer);

    KIS_VK_CHECK(vkEndCommandBuffer(g_kisVkInfo.m_cmdBuffer));

    const VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

    const VkSubmitInfo submitInfo = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &g_kisVkRenderContext.m_imageAvailableSemaphore,
        .pWaitDstStageMask = waitStages,
        .commandBufferCount = 1,
        .pCommandBuffers = &g_kisVkInfo.m_cmdBuffer,
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &g_kisVkRenderContext.m_queueExecutedSemaphore,
    };

    vkQueueSubmit(g_kisVkInfo.m_graphicsQueue, 1, &submitInfo, g_kisVkRenderContext.m_queueExecutedFence);

    const VkPresentInfoKHR presentInfo = {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &g_kisVkRenderContext.m_queueExecutedSemaphore,
        .swapchainCount = 1,
        .pSwapchains = &g_kisVkInfo.m_swapchain,
        .pImageIndices = &iImage,
        .pResults = nullptr,
    };

    vkQueuePresentKHR(g_kisVkInfo.m_presentQueue, &presentInfo);
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
void kisVkResize(uint32_t width, uint32_t height)
{
    kisVkPrepareResolution(width, height);
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
uint32_t kisVkCreateMesh(kisVertex_XYZ_UV_Color_Normal_Tangent_Bitangent* vertices, uint32_t nVertices, void* indices, uint32_t nIndices, kisIndexBufferType indexBufferType)
{
    const size_t vertexBufferSize = sizeof(kisVertex_XYZ_UV_Color_Normal_Tangent_Bitangent) * nVertices;
    const size_t indexBufferSize = nIndices * (indexBufferType == kisIndexBufferType::U16 ? sizeof(uint16_t) : sizeof(uint32_t));

    kisVkMesh& mesh = g_kisVkMeshes.add();
    kisVkCreateVertexBuffer(vertices, vertexBufferSize, mesh.m_vertexBuffer, mesh.m_vertexBufferAlloc);
    kisVkCreateIndexBuffer(indices, indexBufferSize, mesh.m_indexBuffer, mesh.m_indexBufferAlloc);
    mesh.m_nVertices = nVertices;
    mesh.m_nIndices = nIndices;
    mesh.m_indexType = kisVkIndexType(indexBufferType);

    return g_kisVkMeshes.num() - 1;
}
