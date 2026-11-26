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

struct PSInput
{
    float4 position : SV_Position;
    float3 normal : NORMAL;
    float3 color : COLOR;
    float3 viewVec : TEXCOORD0;
    float3 lightVec : TEXCOORD1;
    float3 worldPos : TEXCOORD2;
};


RaytracingAccelerationStructure scene : BINDING(t0, 1);

#define ambient 0.1


float4 main(PSInput input) : SV_TARGET
{
    float3 N = normalize(input.normal);
    float3 L = normalize(input.lightVec);
    float3 V = normalize(input.viewVec);
    float3 R = normalize(-reflect(L, N));
    float3 diffuse = max(dot(N, L), ambient) * input.color;

    float4 color = float4(diffuse, 1.0);

    RayDesc rayDesc;
    rayDesc.Origin = input.worldPos;
    rayDesc.Direction = L;
    rayDesc.TMin = 0.01;
    rayDesc.TMax = 1000.0;

    RayQuery<RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH> rayQuery;
    rayQuery.TraceRayInline(scene, RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH, 0xFF, rayDesc);

    // Traverse the acceleration structure and store information about the first intersection (if any)
    rayQuery.Proceed();

    // If the intersection has hit a triangle, the fragment is shadowed
    if (rayQuery.CommittedStatus() == COMMITTED_TRIANGLE_HIT)
    {
        color.rgb *= 0.1;
    }

    return color;
}