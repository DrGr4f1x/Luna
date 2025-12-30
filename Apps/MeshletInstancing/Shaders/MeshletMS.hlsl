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


struct Constants
{
    float4x4 View;
    float4x4 ViewProj;
    uint DrawMeshlets;
};



struct Vertex
{
    float3 Position;
    float3 Normal;
};


struct Meshlet
{
    uint VertCount;
    uint VertOffset;
    uint PrimCount;
    uint PrimOffset;
};


struct Instance
{
    float4x4 World;
    float4x4 WorldInvTranspose;
};


struct VertexOut
{
    float4 PositionHS : SV_Position;
    float3 PositionVS : POSITION0;
    float3 Normal : NORMAL0;
    uint MeshletIndex : COLOR0;
};


ConstantBuffer<Constants> Globals : register(b0 VK_DESCRIPTOR_SET(0));


struct DrawMeshParams
{
    uint InstanceCount;
    uint InstanceOffset;
    uint IndexBytes;
    uint MeshletCount;
    uint MeshletOffset;
};


[[vk::push_constant]]
ConstantBuffer<DrawMeshParams> DrawParams : register(b1); // Root parameter 1


uint GetInstanceCount()
{
    return DrawParams.InstanceCount;
}


uint GetInstanceOffset()
{
    return DrawParams.InstanceOffset;
}


uint GetIndexBytes()
{
    return DrawParams.IndexBytes;
}


uint GetMeshletCount()
{
    return DrawParams.MeshletCount;
}


uint GetMeshletOffset()
{
    return DrawParams.MeshletOffset;
}


StructuredBuffer<Vertex> Vertices : register(t0 VK_DESCRIPTOR_SET(2));
StructuredBuffer<Meshlet> Meshlets : register(t1 VK_DESCRIPTOR_SET(2));
ByteAddressBuffer UniqueVertexIndices : register(t2 VK_DESCRIPTOR_SET(2));
StructuredBuffer<uint> PrimitiveIndices : register(t3 VK_DESCRIPTOR_SET(2));
StructuredBuffer<Instance> Instances : register(t4 VK_DESCRIPTOR_SET(3));


/////
// Data Loaders

uint3 UnpackPrimitive(uint primitive)
{
    // Unpacks a 10 bits per index triangle from a 32-bit uint.
    return uint3(primitive & 0x3FF, (primitive >> 10) & 0x3FF, (primitive >> 20) & 0x3FF);
}

uint3 GetPrimitive(Meshlet m, uint index)
{
    return UnpackPrimitive(PrimitiveIndices[m.PrimOffset + index]);
}

uint GetVertexIndex(Meshlet m, uint localIndex)
{
    localIndex = m.VertOffset + localIndex;

    if (GetIndexBytes() == 4) // 32-bit Vertex Indices
    {
        return UniqueVertexIndices.Load(localIndex * 4);
    }
    else // 16-bit Vertex Indices
    {
        // Byte address must be 4-byte aligned.
        uint wordOffset = (localIndex & 0x1);
        uint byteOffset = (localIndex / 2) * 4;

        // Grab the pair of 16-bit indices, shift & mask off proper 16-bits.
        uint indexPair = UniqueVertexIndices.Load(byteOffset);
        uint index = (indexPair >> (wordOffset * 16)) & 0xffff;

        return index;
    }
}

VertexOut GetVertexAttributes(uint meshletIndex, uint vertexIndex, uint instanceIndex)
{
    Instance n = Instances[GetInstanceOffset() + instanceIndex];
    Vertex v = Vertices[vertexIndex];

    float4 positionWS = mul(n.World, float4(v.Position, 1));

    VertexOut vout;
    vout.PositionVS = mul(Globals.View, positionWS).xyz;
    vout.PositionHS = mul(Globals.ViewProj, positionWS);
    vout.Normal = mul(n.WorldInvTranspose, float4(v.Normal, 0)).xyz;
    vout.MeshletIndex = meshletIndex;

    return vout;
}


[NumThreads(128, 1, 1)]
[OutputTopology("triangle")]
void main(
    uint gtid : SV_GroupIndex,
    uint gid : SV_GroupID,
    out vertices VertexOut verts[MAX_VERTS],
    out indices uint3 tris[MAX_PRIMS]
)
{
    uint meshletIndex = gid / GetInstanceCount();
    Meshlet m = Meshlets[meshletIndex];

    // Determine instance count - only 1 instance per threadgroup in the general case
    uint startInstance = gid % GetInstanceCount();
    uint instanceCount = 1;

    // Last meshlet in mesh may be be packed - multiple instances submitted by a single threadgroup.
    if (meshletIndex == GetMeshletCount() - 1)
    {
        const uint instancesPerGroup = min(MAX_VERTS / m.VertCount, MAX_PRIMS / m.PrimCount);

        // Determine how many packed instances there are in this group
        uint unpackedGroupCount = (GetMeshletCount() - 1) * GetInstanceCount();
        uint packedIndex = gid - unpackedGroupCount;

        startInstance = packedIndex * instancesPerGroup;
        instanceCount = min(GetInstanceCount() - startInstance, instancesPerGroup);
    }

    // Compute our total vertex & primitive counts
    uint vertCount = m.VertCount * instanceCount;
    uint primCount = m.PrimCount * instanceCount;

    SetMeshOutputCounts(vertCount, primCount);

    //--------------------------------------------------------------------
    // Export Primitive & Vertex Data

    if (gtid < vertCount)
    {
        uint readIndex = gtid % m.VertCount; // Wrap our reads for packed instancing.
        uint instanceId = gtid / m.VertCount; // Instance index into this threadgroup's instances (only non-zero for packed threadgroups.)

        uint vertexIndex = GetVertexIndex(m, readIndex);
        uint instanceIndex = startInstance + instanceId;

        verts[gtid] = GetVertexAttributes(meshletIndex, vertexIndex, instanceIndex);
    }

    if (gtid < primCount)
    {
        uint readIndex = gtid % m.PrimCount; // Wrap our reads for packed instancing.
        uint instanceId = gtid / m.PrimCount; // Instance index within this threadgroup (only non-zero in last meshlet threadgroups.)

        // Must offset the vertex indices to this thread's instanced verts
        tris[gtid] = GetPrimitive(m, readIndex) + (m.VertCount * instanceId);
    }
}