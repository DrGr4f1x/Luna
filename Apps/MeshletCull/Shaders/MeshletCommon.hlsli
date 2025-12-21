//
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
// Author:  David Elder
//

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

[[vk::binding(0, 0)]]
ConstantBuffer<Constants> Constants : register(b0);
[[vk::binding(1, 0)]]
ConstantBuffer<MeshInfo> MeshInfo : register(b1);
[[vk::binding(2, 0)]]
ConstantBuffer<Instance> Instance : register(b2);

[[vk::binding(3, 0)]]
StructuredBuffer<Vertex> Vertices : register(t3);
[[vk::binding(4, 0)]]
StructuredBuffer<Meshlet> Meshlets : register(t4);
[[vk::binding(5, 0)]]
ByteAddressBuffer UniqueVertexIndices : register(t5);
[[vk::binding(6, 0)]]
StructuredBuffer<uint> PrimitiveIndices : register(t6);
[[vk::binding(7, 0)]]
StructuredBuffer<CullData> MeshletCullData : register(t7);


// Rotates a vector, v0, about an axis by some angle
float3 RotateVector(float3 v0, float3 axis, float angle)
{
    float cs = cos(angle);
    return cs * v0 + sin(angle) * cross(axis, v0) + (1 - cs) * dot(axis, v0) * axis;
}