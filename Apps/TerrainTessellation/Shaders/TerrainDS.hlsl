//
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
// Author:  David Elder
//

#define INPUT_PATCH_SIZE 4
#define OUTPUT_PATCH_SIZE 4

struct DSInput
{
    float4 pos : BEZIERPOS;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD;
};


struct DSConstantInput
{
    float edgeTesselation[4] : SV_TessFactor;
    float insideTesselation[2] : SV_InsideTessFactor;
};


struct DSOutput
{
    float4 pos : SV_Position;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD0;
    float3 lightVec : TEXCOORD1;
};

[[vk::binding(0, 2)]]
cbuffer DSConstants : register(b0)
{
    float4x4 projectionMatrix;
    float4x4 modelViewMatrix;
    float4 lightPos;
    float4 frustumPlanes[6];
    float2 viewportDim;
    float displacementFactor;
    float tessellationFactor;
    float tessellatedEdgeSize;
};

[[vk::binding(1, 2)]]
Texture2D heightTex : register(t1);

[[vk::binding(0, 4)]]
SamplerState linearSamplerMirror : register(s0);


[domain("quad")]
DSOutput main(DSConstantInput constInput, float3 triCoords : SV_DomainLocation, const OutputPatch<DSInput, OUTPUT_PATCH_SIZE> patch)
{
    DSOutput output = (DSOutput) 0;

	// Interpolate UV coordinates
    float2 uv1 = lerp(patch[0].uv, patch[1].uv, triCoords.x);
    float2 uv2 = lerp(patch[3].uv, patch[2].uv, triCoords.x);
    output.uv = lerp(uv1, uv2, triCoords.y);

	// Interpolate normals
    float3 n1 = lerp(patch[0].normal, patch[1].normal, triCoords.x);
    float3 n2 = lerp(patch[3].normal, patch[2].normal, triCoords.x);
    output.normal = lerp(n1, n2, triCoords.y);

	// Interpolate positions
    float4 pos1 = lerp(patch[0].pos, patch[1].pos, triCoords.x);
    float4 pos2 = lerp(patch[3].pos, patch[2].pos, triCoords.x);
    float4 pos = lerp(pos1, pos2, triCoords.y);

	// Displace
    pos.y -= heightTex.SampleLevel(linearSamplerMirror, output.uv, 0.0).r * displacementFactor;
	// Perspective projection
    output.pos = mul(projectionMatrix, mul(modelViewMatrix, pos));

	// Calculate vectors for lighting based on tessellated position
    float3 viewVec = -pos.xyz;
    output.lightVec = normalize(lightPos.xyz + viewVec);

    return output;
}