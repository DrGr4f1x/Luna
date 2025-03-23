//
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
// Author:  David Elder
//

#include "Stdafx.h"

#include "Profiling.h"

#include "pix3.h"


namespace Luna
{

void BeginEvent(const std::string& event)
{
	PIXBeginEvent(0, event.c_str());
}


void EndEvent()
{
	PIXEndEvent();
}


void SetMarker(const std::string& marker)
{
	PIXSetMarker(0, marker.c_str());
}

} // namespace Luna