//
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
// Author:  David Elder
//

[[vk::binding(0, 1)]]
Texture2D textureColor : register(t0);
[[vk::binding(0, 2)]]
SamplerState samplerColor : register(s0);


struct PSInput
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD;
};


float4 main(PSInput input) : SV_TARGET
{
    float4 color = textureColor.Sample(samplerColor, input.uv);
    return float4(color.rgb, 1.0);
}