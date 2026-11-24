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
void closesthit_main(inout Payload payload, in Attributes attribs)
{
    uint geometryIndex = NonUniformResourceIndex(GeometryIndex());
    uint primitiveIndex = NonUniformResourceIndex(PrimitiveIndex());
    
    Triangle tri = UnpackTriangle(geometryIndex, primitiveIndex, attribs);

    float3 color = colorTextures[geometryIndex].SampleLevel(samplerLinear, tri.uv, 0.0).rgb;
    float occlusion = occlusionTextures[geometryIndex].SampleLevel(samplerLinear, tri.uv, 0.0).r;
    color *= occlusion;

    payload.hitValue = color;

#if 0
    // Shadow casting
    float tmin = 0.001;
    float tmax = 10000.0;
    float epsilon = 0.001;
    float3 origin = WorldRayOrigin() + WorldRayDirection() * RayTCurrent() + tri.normal * epsilon;
    payload.shadowed = true;
    float3 lightVector = float3(-5.0, -2.5, -5.0);
    // Trace shadow ray and offset indices to match shadow hit/miss shader group indices
    //	traceRayEXT(topLevelAS, gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsOpaqueEXT | gl_RayFlagsSkipClosestHitShaderEXT, 0xFF, 0, 0, 1, origin, tmin, lightVector, tmax, 2);
    //	if (shadowed) {
    //		hitValue *= 0.7;
    //	}
#endif
}