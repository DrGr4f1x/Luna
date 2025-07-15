//
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
// Author:  David Elder
//

struct Particle
{
    float2 pos;
    float2 vel;
    float4 gradientPos;
};

[[vk::binding(0, 0)]]
StructuredBuffer<Particle> particles : register(t0);

[[vk::binding(1, 0)]]
cbuffer VSConstants : register(b1)
{
    float2 invTargetSize;
    float pointSize;
};

struct VSOutput
{
    float4 pos : SV_Position;
    float2 uv : TEXCOORD0;
    float gradientU : TEXCOORD1;
};

VSOutput main(uint vertId : SV_VertexID)
{
    VSOutput output = (VSOutput) 0;

    uint particleId = vertId / 6;
    uint index = vertId % 6;

    Particle particle = particles[particleId];
    float2 pos = particle.pos;

    float scale = pointSize;

    if (index == 0)
    {
        output.pos = float4(pos + scale * invTargetSize * float2(-1.0, 1.0), 0.0, 1.0);
        output.uv = float2(0.0, 0.0);
    }
    else if (index == 1 || index == 5)
    {
        output.pos = float4(pos + scale * invTargetSize * float2(-1.0, -1.0), 0.0, 1.0);
        output.uv = float2(0.0, 1.0);
    }
    else if (index == 2 || index == 4)
    {
        output.pos = float4(pos + scale * invTargetSize * float2(1.0, 1.0), 0.0, 1.0);
        output.uv = float2(1.0, 0.0);
    }
    else
    {
        output.pos = float4(pos + scale * invTargetSize * float2(1.0, -1.0), 0.0, 1.0);
        output.uv = float2(1.0, 1.0);
    }

    output.gradientU = particle.gradientPos.x;

    return output;
}