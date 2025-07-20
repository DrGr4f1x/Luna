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
    float2 uv : TEXCOORD;
};


[[vk::binding(0)]]
Texture2D colorTex : register(t0);
[[vk::binding(0, 1)]]
SamplerState linearSampler : register(s0);


[[vk::binding(1)]]
cbuffer PSConstants : register(b0)
{
    float blurScale;
    float blurStrength;
    int blurDirection;
};


float4 main(PSInput input) : SV_Target
{
    float weight[5];
    weight[0] = 0.227027;
    weight[1] = 0.1945946;
    weight[2] = 0.1216216;
    weight[3] = 0.054054;
    weight[4] = 0.016216;

    uint2 texDim;
    colorTex.GetDimensions(texDim.x, texDim.y);

    float2 texOffset = 1.0 / float2(texDim) * blurScale; // gets size of single texel
    float3 result = colorTex.Sample(linearSampler, input.uv).rgb * weight[0]; // current fragment's contribution

    for (int i = 1; i < 5; ++i)
    {
        if (blurDirection == 1)
        {
			// H
            result += colorTex.Sample(linearSampler, input.uv + float2(texOffset.x * i, 0.0)).rgb * weight[i] * blurStrength;
            result += colorTex.Sample(linearSampler, input.uv - float2(texOffset.x * i, 0.0)).rgb * weight[i] * blurStrength;
        }
        else
        {
			// V
            result += colorTex.Sample(linearSampler, input.uv + float2(0.0, texOffset.y * i)).rgb * weight[i] * blurStrength;
            result += colorTex.Sample(linearSampler, input.uv - float2(0.0, texOffset.y * i)).rgb * weight[i] * blurStrength;
        }
    }

    return float4(result, 1.0);
}