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
void closesthit_main(inout Payload payload, in ProceduralPrimitiveAttributes attr)
{
    Sphere sphere = spheres[PrimitiveIndex()];

    float3 worldPos = WorldRayOrigin() + WorldRayDirection() * RayTCurrent();
    float3 worldNormal = normalize(worldPos - sphere.center);

    // Basic lighting
    float3 lightVector = normalize(lightPos.xyz);
    float dot_product = max(dot(lightVector, worldNormal), 0.2);
    payload.hitValue = sphere.color.rgb * dot_product;
}