#pragma once

#include "kisCoreShared.h"

#include <stddef.h>



void* kisMemAlloc(size_t size, uint32_t alignment = 8);
void kisMemFree(void* p);
