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
#include "Payload.hlsli"

RaytracingAccelerationStructure rs : BINDING(t0, 0);
RWTexture2D<float4> image : BINDING(u1, 1);

struct SceneConstants
{
    float4x4 projectionToWorld;
    float3 cameraPosition;
    float3 lightPosition;
    int vertexSize;
};

ConstantBuffer<SceneConstants> sceneConstants : BINDING(b2, 1);