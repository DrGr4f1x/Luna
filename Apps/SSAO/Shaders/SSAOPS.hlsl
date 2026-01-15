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


struct PSInput
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD0;
};


Texture2D positionTex : BINDING(t0, 0);
Texture2D depthTex : BINDING(t1, 0);
Texture2D normalTex : BINDING(t2, 0);
Texture2D noiseTex : BINDING(t3, 0);

SamplerState samplerClamp : BINDING(s0, 1);
SamplerState samplerWrap : BINDING(s1, 1);


#define SSAO_KERNEL_ARRAY_SIZE 64
static const int SSAO_KERNEL_SIZE = 64;
static const float SSAO_RADIUS = 0.3;


struct UBOSSAOKernel
{
    float4 samples[SSAO_KERNEL_ARRAY_SIZE];
};


struct UBO
{
    float4x4 projectionMatrix;
    float2 gbufferTexDim;
    float2 noiseTexDim;
    float invDepthRangeA;
    float invDepthRangeB;
    float linearizeDepthA;
    float linearizeDepthB;
};


ConstantBuffer<UBOSSAOKernel> uboSSAOKernel : BINDING(b0, 0);
ConstantBuffer<UBO> ubo : BINDING(b1, 0);


float DeviceZToLinearZ(float deviceZ)
{
    float normalizedDepth = saturate(ubo.invDepthRangeA * deviceZ + ubo.invDepthRangeB);

    return 1.0 / (normalizedDepth * ubo.linearizeDepthA + ubo.linearizeDepthB);
}


float main(PSInput input) : SV_TARGET
{
	// Get G-Buffer values
    float3 normal = normalize(normalTex.Sample(samplerClamp, input.uv).xyz * 2.0 - 1.0);
    float3 viewPos = positionTex.Sample(samplerClamp, input.uv).xyz;
    
	// Get a random vector using a noise lookup
    const float2 noiseUV = float2(float(ubo.gbufferTexDim.x) / float(ubo.noiseTexDim.x), float(ubo.gbufferTexDim.y) / (ubo.noiseTexDim.y)) * input.uv;
    float3 randomVec = noiseTex.Sample(samplerWrap, noiseUV).xyz * 2.0 - 1.0;

	// Create TBN matrix
    float3 tangent = normalize(randomVec - normal * dot(randomVec, normal));
    float3 bitangent = cross(normal, tangent);
    float3x3 TBN = transpose(float3x3(tangent, bitangent, normal));
    //float3x3 TBN = float3x3(tangent, bitangent, normal);

	// Calculate occlusion value
    float occlusion = 0.0;
    for (int i = 0; i < SSAO_KERNEL_SIZE; i++)
    {
        float3 samplePos = mul(TBN, uboSSAOKernel.samples[i].xyz);
        samplePos = viewPos + samplePos * SSAO_RADIUS;
        
        float3 sampleDir = normalize(samplePos - viewPos);
        float nDotS = max(dot(normal, sampleDir), 0.0);
        
		// project
        float4 offset = mul(ubo.projectionMatrix, float4(samplePos, 1.0));
        offset.xyz /= offset.w;
        
        float2 sampleUV = float2(offset.x * 0.5 + 0.5, -offset.y * 0.5 + 0.5);
        float sampleZ = -positionTex.Sample(samplerClamp, sampleUV).w;

        float rangeCheck = smoothstep(0.0f, 1.0f, SSAO_RADIUS / abs(viewPos.z - sampleZ));
        occlusion += step(samplePos.z, sampleZ) * rangeCheck * nDotS;

    }
    occlusion = 1.0 - (occlusion / float(SSAO_KERNEL_SIZE));

    return occlusion;
}