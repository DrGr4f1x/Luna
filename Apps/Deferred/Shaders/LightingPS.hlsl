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


Texture2D texturePosition : BINDING(t0, 0);
Texture2D textureNormal : BINDING(t1, 0);
Texture2D textureAlbedo : BINDING(t2, 0);
SamplerState samplerLinear : BINDING(s0, 1);


struct Light
{
    float4 position;
    float4 colorAndRadius;
};


cbuffer ubo : BINDING(b0, 0)
{
    Light lights[6];
    float4 viewPos;
    int displayDebugTarget;
}


struct PSInput
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD0;
};


float4 main(PSInput input) : SV_TARGET
{
	// Get G-Buffer values
    float3 fragPos = texturePosition.Sample(samplerLinear, input.uv).rgb;
    float3 normal = textureNormal.Sample(samplerLinear, input.uv).rgb;
    float4 albedo = textureAlbedo.Sample(samplerLinear, input.uv);

    float3 fragcolor;

	// Debug display
    if (displayDebugTarget > 0)
    {
        switch (displayDebugTarget)
        {
            case 1:
                fragcolor.rgb = fragPos;
                break;
            case 2:
                fragcolor.rgb = normal;
                break;
            case 3:
                fragcolor.rgb = albedo.rgb;
                break;
            case 4:
                fragcolor.rgb = albedo.aaa;
                break;
        }
        return float4(fragcolor, 1.0);
    }

#define lightCount 6
#define ambient 0.0

	// Ambient part
    fragcolor = albedo.rgb * ambient;

    for (int i = 0; i < lightCount; ++i)
    {
		// Vector to light
        float3 L = lights[i].position.xyz - fragPos;
		// Distance from light to fragment position
        float dist = length(L);

		// Viewer to fragment
        float3 V = viewPos.xyz - fragPos;
        V = normalize(V);

		//if(dist < ubo.lights[i].radius)
		{
			// Light to fragment
            L = normalize(L);

            float3 color = lights[i].colorAndRadius.xyz;
            float radius = lights[i].colorAndRadius.w;
            
			// Attenuation
            float atten = radius / (pow(dist, 2.0) + 1.0);

			// Diffuse part
            float3 N = normalize(normal);
            float NdotL = max(0.0, dot(N, L));
            float3 diff = color * albedo.rgb * NdotL * atten;

			// Specular part
			// Specular map values are stored in alpha of albedo mrt
            float3 R = reflect(-L, N);
            float NdotR = max(0.0, dot(R, V));
            float3 spec = color * albedo.a * pow(NdotR, 16.0) * atten;

            fragcolor += diff + spec;
        }
    }

    return float4(fragcolor, 1.0);
}