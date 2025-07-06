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
    float4 pos : SV_Position;
    float3 uvw : TEXCOORD;
};


VK_BINDING(1, 1)
TextureCubeArray texCubeArray : register(t1);

VK_BINDING(0, 2)
SamplerState samplerLinear : register(s0);

VK_BINDING(0, 1)
cbuffer PSConstants : register(b0)
{
    float lodBias;
    int arraySlice;
};


float4 main(PSInput input) : SV_Target
{
    return texCubeArray.Sample(samplerLinear, float4(input.uvw, arraySlice));
}