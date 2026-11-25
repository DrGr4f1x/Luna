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


inline void GenerateCameraRay(uint2 index, out float3 origin, out float3 direction)
{
    uint3 LaunchSize = DispatchRaysDimensions();
    uint seed = tea(index.y * LaunchSize.x + index.x, sceneConstants.frame);

    float r1 = rnd(seed);
    float r2 = rnd(seed);
    
    float2 subpixel_jitter = sceneConstants.frame == 0 ? float2(0.5f, 0.5f) : float2(r1, r2);
    
    float2 xy = float2(index) + subpixel_jitter;
    float2 screenPos = xy / DispatchRaysDimensions().xy * 2.0 - 1.0;

    // Invert Y for DirectX-style coordinates.
    screenPos.y = -screenPos.y;

    // Unproject the pixel coordinate into a ray.
    float4 world = mul(sceneConstants.projectionToWorld, float4(screenPos, 0, 1));

    world.xyz /= world.w;
    origin = sceneConstants.cameraPosition.xyz;
    direction = normalize(world.xyz - origin);
}


[shader("raygeneration")]
void raygen_main()
{
    uint3 LaunchID = DispatchRaysIndex();
    uint3 LaunchSize = DispatchRaysDimensions();

    float3 origin;
    float3 direction;
    GenerateCameraRay(LaunchID.xy, origin, direction);
    
    RayDesc rayDesc;
    rayDesc.Origin = origin;
    rayDesc.Direction = direction;
    rayDesc.TMin = 0.001;
    rayDesc.TMax = 10000.0;

    Payload payload;
    payload.hitValue = float3(0.0, 0.0, 0.0);
    float3 hitValues = float3(0.0, 0.0, 0.0);
    payload.recursionDepth = 1;

    const int samples = 4;

    // Trace multiple rays for e.g. transparency
    for (int smpl = 0; smpl < samples; smpl++)
    {
        payload.payloadSeed = tea(LaunchID.y * LaunchSize.x + LaunchID.x, sceneConstants.frame);
        TraceRay(scene, RAY_FLAG_NONE, 0xff, 0, 0, 0, rayDesc, payload);
        hitValues += payload.hitValue;
    }

    float3 hitVal = hitValues / float(samples);

#if 1
    if (sceneConstants.frame > 0)
    {
        float a = 1.0f / float(sceneConstants.frame + 1);
        float3 old_color = image[int2(LaunchID.xy)].xyz;
        image[int2(LaunchID.xy)] = float4(lerp(old_color, hitVal, a), 1.0f);
    }
    else
    {
        // First frame, replace the value in the buffer
        image[int2(LaunchID.xy)] = float4(hitVal, 1.0f);
    }
#else
    // First frame, replace the value in the buffer
    image[int2(LaunchID.xy)] = float4(hitVal, 1.0f);
#endif
}