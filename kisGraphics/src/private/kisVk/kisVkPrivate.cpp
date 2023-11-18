#define VMA_IMPLEMENTATION
#pragma clang diagnostic push
//#pragma clang diagnostic ignored "-Wdocumentation"
//#pragma clang diagnostic ignored "-Wnullability-completeness"
#pragma clang diagnostic ignored "-Wall"
//#include <vk_mem_alloc.h>
#pragma clang diagnostic pop


#include "kisVkPrivate.h"


kisVkInfo g_kisVkInfo;
VkAllocationCallbacks g_kisVkAllocCallbacks;
kisStaticArray<kisVkFrame, k_kisVkMaxNumImages> g_kisVkFrames;
kisFixedArray<kisVkMesh, 1024> g_kisVkMeshes;
