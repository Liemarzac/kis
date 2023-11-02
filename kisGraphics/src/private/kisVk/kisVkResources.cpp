#include "kisVkResources.h"

#include <cstring>

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
bool kisVKMemoryTypeFromProperties(uint32_t typeBits, VkFlags requirementsMask, uint32_t& typeIndex)
{
    // Search memtypes to find first index with those properties
    for(uint32_t i = 0; i < VK_MAX_MEMORY_TYPES; i++)
    {
        if((typeBits & 1) != 0)
        {
            // Type is available, does it match user properties?
            if((g_kisVkInfo.m_memoryProperties.memoryTypes[i].propertyFlags & requirementsMask) != 0)
            {
                typeIndex = i;
                return true;
            }
        }

        typeBits >>= 1;
    }

    // No memory types matched, return failure
    return false;
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
void kisVkCreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& memory)
{
    VkBufferCreateInfo bufferCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = size,
        .usage = usage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE
    };

    KIS_VK_CHECK(vkCreateBuffer(g_kisVkInfo.m_device, &bufferCreateInfo, &g_kisVkAllocCallbacks, &buffer));

    VkMemoryRequirements memReqs;
    vkGetBufferMemoryRequirements(g_kisVkInfo.m_device, buffer, &memReqs);

    uint32_t memTypeIndex;
    KIS_CHECK(kisVKMemoryTypeFromProperties(memReqs.memoryTypeBits, properties, memTypeIndex));

    const VkMemoryAllocateInfo memAllocInfo = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = memReqs.size,
        .memoryTypeIndex = memTypeIndex,
    };

    KIS_VK_CHECK(vkAllocateMemory(g_kisVkInfo.m_device, &memAllocInfo, &g_kisVkAllocCallbacks, &memory));

    vkBindBufferMemory(g_kisVkInfo.m_device, buffer, memory, 0);
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
void kisVKCopyBuffer(VkBuffer dst, VkBuffer src, VkDeviceSize size)
{
    const VkCommandBufferAllocateInfo cmdBufferAllocInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandPool = g_kisVkInfo.m_cmdPool,
        .commandBufferCount = 1,
    };
    
    VkCommandBuffer cmdBuffer;
    vkAllocateCommandBuffers(g_kisVkInfo.m_device, &cmdBufferAllocInfo, &cmdBuffer);
    
    const VkCommandBufferBeginInfo beginInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };
    
    vkBeginCommandBuffer(cmdBuffer, &beginInfo);

    VkBufferCopy copyRegion = {
        .srcOffset = 0,
        .dstOffset = 0,
        .size = size
    };

    vkCmdCopyBuffer(cmdBuffer, src, dst, 1, &copyRegion);

    vkEndCommandBuffer(cmdBuffer);

    VkSubmitInfo submitInfo = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers = &cmdBuffer
    };

    vkQueueSubmit(g_kisVkInfo.m_graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(g_kisVkInfo.m_graphicsQueue);

    vkFreeCommandBuffers(g_kisVkInfo.m_device, g_kisVkInfo.m_cmdPool, 1, &cmdBuffer);
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
void kisVkCreateVertexBuffer(void* vertices, VkDeviceSize size, VkBuffer& buffer, VkDeviceMemory& memory)
{
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingMemory;

    kisVkCreateBuffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingMemory);

    // Map vertex buffer.
    {
        void* mappedBuffer;
        vkMapMemory(g_kisVkInfo.m_device, stagingMemory, 0, size, 0, &mappedBuffer);
        memcpy(mappedBuffer, vertices, size);
        vkUnmapMemory(g_kisVkInfo.m_device, stagingMemory);
    }

    kisVkCreateBuffer(size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, buffer, memory);

    kisVKCopyBuffer(buffer, stagingBuffer , size);

    vkFreeMemory(g_kisVkInfo.m_device, stagingMemory, &g_kisVkAllocCallbacks);
    vkDestroyBuffer(g_kisVkInfo.m_device, stagingBuffer, &g_kisVkAllocCallbacks);
}

