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
    VkShaderModule m_vsShader;
    VkShaderModule m_psShader;
};

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
struct kisVertexFormatPos2Color3
{
    kisVec2 m_pos;
    kisVec3 m_color;
};

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
kisVkRenderContext g_kisVkRenderContext;
kisVk* g_kisVk = nullptr;

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
    #if defined(KIS_DEBUG)
    static uint32_t s_allocIndex = 0;
    kisStringANSIStatic<64> allocName;
    allocName.concat("vulkan_alloc_%u", s_allocIndex);
    s_allocIndex++;
    #endif

    return KIS_ALIGNED_ALLOC(size, (uint32_t)alignment, kisMemTag::Vulkan, allocName.c_str());
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
    return KIS_REALLOC(pOriginal, size);
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
void kisVkFree(void* pUserData, void* pointer)
{
    KIS_FREE(pointer);
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
    KIS_MEM_REGISTER_EXTERNAL(g_kisVkInfo.m_instance, kisMemTag::Vulkan,  "vulkan_instance");

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
    KIS_MEM_REGISTER_EXTERNAL(g_kisVkInfo.m_surface, kisMemTag::Vulkan, "vulkan_metal_surface");
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
    KIS_MEM_REGISTER_EXTERNAL(g_kisVkInfo.m_device, kisMemTag::Vulkan, "vulkan_device");

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
    KIS_MEM_REGISTER_EXTERNAL(g_kisVkInfo.m_cmdBuffer, kisMemTag::Vulkan, "vulkan_command_buffer");

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
    KIS_MEM_REGISTER_EXTERNAL(g_kisVkInfo.m_descriptorPool, kisMemTag::Vulkan, "vulkan_descriptor_pool");

    const VkSemaphoreCreateInfo semaphoreCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
    };

    vkCreateSemaphore(g_kisVkInfo.m_device, &semaphoreCreateInfo, &g_kisVkAllocCallbacks, &g_kisVkRenderContext.m_imageAvailableSemaphore);
    KIS_MEM_REGISTER_EXTERNAL(g_kisVkRenderContext.m_imageAvailableSemaphore, kisMemTag::Vulkan, "vulkan_semaphore_image_avail");

    vkCreateSemaphore(g_kisVkInfo.m_device, &semaphoreCreateInfo, &g_kisVkAllocCallbacks, &g_kisVkRenderContext.m_queueExecutedSemaphore);
    KIS_MEM_REGISTER_EXTERNAL(g_kisVkRenderContext.m_queueExecutedSemaphore, kisMemTag::Vulkan, "vulkan_semaphore_queue_executed");

    const VkFenceCreateInfo fenceCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT,
    };

    vkCreateFence(g_kisVkInfo.m_device, &fenceCreateInfo, &g_kisVkAllocCallbacks, &g_kisVkRenderContext.m_queueExecutedFence);
    KIS_MEM_REGISTER_EXTERNAL(g_kisVkRenderContext.m_queueExecutedFence, kisMemTag::Vulkan, "vulkan_fence_queue_executed");

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
    KIS_MEM_REGISTER_EXTERNAL(g_kisVkRenderContext.m_renderPass, kisMemTag::Vulkan, "vulkan_render_pass");
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
    KIS_MEM_REGISTER_EXTERNAL(shader, kisMemTag::Vulkan, "vulkan_shader_module");
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
        KIS_MEM_REGISTER_EXTERNAL(buffer, kisMemTag::Vulkan, "uniform_buffer");
        KIS_MEM_REGISTER_EXTERNAL(alloc, kisMemTag::Vulkan, "uniform_buffer_alloc");
        KIS_ASSERT(allocInfo.pMappedData != nullptr);
        g_kisVk->m_frames[i].m_uniformBuffer = buffer;
        g_kisVk->m_frames[i].m_uniformBufferAlloc = alloc;
        g_kisVk->m_frames[i].m_uniformBufferMapped = (kisByte*)allocInfo.pMappedData;
        g_kisVk->m_frames[i].m_uniformBufferOffset = 0;
        g_kisVk->m_frames[i].m_uniformBufferSize = uniformBufferSize;
    }

    kisFileBuffer vsBuffer = kisFileBufferCreate("data/triangle/triangle_vert.spv");
    g_kisVkRenderContext.m_vsShader = kisVkCreateShader(vsBuffer);
    kisFileBufferDestroy(vsBuffer);

    kisFileBuffer psBuffer = kisFileBufferCreate("data/triangle/triangle_frag.spv");
    g_kisVkRenderContext.m_psShader = kisVkCreateShader(psBuffer);
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
            .module = g_kisVkRenderContext.m_vsShader,
            .pName = "main",
        },
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .pNext = nullptr,
            .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
            .module = g_kisVkRenderContext.m_psShader,
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
    KIS_MEM_REGISTER_EXTERNAL(g_kisVkRenderContext.m_objectDescriptorSetLayout, kisMemTag::Vulkan, "vulkan_descriptorset_layout");

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
    KIS_MEM_REGISTER_EXTERNAL(g_kisVkRenderContext.m_pipelineLayout, kisMemTag::Vulkan, "vulkan_pipeline_layout");

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
    KIS_MEM_REGISTER_EXTERNAL(g_kisVkRenderContext.m_pipeline, kisMemTag::Vulkan, "vulkan_pipeline");
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
void kisVkUnloadAssets()
{
    KIS_MEM_UNREGISTER_EXTERNAL(g_kisVkRenderContext.m_pipeline);
    vkDestroyPipeline(g_kisVkInfo.m_device, g_kisVkRenderContext.m_pipeline, &g_kisVkAllocCallbacks);
    g_kisVkRenderContext.m_pipeline = VK_NULL_HANDLE;

    KIS_MEM_UNREGISTER_EXTERNAL(g_kisVkRenderContext.m_pipelineLayout);
    vkDestroyPipelineLayout(g_kisVkInfo.m_device, g_kisVkRenderContext.m_pipelineLayout, &g_kisVkAllocCallbacks);
    g_kisVkRenderContext.m_pipelineLayout = VK_NULL_HANDLE;
    
    KIS_MEM_UNREGISTER_EXTERNAL(g_kisVkRenderContext.m_objectDescriptorSetLayout);
    vkDestroyDescriptorSetLayout(g_kisVkInfo.m_device, g_kisVkRenderContext.m_objectDescriptorSetLayout, &g_kisVkAllocCallbacks);
    g_kisVkRenderContext.m_objectDescriptorSetLayout = VK_NULL_HANDLE;

    KIS_MEM_UNREGISTER_EXTERNAL(g_kisVkRenderContext.m_psShader);
    vkDestroyShaderModule(g_kisVkInfo.m_device, g_kisVkRenderContext.m_psShader, &g_kisVkAllocCallbacks);
    g_kisVkRenderContext.m_psShader = VK_NULL_HANDLE;

    KIS_MEM_UNREGISTER_EXTERNAL(g_kisVkRenderContext.m_vsShader);
    vkDestroyShaderModule(g_kisVkInfo.m_device, g_kisVkRenderContext.m_vsShader, &g_kisVkAllocCallbacks);
    g_kisVkRenderContext.m_vsShader = VK_NULL_HANDLE;

    for(uint32_t i = 0 ; i < g_kisVkInfo.m_nImages; ++ i)
    {
        KIS_MEM_UNREGISTER_EXTERNAL(g_kisVk->m_frames[i].m_uniformBufferAlloc);
        KIS_MEM_UNREGISTER_EXTERNAL(g_kisVk->m_frames[i].m_uniformBuffer);
        vmaDestroyBuffer(g_kisVkInfo.m_vmaAllocator, g_kisVk->m_frames[i].m_uniformBuffer, g_kisVk->m_frames[i].m_uniformBufferAlloc);
        g_kisVk->m_frames[i].m_uniformBuffer = VK_NULL_HANDLE;
        g_kisVk->m_frames[i].m_uniformBufferAlloc = VK_NULL_HANDLE;
        g_kisVk->m_frames[i].m_uniformBufferMapped = nullptr;
        g_kisVk->m_frames[i].m_uniformBufferOffset = 0;
        g_kisVk->m_frames[i].m_uniformBufferSize = 0;
    }
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

    // Old swapchain is destroyed in the same call used to create the new one.
    KIS_MEM_UNREGISTER_EXTERNAL(oldSwapchain);
    
    KIS_VK_CHECK(vkCreateSwapchainKHR(g_kisVkInfo.m_device, &swapchainCreateInfo, &g_kisVkAllocCallbacks, &g_kisVkInfo.m_swapchain));
    KIS_MEM_REGISTER_EXTERNAL(g_kisVkInfo.m_swapchain, kisMemTag::Vulkan, "vulkan_swapchain");

    g_kisVkInfo.m_nImages = nSwapchainImages;

    // Destroy old image views.
    for(int i = 0 ; i < g_kisVkInfo.m_swapchainImageViews.num(); ++i)
    {
        KIS_MEM_UNREGISTER_EXTERNAL(g_kisVkInfo.m_swapchainImageViews[i]);
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
        KIS_MEM_REGISTER_EXTERNAL(imageView, kisMemTag::Vulkan, "vulkan_image_view");
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
        KIS_MEM_UNREGISTER_EXTERNAL(g_kisVkInfo.m_depthImageView);
        KIS_MEM_UNREGISTER_EXTERNAL(g_kisVkInfo.m_depthImage);
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
    KIS_MEM_REGISTER_EXTERNAL(g_kisVkInfo.m_depthImage, kisMemTag::Vulkan, "depth_image");

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
    KIS_MEM_REGISTER_EXTERNAL(g_kisVkInfo.m_depthImageView, kisMemTag::Vulkan, "vulkan_depth_image_view");
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
        KIS_MEM_REGISTER_EXTERNAL(frameBuffer, kisMemTag::Vulkan, "vulkan_frame_buffer");
        g_kisVkInfo.m_frameBuffers.add(frameBuffer);
    }
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
void kisVkDestroyFramebuffers()
{
    for(VkFramebuffer frameBuffer : g_kisVkInfo.m_frameBuffers)
    {
        KIS_MEM_UNREGISTER_EXTERNAL(frameBuffer);
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
    KIS_MEM_SCOPE_BEGIN(kisMemTag::Vulkan);

    kisLog("Initializing Vulkan");

    g_kisVk = KIS_NEW(kisMemTag::Vulkan, "vk")kisVk();

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
    kisVkUnloadAssets();

    for(uint32_t i = 0; i < g_kisVkInfo.m_nImages; i++)
    {
        KIS_MEM_UNREGISTER_EXTERNAL(g_kisVk->m_frames[i].m_uniformBuffer);
        vkDestroyBuffer(g_kisVkInfo.m_device, g_kisVk->m_frames[i].m_uniformBuffer, &g_kisVkAllocCallbacks);
        KIS_MEM_UNREGISTER_EXTERNAL(g_kisVk->m_frames[i].m_uniformBufferAlloc);
        vmaFreeMemory(g_kisVkInfo.m_vmaAllocator, g_kisVk->m_frames[i].m_uniformBufferAlloc);
    }

    for(VkSemaphore semaphore : g_kisVkInfo.m_imageAvailSemaphore)
    {
        KIS_MEM_UNREGISTER_EXTERNAL(semaphore);
        vkDestroySemaphore(g_kisVkInfo.m_device, semaphore, &g_kisVkAllocCallbacks);
    }

    KIS_MEM_UNREGISTER_EXTERNAL(g_kisVkRenderContext.m_renderPass);
    vkDestroyRenderPass(g_kisVkInfo.m_device, g_kisVkRenderContext.m_renderPass, &g_kisVkAllocCallbacks);

    KIS_MEM_UNREGISTER_EXTERNAL(g_kisVkRenderContext.m_queueExecutedFence);
    vkDestroyFence(g_kisVkInfo.m_device, g_kisVkRenderContext.m_queueExecutedFence, &g_kisVkAllocCallbacks);

    KIS_MEM_UNREGISTER_EXTERNAL(g_kisVkRenderContext.m_queueExecutedSemaphore);
    vkDestroySemaphore(g_kisVkInfo.m_device, g_kisVkRenderContext.m_queueExecutedSemaphore, &g_kisVkAllocCallbacks);

    KIS_MEM_UNREGISTER_EXTERNAL(g_kisVkRenderContext.m_imageAvailableSemaphore);
    vkDestroySemaphore(g_kisVkInfo.m_device, g_kisVkRenderContext.m_imageAvailableSemaphore, &g_kisVkAllocCallbacks);

    if(g_kisVkInfo.m_descriptorPool != VK_NULL_HANDLE)
    {
        KIS_MEM_UNREGISTER_EXTERNAL(g_kisVkInfo.m_descriptorPool);
        vkDestroyDescriptorPool(g_kisVkInfo.m_device, g_kisVkInfo.m_descriptorPool, &g_kisVkAllocCallbacks);
    }

    if(g_kisVkInfo.m_cmdBuffer != VK_NULL_HANDLE)
    {
        KIS_MEM_UNREGISTER_EXTERNAL(g_kisVkInfo.m_cmdBuffer);
        vkFreeCommandBuffers(g_kisVkInfo.m_device, g_kisVkInfo.m_cmdPool, 1, &g_kisVkInfo.m_cmdBuffer);
    }

    if(g_kisVkInfo.m_cmdPool != VK_NULL_HANDLE)
    {
        KIS_MEM_UNREGISTER_EXTERNAL(g_kisVkInfo.m_cmdPool);
        vkDestroyCommandPool(g_kisVkInfo.m_device, g_kisVkInfo.m_cmdPool, &g_kisVkAllocCallbacks);
    }

    for(VkFramebuffer framebuffer : g_kisVkInfo.m_frameBuffers)
    {
        KIS_MEM_UNREGISTER_EXTERNAL(framebuffer);
        vkDestroyFramebuffer(g_kisVkInfo.m_device, framebuffer, &g_kisVkAllocCallbacks);
    }

    if(g_kisVkInfo.m_depthImageView != VK_NULL_HANDLE)
    {
        KIS_MEM_UNREGISTER_EXTERNAL(g_kisVkInfo.m_depthImageView);
        vkDestroyImageView(g_kisVkInfo.m_device, g_kisVkInfo.m_depthImageView, &g_kisVkAllocCallbacks);
    }

    if(g_kisVkInfo.m_depthImage != VK_NULL_HANDLE)
    {
        KIS_MEM_UNREGISTER_EXTERNAL(g_kisVkInfo.m_depthImage);
        vkDestroyImage(g_kisVkInfo.m_device, g_kisVkInfo.m_depthImage, &g_kisVkAllocCallbacks);
    }

    if(g_kisVkInfo.m_depthAlloc)
    {
        KIS_MEM_UNREGISTER_EXTERNAL(g_kisVkInfo.m_depthAlloc);
        vmaFreeMemory(g_kisVkInfo.m_vmaAllocator, g_kisVkInfo.m_depthAlloc);
    }

    for(VkImageView imageView : g_kisVkInfo.m_swapchainImageViews)
    {
        KIS_MEM_UNREGISTER_EXTERNAL(imageView);
        vkDestroyImageView(g_kisVkInfo.m_device, imageView, &g_kisVkAllocCallbacks);
    }

    if(g_kisVkInfo.m_swapchain != VK_NULL_HANDLE)
    {
        KIS_MEM_UNREGISTER_EXTERNAL(g_kisVkInfo.m_swapchain);
        vkDestroySwapchainKHR(g_kisVkInfo.m_device, g_kisVkInfo.m_swapchain, &g_kisVkAllocCallbacks);
    }

    if(g_kisVkInfo.m_surface != VK_NULL_HANDLE)
    {
        KIS_MEM_UNREGISTER_EXTERNAL(g_kisVkInfo.m_surface);
        vkDestroySurfaceKHR(g_kisVkInfo.m_instance, g_kisVkInfo.m_surface, &g_kisVkAllocCallbacks);
    }

    if(g_kisVkInfo.m_instance != VK_NULL_HANDLE)
    {
        KIS_MEM_UNREGISTER_EXTERNAL(g_kisVkInfo.m_instance);
        vkDestroyInstance(g_kisVkInfo.m_instance, nullptr);
    }

    if(g_kisVkInfo.m_vmaAllocator)
    {
        KIS_MEM_UNREGISTER_EXTERNAL(g_kisVkInfo.m_vmaAllocator);
        vmaDestroyAllocator(g_kisVkInfo.m_vmaAllocator);
    }

    if(g_kisVkInfo.m_device != VK_NULL_HANDLE)
    {
        KIS_MEM_UNREGISTER_EXTERNAL(g_kisVkInfo.m_device);
        vkDestroyDevice(g_kisVkInfo.m_device, &g_kisVkAllocCallbacks);
    }

    KIS_DELETE(g_kisVk);

    KIS_MEM_SCOPE_END(kisMemTag::Vulkan);
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

    kisVkFrame& frame = g_kisVk->m_frames[iImage];

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
        frame.m_uniformBufferOffset = kisAlignPowerOf2(g_kisVk->m_frames[iImage].m_uniformBufferOffset, 16);

        kisVkDraw& draw = frame.m_draws.add();
        draw.m_descriptorSet = descriptorSet;
        draw.m_iMesh = renderParams.m_instances[i].m_meshHandle.m_index;
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

    for(const kisVkDraw& draw : g_kisVk->m_frames[iImage].m_draws)
    {
        vkCmdBindDescriptorSets(g_kisVkInfo.m_cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, g_kisVkRenderContext.m_pipelineLayout, 0, 1, &draw.m_descriptorSet, 0, nullptr);

        const kisVkMesh& mesh = g_kisVk->m_meshes[draw.m_iMesh];

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

    kisVkMesh& mesh = g_kisVk->m_meshes.add();
    kisVkCreateVertexBuffer(vertices, vertexBufferSize, mesh.m_vertexBuffer, mesh.m_vertexBufferAlloc);
    kisVkCreateIndexBuffer(indices, indexBufferSize, mesh.m_indexBuffer, mesh.m_indexBufferAlloc);
    mesh.m_nVertices = nVertices;
    mesh.m_nIndices = nIndices;
    mesh.m_indexType = kisVkIndexType(indexBufferType);

    return g_kisVk->m_meshes.num() - 1;
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
void kisVkDestroyMesh(uint32_t iMesh)
{
    kisVkMesh& mesh = g_kisVk->m_meshes[iMesh];
    kisVkDestroyIndexBuffer(mesh.m_indexBuffer, mesh.m_indexBufferAlloc);
    kisVkDestroyVertexBuffer(mesh.m_vertexBuffer, mesh.m_vertexBufferAlloc);
    mesh.m_vertexBuffer = VK_NULL_HANDLE;
    mesh.m_vertexBufferAlloc = VK_NULL_HANDLE;
    mesh.m_indexBuffer = VK_NULL_HANDLE;
    mesh.m_indexBufferAlloc = VK_NULL_HANDLE;
    mesh.m_nVertices = 0;
    mesh.m_nIndices = 0;
}
