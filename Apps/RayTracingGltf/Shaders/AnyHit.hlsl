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

[shader("anyhit")]
void anyhit_main(inout Payload payload, in Attributes attribs)
{
    uint geometryIndex = NonUniformResourceIndex(GeometryIndex());
    uint primitiveIndex = NonUniformResourceIndex(PrimitiveIndex());
    
    Triangle tri = UnpackTriangle(geometryIndex, primitiveIndex, attribs);
    
    float4 color = colorTextures[geometryIndex].SampleLevel(samplerLinear, tri.uv, 0.0);
    
	// If the alpha value of the texture at the current UV coordinates is below a given threshold, we'll ignore this intersection
	// That way ray traversal will be stopped and the miss shader will be invoked
    if (color.a < 0.9)
    {
        if (rnd(payload.payloadSeed) > color.a)
        {
            IgnoreHit();
        }
    }
}