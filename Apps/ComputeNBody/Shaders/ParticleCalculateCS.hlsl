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


cbuffer CSConstants : BINDING(b0, 0)
{
    float deltaT;
    float destX;
    float destY;
    int particleCount;
};


RWStructuredBuffer<Particle> particles : BINDING(u0, 0);


#define SHARED_DATA_SIZE 512


// Share data between computer shader invocations to speed up caluclations
groupshared float4 sharedData[SHARED_DATA_SIZE];


[numthreads(256, 1, 1)]
void main(uint GIndex : SV_GroupIndex, uint DTid : SV_DispatchThreadId)
{
    const float GRAVITY = 0.002;
    const float POWER = 0.75;
    const float SOFTEN = 0.0075;

	// Current UAV index
    uint index = DTid;
    if (index >= particleCount)
        return;

    float4 position = particles[index].pos;
    float4 velocity = particles[index].vel;
    float4 acceleration = 0.0.xxxx;

    for (uint i = 0; i < particleCount; i += SHARED_DATA_SIZE)
    {
        if (i + GIndex < particleCount)
        {
            sharedData[GIndex] = particles[i + GIndex].pos;
        }
        else
        {
            sharedData[GIndex] = 0.0.xxxx;
        }

        GroupMemoryBarrierWithGroupSync();

        for (int j = 0; j < 256; j++)
        {
            float4 other = sharedData[j];
            float3 len = other.xyz - position.xyz;
            acceleration.xyz += GRAVITY * len * other.w / pow(dot(len, len) + SOFTEN, POWER);
        }

        GroupMemoryBarrierWithGroupSync();
    }

    particles[index].vel.xyz += deltaT * acceleration.xyz;

	// Gradient texture position
    particles[index].vel.w += 0.1 * deltaT;
    if (particles[index].vel.w > 1.0)
        particles[index].vel.w -= 1.0;
}