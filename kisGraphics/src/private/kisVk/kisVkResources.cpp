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
void kisVkCopyBuffer(VkBuffer src, VkBuffer dst, VkDeviceSize size)
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

void kisVkCreateBuffer(void* data, VkDeviceSize size, VkBufferUsageFlags usage, VkBuffer& buffer, VmaAllocation& alloc)
{
    // Create staging buffer.
    VkBuffer stagingBuffer;
    VmaAllocation stagingBufferAlloc;
    VmaAllocationInfo stagingBufferAllocInfo;

    const VkBufferCreateInfo stagingBufferCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = size,
        .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE
    };

    const VmaAllocationCreateInfo allocCreateInfo = {
        .usage = VMA_MEMORY_USAGE_AUTO,
        .flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
    };

    vmaCreateBuffer(g_kisVkInfo.m_vmaAllocator, &stagingBufferCreateInfo, &allocCreateInfo, &stagingBuffer, &stagingBufferAlloc, &stagingBufferAllocInfo);

    // Map vertex buffer.
    {
        void* mappedBuffer;
        vmaMapMemory(g_kisVkInfo.m_vmaAllocator, stagingBufferAlloc, &mappedBuffer);
        memcpy(mappedBuffer, data, size);
        vmaUnmapMemory(g_kisVkInfo.m_vmaAllocator, stagingBufferAlloc);
    }

    // Create device buffer
    VmaAllocationInfo deviceBufferAllocationInfo;

    const VkBufferCreateInfo deviceBufferCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = size,
        .usage = usage | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE
    };

    const VmaAllocationCreateInfo deviceAllocCreateInfo = {
        .usage = VMA_MEMORY_USAGE_AUTO,
        .flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
    };

    vmaCreateBuffer(g_kisVkInfo.m_vmaAllocator, &deviceBufferCreateInfo, &deviceAllocCreateInfo, &buffer, &alloc, &deviceBufferAllocationInfo);

    kisVkCopyBuffer(stagingBuffer, buffer, size);

    vmaDestroyBuffer(g_kisVkInfo.m_vmaAllocator, stagingBuffer, stagingBufferAlloc);
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
void kisVkCreateVertexBuffer(void* vertices, VkDeviceSize size, VkBuffer& buffer, VmaAllocation& alloc)
{
    kisVkCreateBuffer(vertices, size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, buffer, alloc);
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
void kisVkCreateIndexBuffer(void* indices, VkDeviceSize size, VkBuffer& buffer, VmaAllocation& alloc)
{
    kisVkCreateBuffer(indices, size, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, buffer, alloc);
}

