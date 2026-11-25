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

struct SceneConstants
{
    float4x4 projectionToWorld;
    float3 cameraPosition;
    float4 lightPos;
};
ConstantBuffer<SceneConstants> sceneConstants : BINDING(b2, 1);


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