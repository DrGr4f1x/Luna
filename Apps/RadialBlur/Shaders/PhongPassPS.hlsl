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
    float3 eyePos : TEXCOORD0;
    float3 lightDir : TEXCOORD1;
    float2 texcoord : TEXCOORD2;
};



Texture1D gradientTex : BINDING(t0, 1);
SamplerState linearSampler : BINDING(s0, 2);


float4 main(PSInput input) : SV_TARGET
{
	// No light calculations for glow color 
	// Use max. color channel value
	// to detect bright glow emitters
    if ((input.color.r >= 0.9f) || (input.color.g >= 0.9f) || (input.color.b >= 0.9f))
    {
        return gradientTex.Sample(linearSampler, input.texcoord.x);
    }
    else
    {
        float3 eye = normalize(-input.eyePos);
        float3 reflected = normalize(reflect(-input.lightDir, input.normal));

        float4 vAmbient = float4(0.2f, 0.2f, 0.2f, 1.0f);
        float4 vDiffuse = float4(0.5f, 0.5f, 0.5f, 0.5f) * max(dot(input.normal, input.lightDir), 0.0f);
        float specular = 0.25f;
        float4 vSpecular = float4(0.5f, 0.5f, 0.5f, 1.0f) * pow(max(dot(reflected, eye), 0.0), 4.0) * specular;
        return float4((vAmbient + vDiffuse) * float4(input.color, 1.0) + vSpecular);
    }
}