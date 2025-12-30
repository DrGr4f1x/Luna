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
#include "Shared.h"
#include "MeshletUtils.hlsli"


struct Vertex
{
    float3 Position;
    float3 Normal;
};


struct VertexOut
{
    float4 PositionHS : SV_Position;
    float3 PositionVS : POSITION0;
    float3 Normal : NORMAL0;
    uint MeshletIndex : COLOR0;
};


struct Payload
{
    uint MeshletIndices[AS_GROUP_SIZE];
};


ConstantBuffer<Constants> Constants : register(b0 VK_DESCRIPTOR_SET(0));
ConstantBuffer<MeshInfo> MeshInfo : register(b1 VK_DESCRIPTOR_SET(0));
ConstantBuffer<Instance> Instance : register(b2 VK_DESCRIPTOR_SET(0));


// TODO - if I set the texture registers to start with t0 (which should be fine),
// it doesn't work with Vulkan push descriptors (something in the RootSignature).
// Look into it.
StructuredBuffer<Vertex> Vertices : register(t3 VK_DESCRIPTOR_SET(0));
StructuredBuffer<Meshlet> Meshlets : register(t4 VK_DESCRIPTOR_SET(0));
ByteAddressBuffer UniqueVertexIndices : register(t5 VK_DESCRIPTOR_SET(0));
StructuredBuffer<uint> PrimitiveIndices : register(t6 VK_DESCRIPTOR_SET(0));
StructuredBuffer<CullData> MeshletCullData : register(t7 VK_DESCRIPTOR_SET(0));


// Rotates a vector, v0, about an axis by some angle
float3 RotateVector(float3 v0, float3 axis, float angle)
{
    float cs = cos(angle);
    return cs * v0 + sin(angle) * cross(axis, v0) + (1 - cs) * dot(axis, v0) * axis;
}