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


ConstantBuffer<Constants> Constants : BINDING(b0, 0);
ConstantBuffer<MeshInfo> MeshInfo : BINDING(b1, 0);
ConstantBuffer<Instance> Instance : BINDING(b2, 0);


// TODO - if I set the texture registers to start with t0 (which should be fine),
// it doesn't work with Vulkan push descriptors (something in the RootSignature).
// Look into it.
StructuredBuffer<Vertex> Vertices : BINDING(t3, 0);
StructuredBuffer<Meshlet> Meshlets : BINDING(t4, 0);
ByteAddressBuffer UniqueVertexIndices : BINDING(t5, 0);
StructuredBuffer<uint> PrimitiveIndices : BINDING(t6, 0);
StructuredBuffer<CullData> MeshletCullData : BINDING(t7, 0);


// Rotates a vector, v0, about an axis by some angle
float3 RotateVector(float3 v0, float3 axis, float angle)
{
    float cs = cos(angle);
    return cs * v0 + sin(angle) * cross(axis, v0) + (1 - cs) * dot(axis, v0) * axis;
}