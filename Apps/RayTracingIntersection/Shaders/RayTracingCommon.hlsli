//
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
// Author:  David Elder
//

#include "Common.hlsli"

RaytracingAccelerationStructure rs : BINDING(t0, 0);
RWTexture2D<float4> image : BINDING(u1, 1);

cbuffer CameraProperties : BINDING(b2, 1)
{
    float4x4 viewInverse;
    float4x4 projInverse;
    float4 lightPos;
};

struct Sphere
{
    float3 center;
    float radius;
    float4 color;
};
StructuredBuffer<Sphere> spheres : BINDING(t1, 1);

struct Payload
{
    float3 hitValue;
};

struct ProceduralPrimitiveAttributes
{
    float dummy;
};