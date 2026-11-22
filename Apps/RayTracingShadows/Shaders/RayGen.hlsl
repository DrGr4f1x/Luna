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


[shader("raygeneration")]
void raygen_main()
{
    uint3 LaunchID = DispatchRaysIndex();
    uint3 LaunchSize = DispatchRaysDimensions();

    const float2 pixelCenter = float2(LaunchID.xy) + float2(0.5, 0.5);
    const float2 inUV = pixelCenter / float2(LaunchSize.xy);
    float2 d = inUV * 2.0 - 1.0;
    
    // Invert Y for DirectX-style coordinates.
    d.y = -d.y;
    
    float4 target = mul(projInverse, float4(d.x, d.y, 0, 1));

    RayDesc rayDesc;
    rayDesc.Origin = mul(viewInverse, float4(0, 0, 0, 1)).xyz;
    rayDesc.Direction = mul(viewInverse, float4(normalize(target.xyz), 0)).xyz;
    rayDesc.TMin = 0.001;
    rayDesc.TMax = 10000.0;

    Payload payload = { float4(0, 0, 0, 0) };
    TraceRay(rs, RAY_FLAG_FORCE_OPAQUE, 0xff, 0, 0, 0, rayDesc, payload);

    image[int2(LaunchID.xy)] = float4(payload.hitValue, 0.0);
}