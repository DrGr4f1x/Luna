//
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
// Author:  David Elder
//

struct PSInput
{
    float4 pos : SV_Position;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD0;
    float3 eyePos : TEXCOORD1;
    float3 lightVec : TEXCOORD2;
};


[[vk::binding(0, 2)]]
Texture2D colorTex : register(t0);
[[vk::binding(0, 4)]]
SamplerState linearSampler : register(s0);


float4 main(PSInput input) : SV_Target
{
    float4 ambient = float4(0.1.xxx, 1.0);
    float4 diffuse = float4(0.9.xxx, 1.0) * max(dot(input.normal, input.lightVec), 0.0f);

    return (ambient + diffuse) * colorTex.Sample(linearSampler, input.uv);
}