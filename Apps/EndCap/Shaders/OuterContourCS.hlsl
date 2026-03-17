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

RWTexture2D<uint> OuterContourTex : BINDING(u0, 0);
Buffer<int> LeftBounds : BINDING(t0, 0);
Buffer<int> RightBounds : BINDING(t1, 0);
Buffer<int> TopBounds : BINDING(t2, 0);
Buffer<int> BottomBounds : BINDING(t3, 0);

struct Constants_t
{
    int width;
    int height;
};

[[vk::push_constant]]
ConstantBuffer<Constants_t> Constants : register(b0);

[numthreads(8, 8, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    if(LeftBounds[DTid.y] == DTid.x || RightBounds[DTid.y] == DTid.x ||
        TopBounds[DTid.x] == DTid.y || BottomBounds[DTid.x] == DTid.y)
    {
        OuterContourTex[DTid.xy] = 1;
    }
}