//
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
// Author:  David Elder
//

struct VSInput
{
    float3 pos : POSITION;
};


[[vk::binding(0, 0)]]
cbuffer ubo : register(b0)
{
    float4x4 depthModelViewProjectionMatrix;
}


float4 main(VSInput input) : SV_POSITION
{
    return mul(depthModelViewProjectionMatrix, float4(input.pos, 1.0));
}