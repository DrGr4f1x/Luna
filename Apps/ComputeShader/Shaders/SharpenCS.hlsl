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


Texture2D inputTex : register(t0 VK_DESCRIPTOR_SET(0));
RWTexture2D<float4> outputTex : register(u0 VK_DESCRIPTOR_SET(0));


float Convolve(float kernel[9], float data[9], float denom, float offset)
{
    float res = 0.0f;
    for (int i = 0; i < 9; ++i)
    {
        res += kernel[i] * data[i];
    }

    return clamp((res / denom) + offset, 0.0f, 1.0f);
}


[numthreads(16, 16, 1)]
void main(uint3 DTid : SV_DispatchThreadId)
{
    int n = -1;
    float temp_r[9];
    float temp_g[9];
    float temp_b[9];

    for (int i = -1; i < 2; ++i)
    {
        for (int j = -1; j < 2; ++j)
        {
            n++;
            float3 color = inputTex.Load(int3(DTid.x + i, DTid.y + j, 0)).xyz;
            temp_r[n] = color.r;
            temp_g[n] = color.g;
            temp_b[n] = color.b;
        }
    }

    float kernel[9];
    kernel[0] = -1.0f;
    kernel[1] = -1.0f;
    kernel[2] = -1.0f;
    kernel[3] = -1.0f;
    kernel[4] = 9.0f;
    kernel[5] = -1.0f;
    kernel[6] = -1.0f;
    kernel[7] = -1.0f;
    kernel[8] = -1.0f;

    float4 res = float4(
		Convolve(kernel, temp_r, 1.0f, 0.0f),
		Convolve(kernel, temp_g, 1.0f, 0.0f),
		Convolve(kernel, temp_b, 1.0f, 0.0f),
		1.0f);

    outputTex[DTid.xy] = res;
}