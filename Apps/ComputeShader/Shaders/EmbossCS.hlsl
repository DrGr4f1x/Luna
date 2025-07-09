//
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
// Author:  David Elder
//

[[vk::binding(0, 0)]]
Texture2D inputTex : register(t0);
[[vk::binding(1, 0)]]
RWTexture2D<float4> outputTex : register(u1);


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
    float avg[9];

    for (int i = -1; i < 2; ++i)
    {
        for (int j = -1; j < 2; ++j)
        {
            n++;
            float3 color = inputTex.Load(int3(DTid.x + i, DTid.y + j, 0)).xyz;
            avg[n] = (color.r + color.g + color.b) / 3.0f;
        }
    }

    float kernel[9];
    kernel[0] = -1.0;
    kernel[1] = 0.0;
    kernel[2] = 0.0;
    kernel[3] = 0.0;
    kernel[4] = -1.0;
    kernel[5] = 0.0;
    kernel[6] = 0.0;
    kernel[7] = 0.0;
    kernel[8] = 2.0;

    float t = Convolve(kernel, avg, 1.0f, 0.5f);
    float4 res = float4(float3(t, t, t), 1.0f);

    outputTex[DTid.xy] = res;
}