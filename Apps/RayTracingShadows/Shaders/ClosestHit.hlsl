//
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
// Author:  David Elder
//

#include "RayTracingCommon.hlsli"

StructuredBuffer<float4> vertices : BINDING(t1, 1);
StructuredBuffer<uint> indices : BINDING(t2, 1);

struct Attributes
{
    float2 bary;
};

struct Vertex
{
    float3 pos;
    float3 normal;
    float2 uv;
    float4 color;
    float4 _pad0;
    float4 _pad1;
};


Vertex UnpackVertex(uint index)
{
	// Unpack the vertices from the SSBO using the glTF vertex structure
	// The multiplier is the size of the vertex divided by four float components (=16 bytes)
    const int m = vertexSize / 16;

    float4 d0 = vertices[m * index + 0];
    float4 d1 = vertices[m * index + 1];
    float4 d2 = vertices[m * index + 2];

    Vertex v;
    v.pos = d0.xyz;
    v.normal = float3(d0.w, d1.x, d1.y);
    v.color = float4(d2.x, d2.y, d2.z, 1.0);

    return v;
}


[shader("closesthit")]
void closesthit_main(inout Payload payload, in Attributes attribs)
{
    uint PrimitiveID = PrimitiveIndex();
    int3 index = int3(indices[3 * PrimitiveID], indices[3 * PrimitiveID + 1], indices[3 * PrimitiveID + 2]);

    Vertex v0 = UnpackVertex(index.x);
    Vertex v1 = UnpackVertex(index.y);
    Vertex v2 = UnpackVertex(index.z);

	// Interpolate normal
    const float3 barycentricCoords = float3(1.0f - attribs.bary.x - attribs.bary.y, attribs.bary.x, attribs.bary.y);
    float3 normal = normalize(v0.normal * barycentricCoords.x + v1.normal * barycentricCoords.y + v2.normal * barycentricCoords.z);

	// Basic lighting
    float3 lightVector = normalize(lightPos.xyz);
    float dot_product = max(dot(lightVector, normal), 0.2);
    payload.hitValue = v0.color.rgb * dot_product;

    RayDesc rayDesc;
    rayDesc.Origin = WorldRayOrigin() + WorldRayDirection() * RayTCurrent();
    rayDesc.Direction = lightVector;
    rayDesc.TMin = 0.001;
    rayDesc.TMax = 100.0;

    payload.shadowed = true;
	// Offset indices to match shadow hit/miss index
    TraceRay(rs, RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH | RAY_FLAG_FORCE_OPAQUE | RAY_FLAG_SKIP_CLOSEST_HIT_SHADER, 0xff, 0, 0, 1, rayDesc, payload);
    if (payload.shadowed)
    {
        payload.hitValue *= 0.3;
    }
}