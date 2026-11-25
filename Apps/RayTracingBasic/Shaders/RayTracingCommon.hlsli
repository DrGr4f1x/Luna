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

struct SceneConstantBuffer
{
    float4x4 projectionToWorld;
    float4 cameraPosition;
};


struct Payload
{
    float3 hitValue;
};


struct Attributes
{
    float2 bary;
};


RaytracingAccelerationStructure scene : BINDING(t0, 0);
RWTexture2D<float4> renderTarget : BINDING(u0, 1);
ConstantBuffer<SceneConstantBuffer> sceneConstants : BINDING(b0, 1);

