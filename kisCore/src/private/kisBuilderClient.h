#pragma once

#include "kisStringANSIStatic.h"


//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
bool kisBuilder_ClientConnect(const char* url);
void kisBuilder_ClientDisconnect();

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
bool kisBuilder_ClientRequestResource(const kisString64& path);

