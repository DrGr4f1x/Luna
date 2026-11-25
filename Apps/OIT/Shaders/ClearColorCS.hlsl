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

RWTexture2D<uint> headIndexImage : BINDING(u0, 0);

[numthreads(8, 8, 1)]
void main(uint3 DTid : SV_DispatchThreadId)
{
    headIndexImage[DTid.xy] = 0xFFFFFFFF;
}