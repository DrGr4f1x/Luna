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


Texture2D textures[] : register(t0 VK_DESCRIPTOR_SET(1));
SamplerState samplerColorMap : register(s0 VK_DESCRIPTOR_SET(2));


struct PSInput
{
    float4 pos          : SV_POSITION;
    float2 uv           : TEXCOORD;
    int textureIndex    : TEXTUREINDEX;
};


float4 main(PSInput input) : SV_TARGET
{
    return textures[NonUniformResourceIndex(input.textureIndex)].Sample(samplerColorMap, input.uv);
}