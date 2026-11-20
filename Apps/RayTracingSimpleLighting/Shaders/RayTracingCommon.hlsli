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


struct Vertex
{
    float3 position;
    float3 normal;
};

struct SceneConstantBuffer
{
    float4x4 projectionToWorld;
    float4 cameraPosition;
    float4 lightPosition;
    float4 lightAmbientColor;
    float4 lightDiffuseColor;
};

struct CubeConstantBuffer
{
    float4 albedo;
};

struct RayPayload
{
    float4 color;
};


RaytracingAccelerationStructure scene : BINDING(t0, 1);
RWTexture2D<float4> renderTarget : BINDING(u0, 0);
ByteAddressBuffer indexBuffer : BINDING(t1, 3);
StructuredBuffer<Vertex> vertexBuffer : BINDING(t2, 3);

ConstantBuffer<SceneConstantBuffer> sceneConstantBuffer : BINDING(b0, 2);
ConstantBuffer<CubeConstantBuffer> cubeConstantBuffer : BINDING(b1, 0);