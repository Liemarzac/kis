#pragma once

#include <kisCore/kisCore.h>

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
void kisGraphicsInit(const void* metalLayer);
void kisGraphicsShutdown();

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
void kisGraphicsRender();
void kisGraphicsResize(uint32_t width, uint32_t height);
