//
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
// Author:  David Elder
//

#define INPUT_PATCH_SIZE 3
#define OUTPUT_PATCH_SIZE 3

struct DSConstantInput
{
    float edgeTesselation[3] : SV_TessFactor;
    float insideTesselation[1] : SV_InsideTessFactor;
};


struct DSInput
{
    float4 pos : BEZIERPOS;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD;
};


struct DSOutput
{
    float4 pos : SV_Position;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD0;
    float3 eyePos : TEXCOORD1;
    float3 lightVec : TEXCOORD2;
};


[[vk::binding(0, 1)]]
cbuffer DSConstants : register(b0)
{
    float4x4 projectionMatrix;
    float4x4 modelMatrix;
    float4 lightPos;
    float tessAlpha;
    float tessStrength;
};

[[vk::binding(1, 1)]]
Texture2D displacementMap : register(t0);
[[vk::binding(0, 3)]]
SamplerState linearSampler : register(s0);

[domain("tri")]
DSOutput main(DSConstantInput constInput, float3 triCoords : SV_DomainLocation, const OutputPatch<DSInput, OUTPUT_PATCH_SIZE> patch)
{
    DSOutput output = (DSOutput) 0;

    float4 pos = (triCoords.x * patch[0].pos) + (triCoords.y * patch[1].pos) + (triCoords.z * patch[2].pos);
    output.uv = (triCoords.x * patch[0].uv) + (triCoords.y * patch[1].uv) + (triCoords.z * patch[2].uv);
    output.normal = (triCoords.x * patch[0].normal) + (triCoords.y * patch[1].normal) + (triCoords.z * patch[2].normal);

    float displacement = displacementMap.SampleLevel(linearSampler, output.uv, 0.0f).a;
    pos.xyz += normalize(output.normal) * max(displacement, 0.0f) * tessStrength;

    output.eyePos = pos.xyz;
    output.lightVec = normalize(lightPos.xyz - output.eyePos);

    output.pos = mul(projectionMatrix, mul(modelMatrix, pos));

    return output;
}