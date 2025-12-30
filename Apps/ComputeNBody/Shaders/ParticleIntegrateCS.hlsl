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


[numthreads(256, 1, 1)]
void main(uint DTid : SV_DispatchThreadId)
{
    float4 position = particles[DTid].pos;
    float4 velocity = particles[DTid].vel;
    position += deltaT * velocity;
    particles[DTid].pos = position;
}