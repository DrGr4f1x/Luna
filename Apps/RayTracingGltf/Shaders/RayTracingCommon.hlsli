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

#define MAX_RECURSION_DEPTH 4

struct Payload
{
    float3 hitValue;
    uint payloadSeed;
    uint recursionDepth;
    bool shadowed;
};

struct Vertex
{
    float3 pos;
    float3 normal;
    float2 uv;
};

struct Triangle
{
    Vertex vertices[3];
    float3 normal;
    float2 uv;
};

struct Attributes
{
    float2 bary;
};


RaytracingAccelerationStructure rs : BINDING(t0, 0);
RWTexture2D<float4> image : BINDING(u0, 1);

cbuffer SceneConstants : BINDING(b0, 1)
{
    float4x4 viewInverse;
    float4x4 projInverse;
    uint frame;
};

StructuredBuffer<Vertex> vertexBuffers[] : BINDING_SPACE(t0, 2);
StructuredBuffer<uint> indexBuffers[] : BINDING_SPACE(t1, 3);
Texture2D<float4> colorTextures[] : BINDING_SPACE(t2, 4);
Texture2D<float4> occlusionTextures[] : BINDING_SPACE(t3, 5);
SamplerState samplerLinear : BINDING(s0, 6);


// Tiny Encryption Algorithm
// By Fahad Zafar, Marc Olano and Aaron Curtis, see https://www.highperformancegraphics.org/previous/www_2010/media/GPUAlgorithms/HPG2010_GPUAlgorithms_Zafar.pdf
uint tea(uint val0, uint val1)
{
    uint sum = 0;
    uint v0 = val0;
    uint v1 = val1;
    for (uint n = 0; n < 16; n++)
    {
        sum += 0x9E3779B9;
        v0 += ((v1 << 4) + 0xA341316C) ^ (v1 + sum) ^ ((v1 >> 5) + 0xC8013EA4);
        v1 += ((v0 << 4) + 0xAD90777D) ^ (v0 + sum) ^ ((v0 >> 5) + 0x7E95761E);
    }
    return v0;
}


// Linear congruential generator based on the previous RNG state
// See https://en.wikipedia.org/wiki/Linear_congruential_generator
uint lcg(inout uint previous)
{
    const uint multiplier = 1664525u;
    const uint increment = 1013904223u;
    previous = (multiplier * previous + increment);
    return previous & 0x00FFFFFF;
}


// Generate a random float in [0, 1) given the previous RNG state
float rnd(inout uint previous)
{
    return (float(lcg(previous)) / float(0x01000000));
}


// This function will unpack our vertex buffer data into a single triangle and calculates uv coordinates
Triangle UnpackTriangle(uint geometryIndex, uint primitiveIndex, Attributes attribs)
{
    StructuredBuffer<Vertex> vertices = vertexBuffers[geometryIndex];
    StructuredBuffer<uint> indices = indexBuffers[geometryIndex];
    
    Triangle tri;
    tri.vertices[0] = vertices[indices[3 * primitiveIndex + 0]];
    tri.vertices[1] = vertices[indices[3 * primitiveIndex + 1]];
    tri.vertices[2] = vertices[indices[3 * primitiveIndex + 2]];
    
    // Calculate values at barycentric coordinates
    float3 barycentricCoords = float3(1.0f - attribs.bary.x - attribs.bary.y, attribs.bary.x, attribs.bary.y);
    tri.uv = tri.vertices[0].uv * barycentricCoords.x + tri.vertices[1].uv * barycentricCoords.y + tri.vertices[2].uv * barycentricCoords.z;
    tri.normal = tri.vertices[0].normal * barycentricCoords.x + tri.vertices[1].normal * barycentricCoords.y + tri.vertices[2].normal * barycentricCoords.z;
    return tri;
}