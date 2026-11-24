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

    uint seed = tea(LaunchID.y * LaunchSize.x + LaunchID.x, frame);

    float r1 = rnd(seed);
    float r2 = rnd(seed);

    // Subpixel jitter: send the ray through a different position inside the pixel
    // each time, to provide antialiasing.
    float2 subpixel_jitter = frame == 0 ? float2(0.5f, 0.5f) : float2(r1, r2);
    const float2 pixelCenter = float2(LaunchID.xy) + subpixel_jitter;
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

    Payload payload;
    payload.hitValue = float3(0.0, 0.0, 0.0);
    float3 hitValues = float3(0.0, 0.0, 0.0);
    payload.recursionDepth = 1;

    const int samples = 4;

    // Trace multiple rays for e.g. transparency
    for (int smpl = 0; smpl < samples; smpl++)
    {
        payload.payloadSeed = tea(LaunchID.y * LaunchSize.x + LaunchID.x, frame);
        TraceRay(rs, RAY_FLAG_NONE, 0xff, 0, 0, 0, rayDesc, payload);
        hitValues += payload.hitValue;
    }

    float3 hitVal = hitValues / float(samples);

#if 1
    if (frame > 0)
    {
        float a = 1.0f / float(frame + 1);
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