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

RWBuffer<int> BorderBuffer0 : BINDING(u0, 0);
RWBuffer<int> BorderBuffer1 : BINDING(u1, 0);

struct Constants_t
{
    int bufferSize;
    int maxSize;
};

[[vk::push_constant]]
ConstantBuffer<Constants_t> Constants : register(b0);

[numthreads(64, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadId)
{
    if(DTid.x < Constants.bufferSize)
    {
        BorderBuffer0[DTid.x] = Constants.maxSize;
        BorderBuffer1[DTid.x] = -1;
    }
}