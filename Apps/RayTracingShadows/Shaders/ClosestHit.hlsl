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

struct Vertex
{
    float3 pos;
    float3 normal;
    float4 color;
    float2 uv;
};

StructuredBuffer<Vertex> vertexBuffers[] : BINDING_SPACE(t1, 2);
StructuredBuffer<uint> indexBuffers[] : BINDING_SPACE(t2, 3);

struct Attributes
{
    float2 bary;
};


Vertex UnpackVertex(uint index)
{
    uint instance = NonUniformResourceIndex(InstanceID());
    StructuredBuffer<Vertex> vertices = vertexBuffers[instance];
    return vertices[index];
}


[shader("closesthit")]
void closesthit_main(inout Payload payload, in Attributes attribs)
{
    uint PrimitiveID = PrimitiveIndex();
    
    uint instance = NonUniformResourceIndex(InstanceID());
    StructuredBuffer<uint> indices = indexBuffers[instance];
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

    ShadowRayPayload shadowPayload = { true };
	// Offset indices to match shadow hit/miss index
    TraceRay(rs, RAY_FLAG_CULL_BACK_FACING_TRIANGLES | RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH | RAY_FLAG_FORCE_OPAQUE | RAY_FLAG_SKIP_CLOSEST_HIT_SHADER, 0xff, 0, 0, 1, rayDesc, shadowPayload);
    if (shadowPayload.hit)
    {
        payload.hitValue *= 0.3;
    }
}