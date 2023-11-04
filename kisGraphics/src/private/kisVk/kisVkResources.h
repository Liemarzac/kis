#pragma once

#include "kisVkPrivate.h"

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
bool kisVKMemoryTypeFromProperties(uint32_t typeBits, VkFlags requirementsMask, uint32_t& typeIndex);

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
void kisVkCreateVertexBuffer(void* vertices, VkDeviceSize size, VkBuffer& buffer, VmaAllocation& alloc);

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
void kisVkCreateIndexBuffer(void* indices, VkDeviceSize size, VkBuffer& buffer, VmaAllocation& alloc);

