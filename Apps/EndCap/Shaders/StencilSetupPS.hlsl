//
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
// Author:  David Elder
//

Texture2D<uint> endCapMaskTex : BINDING(t0, 0);

struct PSInput
{
    float4 pos : SV_Position;
};

float4 main(PSInput input) : SV_Target
{
    int2 st = (int2) input.pos.xy;
    
    if (endCapMaskTex[st].r == 0)
        discard;
    
    return float4(1.0, 1.0, 1.0, 1.0);
}