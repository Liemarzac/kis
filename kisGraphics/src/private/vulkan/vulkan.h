#pragma once

#include <kisCore/core.h>

#define VULKAN_CHECK(X) if(X != VK_SUCCESS){ASSERT(false)}

void vulkan_Init(const void* metalLayer);
