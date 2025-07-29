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
    float4 pos  : SV_POSITION;
    float2 uv   : TEXCOORD;
};


float4 main(PSInput input) : SV_TARGET
{
    const float4 gradientStart = float4(0.93, 0.9, 0.81, 1.0);
    const float4 gradientEnd = float4(0.35, 0.5, 1.0, 1.0);
    return lerp(gradientStart, gradientEnd, min(0.5 - (input.uv.y + 0.05), 0.5) / 0.15 + 0.5);
}