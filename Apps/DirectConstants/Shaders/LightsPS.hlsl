//
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
// Author:  David Elder
//

#define LIGHT_COUNT 6

struct PSInput
{
    float4 position : SV_Position;
    float3 normal : NORMAL;
    float3 color : COLOR;
    float4 lightVec[LIGHT_COUNT] : TEXCOORD0;
};

#define MAX_LIGHT_DIST (9.0 * 9.0)

float4 main(PSInput input) : SV_TARGET
{
    float3 lightColor[LIGHT_COUNT];
    lightColor[0] = float3(1.0, 0.0, 0.0);
    lightColor[1] = float3(0.0, 1.0, 0.0);
    lightColor[2] = float3(0.0, 0.0, 1.0);
    lightColor[3] = float3(1.0, 0.0, 1.0);
    lightColor[4] = float3(0.0, 1.0, 1.0);
    lightColor[5] = float3(1.0, 1.0, 0.0);

    float3 diffuse = 0.0.xxx;

	// Just some very basic attenuation
    for (int i = 0; i < LIGHT_COUNT; ++i)
    {
        float lRadius = MAX_LIGHT_DIST * input.lightVec[i].w;

        float dist = min(dot(input.lightVec[i], input.lightVec[i]), lRadius) / lRadius;
        float distFactor = 1.0 - dist;

        diffuse += lightColor[i] * distFactor;
    }

    return float4(diffuse, 1.0);
}