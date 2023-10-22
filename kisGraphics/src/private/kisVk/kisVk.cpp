#include "kisVk.h"

#include <MoltenVK/mvk_vulkan.h>

#include <kisCore/kisStringANSIStatic.h>
#include <kisCore/kisArrayStatic.h>

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
struct kisVkInfo
{
    VkInstance m_instance;
    VkPhysicalDevice m_physicalDevice;
    VkDevice m_device;
    VkPhysicalDeviceProperties m_deviceProperties;
    VkSurfaceKHR m_surface;
    VkCommandPool m_cmdPool;
    VkSwapchainKHR m_swapchain;
    VkSurfaceFormatKHR m_surfaceFormat;
    VkExtent2D m_swapchainSize;
    uint32_t m_iGraphicsQueueFamily;
    uint32_t m_iPresentQueueFamily;
    bool m_bValidate;
    kisArrayStatic<const char*, 64> m_instanceExtensionNames;
    kisArrayStatic<const char*, 64> m_deviceExtensionNames;
    kisArrayStatic<const char*, 16> m_layerNames;
    kisArrayStatic<VkImage, 3> m_swapchainImages;
    kisArrayStatic<VkImageView, 3> m_swapchainImageViews;
};

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
struct kisVkMemBlock
{
    void* m_pointer;
    size_t m_size;
};

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
kisVkInfo g_kisVkInfo;
VkAllocationCallbacks g_vulkanAllocCallbacks;
kisArrayStatic<kisVkMemBlock, 2048> g_vulkanMemBlocks;

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
    kisVkMemBlock block = {
        .m_pointer = pointer,
        .m_size = size
    };

    g_vulkanMemBlocks.add(block);
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
    size_t originalSize = 0;
    for(uint32_t i = 0; i < g_vulkanMemBlocks.num(); ++i)
    {
        if(g_vulkanMemBlocks[i].m_pointer == pOriginal)
        {
            originalSize = g_vulkanMemBlocks[i].m_size;
            g_vulkanMemBlocks.removeUnordered(i);
            break;
        }
    }

    void* newPointer = aligned_alloc(alignment, size);

    size_t cpySize = originalSize;
    if(originalSize > size)
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
    for(uint32_t i = 0; i < g_vulkanMemBlocks.num(); ++i)
    {
        if(g_vulkanMemBlocks[i].m_pointer == pointer)
        {
            free(pointer);
            g_vulkanMemBlocks.removeUnordered(i);
            break;
        }
    }
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

    if (s_bBreakOnValidationCallback)
    {
#ifndef WIN32
        raise(SIGTRAP);
#else
        DebugBreak();
#endif
    }

    if ((messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT) != 0)
    {
        message.concat("VERBOSE : ");
    }

    if ((messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) != 0)
    {
        message.concat("INFO : ");
    }

    if ((messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) != 0)
    {
        message.concat("WARNING : ");
    }

    if ((messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) != 0)
    {
        message.concat("ERROR : ");
    }

    if ((messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT) != 0)
    {
        message.concat("GENERAL");
    }
    else
    {
        if ((messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT) != 0)
        {
            message.concat("VALIDATION");
        }

        if ((messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT) != 0)
        {
            if ((messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT) != 0)
            {
                message.concat("|");
            }
            message.concat("PERFORMANCE");
        }
    }

    message.concat(" - Message Id Number: %d | Message Id Name: %s\n\t%s\n", pCallbackData->messageIdNumber, pCallbackData->pMessageIdName == NULL ? "" : pCallbackData->pMessageIdName, pCallbackData->pMessage);

    printf("%s\n", message.c_str());

    return false;
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
void kisVkInitInstanceLayers()
{
    const uint32_t k_maxEntries = 100;
    kisArrayStatic<VkLayerProperties, k_maxEntries> propertiesArray(kisArrayStaticInit::Fill);

    bool bValidationLayerActive = false;
    
    vkEnumerateInstanceLayerProperties(propertiesArray.numPointer(), propertiesArray.dataPointer());

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
    const uint32_t k_maxEntries = 100;
    kisArrayStatic<VkExtensionProperties, k_maxEntries> propertiesArray(kisArrayStaticInit::Fill);

    vkEnumerateInstanceExtensionProperties(VK_NULL_HANDLE, propertiesArray.numPointer(), propertiesArray.dataPointer());

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

    KIS_VK_CHECK(vkCreateInstance(&instanceCreateInfo, &g_vulkanAllocCallbacks, &g_kisVkInfo.m_instance));

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
    KIS_VK_CHECK(vkCreateMetalSurfaceEXT(g_kisVkInfo.m_instance, &surfaceCreateInfo, &g_vulkanAllocCallbacks, &g_kisVkInfo.m_surface));
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
void kisVkPickPhysicalDevice()
{
    g_kisVkInfo.m_physicalDevice = nullptr;
    g_kisVkInfo.m_iGraphicsQueueFamily = k_kisCoreInvalidIndex;
    g_kisVkInfo.m_iPresentQueueFamily = k_kisCoreInvalidIndex;
    
    const uint32_t k_maxPhysicalDevices = 16;
    kisArrayStatic<VkPhysicalDevice, k_maxPhysicalDevices> physicalDevices(kisArrayStaticInit::Fill);

    KIS_VK_CHECK(vkEnumeratePhysicalDevices(g_kisVkInfo.m_instance, physicalDevices.numPointer(), physicalDevices.dataPointer()));

    const uint32_t k_maxQueueFamilityProperties = 32;
    kisArrayStatic<VkQueueFamilyProperties, k_maxQueueFamilityProperties> queueFamilyProperties(kisArrayStaticInit::Fill);

    int iBestDevice = k_kisCoreInvalidIndex;
    uint32_t bestImageDimension = 0;
    bool bFoundDiscreteGPU = false;

    for(uint32_t iDevice = 0; iDevice < physicalDevices.num(); ++iDevice)
    {
        VkPhysicalDevice& device = physicalDevices[iDevice];
        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties(device, &properties);

        vkGetPhysicalDeviceQueueFamilyProperties(device, queueFamilyProperties.numPointer(), queueFamilyProperties.dataPointer());

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
                g_kisVkInfo.m_deviceProperties = properties;
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
    
    const uint32_t k_maxDeviceExtensionProperties = 256;
    kisArrayStatic<VkExtensionProperties, k_maxDeviceExtensionProperties> deviceExtensionProperties(kisArrayStaticInit::Fill);
    vkEnumerateDeviceExtensionProperties(g_kisVkInfo.m_physicalDevice, nullptr, deviceExtensionProperties.numPointer(), deviceExtensionProperties.dataPointer());
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

    //demo->fpGetPhysicalDeviceSurfaceFormatsKHR(demo->gpu, demo->surface, &formatCount, surfFormats);
    kisArrayStatic<VkSurfaceFormatKHR, 64> surfaceFormats(kisArrayStaticInit::Fill);
    g_vulkanFuncPtrGetPhysicalDeviceSurfaceFormatsKHR(g_kisVkInfo.m_physicalDevice, g_kisVkInfo.m_surface, surfaceFormats.numPointer(), surfaceFormats.dataPointer());

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

    KIS_VK_CHECK(vkCreateDevice(g_kisVkInfo.m_physicalDevice, &deviceCreateInfos, &g_vulkanAllocCallbacks, &g_kisVkInfo.m_device));
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
void kisVkPrepare()
{
    // Surface capabilities.
    VkSurfaceCapabilitiesKHR surfaceCapabilities;
    KIS_VK_CHECK(g_vulkanFuncPtrGetPhysicalDeviceSurfaceCapabilitiesKHR(g_kisVkInfo.m_physicalDevice, g_kisVkInfo.m_surface, &surfaceCapabilities));

    g_kisVkInfo.m_swapchainSize = surfaceCapabilities.maxImageExtent;


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

    KIS_VK_CHECK(vkCreateSwapchainKHR(g_kisVkInfo.m_device, &swapchainCreateInfo, &g_vulkanAllocCallbacks, &g_kisVkInfo.m_swapchain));

    if(oldSwapchain != VK_NULL_HANDLE)
    {
        vkDestroySwapchainKHR(g_kisVkInfo.m_device, oldSwapchain, &g_vulkanAllocCallbacks);
    }

    // Get swapchain images.
    g_kisVkInfo.m_swapchainImages.fill();
    KIS_VK_CHECK(vkGetSwapchainImagesKHR(g_kisVkInfo.m_device, g_kisVkInfo.m_swapchain, g_kisVkInfo.m_swapchainImages.numPointer(), g_kisVkInfo.m_swapchainImages.dataPointer()));

    // Destroy old image views.
    for(int i = 0 ; i < g_kisVkInfo.m_swapchainImageViews.num(); ++i)
    {
        vkDestroyImageView(g_kisVkInfo.m_device, g_kisVkInfo.m_swapchainImageViews[i], &g_vulkanAllocCallbacks);
    }
    g_kisVkInfo.m_swapchainImageViews.empty();

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
        KIS_VK_CHECK(vkCreateImageView(g_kisVkInfo.m_device, &imageViewCreateInfo, &g_vulkanAllocCallbacks, &imageView));
        g_kisVkInfo.m_swapchainImageViews.add(imageView);
    }

    // Create command pool.
    KIS_ASSERT(g_kisVkInfo.m_cmdPool == nullptr);
    VkCommandPoolCreateInfo cmdPoolCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .pNext = NULL,
        .queueFamilyIndex = g_kisVkInfo.m_iGraphicsQueueFamily,
        .flags = 0,
    };

    KIS_VK_CHECK(vkCreateCommandPool(g_kisVkInfo.m_device, &cmdPoolCreateInfo, &g_vulkanAllocCallbacks, &g_kisVkInfo.m_cmdPool));
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
void kisVkInit(const void* metalLayer)
{
    printf("Initializing Vulkan\n");

    memset(&g_kisVkInfo, 0, sizeof(g_kisVkInfo));
    g_kisVkInfo.m_bValidate = true;

    memset(&g_vulkanAllocCallbacks, 0, sizeof(g_vulkanAllocCallbacks));
    g_vulkanAllocCallbacks.pUserData = (void*)&g_kisVkInfo;
    g_vulkanAllocCallbacks.pfnAllocation = kisVkAlloc;
    g_vulkanAllocCallbacks.pfnReallocation = kisVkRealloc;
    g_vulkanAllocCallbacks.pfnFree = kisVkFree;

    kisVkInitInstanceLayers();

    bool bPortabilityEnumerationActive = false;
    kisVkInitInstanceExtensions(bPortabilityEnumerationActive);
    kisVkCreateInstance(bPortabilityEnumerationActive);
    kisVkCreateSurface(metalLayer);
    kisVkPickPhysicalDevice();
    kisVkCreateDevice();
    kisVkPrepare();
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
void kisVkShutdown()
{
    if(g_kisVkInfo.m_instance != VK_NULL_HANDLE)
    {
        vkDestroyInstance(g_kisVkInfo.m_instance, nullptr);
    }
}
