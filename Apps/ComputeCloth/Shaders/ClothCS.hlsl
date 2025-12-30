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
    float4 uv;
    float4 normal;
    float4 pinned;
};


StructuredBuffer<Particle> particleIn : register(t0 VK_DESCRIPTOR_SET(0));
RWStructuredBuffer<Particle> particleOut : register(u0 VK_DESCRIPTOR_SET(0));


cbuffer CSConstants : register(b0 VK_DESCRIPTOR_SET(0))
{
    float deltaT;
    float particleMass;
    float springStiffness;
    float damping;
    float restDistH;
    float restDistV;
    float restDistD;
    float sphereRadius;
    float4 spherePos;
    float4 gravity;
    int2 particleCount;
    uint calculateNormals;
};


float3 SpringForce(float3 p0, float3 p1, float restDist)
{
    float3 dist = p0 - p1;
    return normalize(dist) * springStiffness * (length(dist) - restDist);
}


[numthreads(8, 8, 1)]
void main(uint3 DTid : SV_DispatchThreadId)
{
    uint index = DTid.y * particleCount.x + DTid.x;
    if (index > particleCount.x * particleCount.y)
        return;

	// Pinned?
    if (particleIn[index].pinned.x == 1.0)
    {
        particleOut[index].pos = particleIn[index].pos;
        particleOut[index].vel = 0.0.xxxx;
        return;
    }

	// Initial force from gravity
    float3 force = gravity.xyz * particleMass;

    float3 pos = particleIn[index].pos.xyz;
    float3 vel = particleIn[index].vel.xyz;

	// Spring forces from neighboring particles

	// left
    if (DTid.x > 0)
    {
        force += SpringForce(particleIn[index - 1].pos.xyz, pos, restDistH);
    }

	// right
    if (DTid.x < particleCount.x - 1)
    {
        force += SpringForce(particleIn[index + 1].pos.xyz, pos, restDistH);
    }

	// upper
    if (DTid.y < particleCount.y - 1)
    {
        force += SpringForce(particleIn[index + particleCount.x].pos.xyz, pos, restDistV);
    }

	// lower
    if (DTid.y > 0)
    {
        force += SpringForce(particleIn[index - particleCount.x].pos.xyz, pos, restDistV);
    }

	// upper-left
    if ((DTid.x > 0) && (DTid.y < particleCount.y - 1))
    {
        force += SpringForce(particleIn[index + particleCount.x - 1].pos.xyz, pos, restDistD);
    }

	// lower-left
    if ((DTid.x > 0) && (DTid.y > 0))
    {
        force += SpringForce(particleIn[index - particleCount.x - 1].pos.xyz, pos, restDistD);
    }

	// upper-right
    if ((DTid.x < particleCount.x - 1) && (DTid.y < particleCount.y - 1))
    {
        force += SpringForce(particleIn[index + particleCount.x + 1].pos.xyz, pos, restDistD);
    }

	// lower-right
    if ((DTid.x < particleCount.x - 1) && (DTid.y > 0))
    {
        force += SpringForce(particleIn[index - particleCount.x + 1].pos.xyz, pos, restDistD);
    }

    force += (-damping * vel);

	// Integrate
    float3 f = force * (1.0 / particleMass);
    particleOut[index].pos = float4(pos + vel * deltaT + 0.5 * f * deltaT * deltaT, 1.0);
    particleOut[index].vel = float4(vel + f * deltaT, 0.0);

	// Sphere collision
    float3 sphereDist = particleOut[index].pos.xyz - spherePos.xyz;

    if (length(sphereDist) < sphereRadius + 0.01)
    {
		// If the particle is inside the sphere, push it to the outer radius
        particleOut[index].pos.xyz = spherePos.xyz + normalize(sphereDist) * (sphereRadius + 0.01);
		// Cancel out velocity
        particleOut[index].vel = 0.0.xxxx;
    }

	// Normals
    if (calculateNormals == 1)
    {
        float3 normal = 0.0.xxx;
        float3 a, b, c;

        if (DTid.y > 0)
        {
            if (DTid.x > 0)
            {
                a = particleIn[index - 1].pos.xyz - pos;
                b = particleIn[index - particleCount.x - 1].pos.xyz - pos;
                c = particleIn[index - particleCount.x].pos.xyz - pos;
                normal += cross(a, b) + cross(b, c);
            }

            if (DTid.x < particleCount.x - 1)
            {
                a = particleIn[index - particleCount.x].pos.xyz - pos;
                b = particleIn[index - particleCount.x + 1].pos.xyz - pos;
                c = particleIn[index + 1].pos.xyz - pos;
                normal += cross(a, b) + cross(b, c);
            }
        }

        if (DTid.y < particleCount.y - 1)
        {
            if (DTid.x > 0)
            {
                a = particleIn[index + particleCount.x].pos.xyz - pos;
                b = particleIn[index + particleCount.x - 1].pos.xyz - pos;
                c = particleIn[index - 1].pos.xyz - pos;
                normal += cross(a, b) + cross(b, c);
            }

            if (DTid.x < particleCount.x - 1)
            {
                a = particleIn[index + 1].pos.xyz - pos;
                b = particleIn[index + particleCount.x + 1].pos.xyz - pos;
                c = particleIn[index + particleCount.x].pos.xyz - pos;
                normal += cross(a, b) + cross(b, c);
            }
        }
        particleOut[index].normal = float4(normalize(normal), 0.0f);
    }
}