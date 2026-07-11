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

Buffer<float3> vertices : BINDING(t0, 0);
Buffer<uint> indices    : BINDING(t1, 1);

struct Edge
{
    float3 p0;
    float3 p1;
};

AppendStructuredBuffer<Edge> output : BINDING(u0, 0);

struct Constants_t
{
    float4x4 modelMatrix;
    float4 plane;
};

ConstantBuffer<Constants_t> Constants : BINDING(b0, 0);

struct IndexBufferConstants_t
{
    uint numIndices;
};

[[vk::push_constant]]
ConstantBuffer<IndexBufferConstants_t> IndexBuffer : register(b1);


float3 EmitVertex(float d0, float3 pt0, float d1, float3 pt1)
{
    float c = d0 / (d0 - d1);
    float3 intpos = lerp(pt0, pt1, c);
    
    return intpos;
}


[numthreads(64, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    uint i0 = DTid.x;
    
    if (i0 >= IndexBuffer.numIndices)
        return;
    
    uint i1 = i0 + 1;
    uint i2 = i0 + 2;
    
    float3 worldPos[3];
    worldPos[0] = mul(Constants.modelMatrix, float4(vertices[i0], 1.0));
    worldPos[1] = mul(Constants.modelMatrix, float4(vertices[i1], 1.0));
    worldPos[2] = mul(Constants.modelMatrix, float4(vertices[i2], 1.0));
    
    float d[3];
    d[0] = dot(worldPos[0], Constants.plane.xyz) + Constants.plane.w;
    d[1] = dot(worldPos[1], Constants.plane.xyz) + Constants.plane.w;
    d[2] = dot(worldPos[2], Constants.plane.xyz) + Constants.plane.w;
    
    // In this bitmask a 1 means the corresponding vertex is above the plane, 0 means below
    uint bitmask = 0u;
    
    if (d[0] > 0.0)
    {
        bitmask |= 1 << 0;
    }
    if (d[1] > 0.0)
    {
        bitmask |= 1 << 1;
    }
    if (d[2] > 0.0)
    {
        bitmask |= 1 << 2;
    }
    
    // If the bitmask is 0 the triangle is entirely below the plane (7 means entirely above plane)
    if (bitmask == 0 || bitmask == 7)
        return;
    
    Edge edge = (Edge) 0;
    
    // In these case blocks, the convention is the first vertex is above the plane, and the second is below.
    // We assume CCW winding order.  Would need to reverse for CW.
    switch (bitmask)
    {
        case 1:
            edge.p0 = EmitVertex(d[0], worldPos[0], d[1], worldPos[1]);
            edge.p1 = EmitVertex(d[0], worldPos[0], d[2], worldPos[2]);
            break;
    
        case 2:
            edge.p0 = EmitVertex(d[1], worldPos[1], d[2], worldPos[2]);
            edge.p1 = EmitVertex(d[1], worldPos[1], d[0], worldPos[0]);
            break;
        
        case 3:
            edge.p0 = EmitVertex(d[1], worldPos[1], d[2], worldPos[2]);
            edge.p1 = EmitVertex(d[0], worldPos[0], d[2], worldPos[2]);
            break;
        
        case 4:
            edge.p0 = EmitVertex(d[2], worldPos[2], d[0], worldPos[0]);
            edge.p1 = EmitVertex(d[2], worldPos[2], d[1], worldPos[1]);
            break;
        
        case 5:
            edge.p0 = EmitVertex(d[0], worldPos[0], d[1], worldPos[1]);
            edge.p1 = EmitVertex(d[2], worldPos[2], d[1], worldPos[1]);
            break;
        
        case 6:
            edge.p0 = EmitVertex(d[2], worldPos[2], d[0], worldPos[0]);
            edge.p1 = EmitVertex(d[1], worldPos[1], d[0], worldPos[0]);
            break;
    }
    
    output.Append(edge);
}