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


[[vk::binding(0, 1)]]
Texture2D colorTex : register(t0);

[[vk::binding(0, 2)]]
SamplerState linearSampler : register(s0);


float4 main(PSInput input) : SV_Target
{
    return colorTex.Sample(linearSampler, input.uv);
}