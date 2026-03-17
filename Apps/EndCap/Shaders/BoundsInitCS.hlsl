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

RWBuffer<int> BoundsBuffer : BINDING(u0, 0);

struct Constants_t
{
    int width;
    int height;
};

[[vk::push_constant]]
ConstantBuffer<Constants_t> Constants : register(b0);

[numthreads(64, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadId)
{
    if (DTid.x == 0)
    {
        BoundsBuffer[0] = Constants.width;
    }
    else if (DTid.x == 1)
    {
        BoundsBuffer[1] = -1;
    }
    else if (DTid.x == 2)
    {
        BoundsBuffer[2] = Constants.height;
    }
    else if (DTid.x == 3)
    {
        BoundsBuffer[3] = -1;
    }
}