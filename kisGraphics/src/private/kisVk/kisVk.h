#pragma once

#include <kisCore/kisCore.h>

#define KIS_VK_CHECK(X) if(X != VK_SUCCESS){KIS_ASSERT(false)}

void kisVkInit(const void* metalLayer);
void kisVkShutdown();
