//
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
// Author:  David Elder
//

#include "RayTracingCommon.hlsli"


[shader("closesthit")]
void closesthit_main(inout Payload payload, in Attributes attributes)
{
    // Execute the callable shader indexed by the current geometry being hit
	// For our sample this means that the first callable shader in the SBT is invoked for the first triangle, the second callable shader for the second triangle, etc.
    CallData callData;
    CallShader(GeometryIndex(), callData);
    payload.hitValue = callData.color;
}