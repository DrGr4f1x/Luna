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
    float4 pos : SV_Position;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD0;
    float3 lightVec : TEXCOORD1;
};


[[vk::binding(0, 3)]]
Texture2D heightTex : register(t0);

[[vk::binding(1, 3)]]
Texture2DArray layerTexArray : register(t1);

[[vk::binding(0, 5)]]
SamplerState linearSamplerMirror : register(s0);

[[vk::binding(1, 5)]]
SamplerState linearSamplerWrap : register(s1);


float3 SampleTerrainLayer(float2 uv)
{
	// Define some layer ranges for sampling depending on terrain height
    float2 layers[6];
    layers[0] = float2(-10.0, 10.0);
    layers[1] = float2(5.0, 45.0);
    layers[2] = float2(45.0, 80.0);
    layers[3] = float2(75.0, 100.0);
    layers[4] = float2(95.0, 140.0);
    layers[5] = float2(140.0, 190.0);

    float3 color = 0.0.xxx;

	// Get height from displacement map
    float height = heightTex.SampleLevel(linearSamplerMirror, uv, 0.0).r * 255.0;

    for (int i = 0; i < 6; ++i)
    {
        float range = layers[i].y - layers[i].x;
        float weight = (range - abs(height - layers[i].y)) / range;
        weight = max(0.0, weight);
        color += weight * layerTexArray.Sample(linearSamplerWrap, float3(uv * 16.0, i)).rgb;
    }

    return color;
}

float Fog(float4 pos, float density)
{
    const float LOG2 = -1.442695;
    float dist = pos.z / pos.w * 0.1;
    float d = density * dist;
    return 1.0 - clamp(exp2(d * d * LOG2), 0.0, 1.0);
}

float4 main(PSInput input) : SV_Target
{
    float3 N = normalize(input.normal);
    float3 L = normalize(input.lightVec);
    float3 ambient = 0.5.xxx;
    float3 diffuse = max(dot(N, L), 0.0).xxx;

    float4 color = float4((ambient + diffuse) * SampleTerrainLayer(input.uv), 1.0);

    const float4 fogColor = float4(0.47, 0.5, 0.67, 0.0);
    return lerp(color, fogColor, Fog(input.pos, 0.25));
}