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


Texture2D textureColor : BINDING(t0, 1);
Texture2D textureNormalMap : BINDING(t1, 1);
SamplerState samplerLinear : BINDING(s0, 2);


struct VSOutput
{
    float4 pos          : SV_POSITION;
    float3 worldPos     : POSITION;
    float2 uv           : TEXCOORD;
    float4 color        : COLOR;
    float3 normal       : NORMAL;
    float3 tangent      : TANGENT;
};


struct FSOutput
{
    float4 position     : SV_TARGET0;
    float4 normal       : SV_TARGET1;
    float4 albedo       : SV_TARGET2;
};


FSOutput main(VSOutput input)
{
    FSOutput output = (FSOutput) 0;
    output.position = float4(input.worldPos, 1.0);

	// Calculate normal in tangent space
    float3 N = normalize(input.normal);
    float3 T = normalize(input.tangent);
    float3 B = cross(N, T);
    float3x3 TBN = float3x3(T, B, N);
    float3 tnorm = mul(normalize(textureNormalMap.Sample(samplerLinear, input.uv).xyz * 2.0 - float3(1.0, 1.0, 1.0)), TBN);
    output.normal = float4(tnorm, 1.0);

    output.albedo = textureColor.Sample(samplerLinear, input.uv);
    return output;
}