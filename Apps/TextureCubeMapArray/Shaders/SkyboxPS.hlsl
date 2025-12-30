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


TextureCubeArray texCubeArray : BINDING(t0, 1);
SamplerState samplerLinear : BINDING(s0, 2);


cbuffer PSConstants : BINDING(b0, 1)
{
    float lodBias;
    int arraySlice;
};


float4 main(PSInput input) : SV_Target
{
    return texCubeArray.Sample(samplerLinear, float4(input.uvw, arraySlice));
}