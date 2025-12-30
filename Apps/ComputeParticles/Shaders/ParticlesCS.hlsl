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
    float2 pos;
    float2 vel;
    float4 gradientPos;
};


RWStructuredBuffer<Particle> particles : register(u0 VK_DESCRIPTOR_SET(0));


cbuffer CSConstants : register(b0 VK_DESCRIPTOR_SET(0))
{
    float deltaT;
    float destX;
    float destY;
    int particleCount;
};


float2 Attraction(float2 pos, float2 attractorPos)
{
    float2 delta = attractorPos - pos;
    const float damp = 0.5;
    float dDampedDot = dot(delta, delta) + damp;
    float invDist = 1.0f / sqrt(dDampedDot);
    float invDistCubed = invDist * invDist * invDist;
    return delta * invDistCubed * 0.0035;
}


float2 Repulsion(float2 pos, float2 attractorPos)
{
    float2 delta = attractorPos - pos;
    float targetDistance = sqrt(dot(delta, delta));
    return delta * (1.0 / (targetDistance * targetDistance * targetDistance)) * -0.000035;
}


[numthreads(256, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadId)
{
    uint index = DTid.x;
    if (index >= particleCount)
        return;

    float2 vel = particles[index].vel;
    float2 pos = particles[index].pos;

    float2 destPos = float2(destX, destY);

    float2 delta = destPos - pos;
    float targetDistance = sqrt(dot(delta, delta));
    vel += Repulsion(pos, destPos) * 0.05;

    pos += vel * deltaT;

    if ((pos.x < -1.0) || (pos.x > 1.0) || (pos.y < -1.0) || (pos.y > 1.0))
        vel = (-vel * 0.1) + Attraction(pos, destPos) * 12.0;
    else
        particles[index].pos.xy = pos;

    particles[index].vel = vel;
    particles[index].gradientPos.x += 0.02 * deltaT;
    if (particles[index].gradientPos.x > 1.0)
        particles[index].gradientPos.x -= 1.0;
}