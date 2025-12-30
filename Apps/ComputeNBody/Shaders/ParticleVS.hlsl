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


struct Particle
{
    float4 pos;
    float4 vel;
};


struct VSOutput
{
    float4 pos : POSITION;
    float gradientPos : TEXCOORD0;
    float pointSize : TEXCOORD1;
};


cbuffer VSConstants : register(b0 VK_DESCRIPTOR_SET(0))
{
    float4x4 projectionMatrix;
    float4x4 modelViewMatrix;
    float4x4 invViewMatrix;
    float2 screenDim;
};


StructuredBuffer<Particle> particles : register(t0 VK_DESCRIPTOR_SET(0));


VSOutput main(uint id : SV_VertexID)
{
    VSOutput output = (VSOutput) 0;

    Particle particle = particles[id];

    const float spriteSize = 0.005 * particle.pos.w; // Point size influenced by mass (stored in inPos.w);

    float4 eyePos = mul(modelViewMatrix, float4(particle.pos.xyz, 1.0));
    float4 projectedCorner = mul(projectionMatrix, float4(0.5 * spriteSize, 0.5 * spriteSize, eyePos.z, eyePos.w));

    output.pos = mul(projectionMatrix, eyePos);
    output.gradientPos = particle.vel.w;
    output.pointSize = clamp(screenDim.x * projectedCorner.x / projectedCorner.w, 1.0, 128.0);

    return output;
}
