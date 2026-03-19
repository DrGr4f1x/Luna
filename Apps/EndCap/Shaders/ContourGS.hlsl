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

struct GSInput
{
    float3 pos      : POSITION;
    float3 normal   : NORMAL;
    float4 color    : COLOR;
    float id        : TEXCOORD;
};


struct GSOutput
{
    float4 pos      : SV_Position;
    float3 normal   : NORMAL;
    float3 color    : COLOR;
    float4 wpos     : TEXCOORD;
    float id        : TEXCOORD1;
};


cbuffer GSConstants : BINDING(b0, 1)
{
    float4x4 modelViewProjectionMatrix;
    float4x4 modelViewProjectionInvTransposeMatrix;
    float4x4 modelViewMatrix;
    float4x4 modelMatrix;
    float4 plane;
};


float3 ProjectOntoPlane(float3 vec, float3 normal)
{
    return vec - dot(vec, normal) * normal;
}


GSOutput EmitVertex(float d0, float3 pt0, float d1, float3 pt1)
{
    float c = d0 / (d0 - d1);
    float3 intpos = lerp(pt0, pt1, c);
    
    GSOutput output = (GSOutput) 0;
    
    output.pos = mul(modelViewProjectionMatrix, float4(intpos, 1.0f));
    output.color = float4(1.0, 0.0, 0.0, 1.0);
    output.wpos = float4(intpos, 0.0);
    
    return output;
}


[maxvertexcount(2)]
void main(triangle GSInput input[3], inout LineStream<GSOutput> outputStream)
{
    float3 worldPos[3];
    worldPos[0] = mul(modelMatrix, float4(input[0].pos, 1.0));
    worldPos[1] = mul(modelMatrix, float4(input[1].pos, 1.0));
    worldPos[2] = mul(modelMatrix, float4(input[2].pos, 1.0));
    
    float d[3];
    d[0] = dot(worldPos[0], plane.xyz) + plane.w;
    d[1] = dot(worldPos[1], plane.xyz) + plane.w;
    d[2] = dot(worldPos[2], plane.xyz) + plane.w;
    
    // In this bitmask a 1 means the corresponding vertex is above the plane, 0 means below
    uint bitmask = 0u;
    
    if (d[0] > 0.0)
    {
        bitmask |= 1 << 0;
    }
    if (d[1] > 0.0)
    {
        bitmask |= 1 << 1;
    }
    if (d[2] > 0.0)
    {
        bitmask |= 1 << 2;
    }
    
    // If the bitmask is 0 the triangle is entirely below the plane (7 means entirely above plane)
    if (bitmask == 0 || bitmask == 7)
        return;
    
    GSOutput output[2];
    
    // In these case blocks, the convention is the first vertex is above the plane, and the second is below.
    // We assume CCW winding order.  Would need to reverse for CW.
    switch (bitmask)
    {
        case 1:
            output[0] = EmitVertex(d[0], worldPos[0], d[1], worldPos[1]);
            output[1] = EmitVertex(d[0], worldPos[0], d[2], worldPos[2]);
            break;
    
        case 2:
            output[0] = EmitVertex(d[1], worldPos[1], d[2], worldPos[2]);
            output[1] = EmitVertex(d[1], worldPos[1], d[0], worldPos[0]);
            break;
        
        case 3:
            output[0] = EmitVertex(d[1], worldPos[1], d[2], worldPos[2]);
            output[1] = EmitVertex(d[0], worldPos[0], d[2], worldPos[2]);
            break;
        
        case 4:
            output[0] = EmitVertex(d[2], worldPos[2], d[0], worldPos[0]);
            output[1] = EmitVertex(d[2], worldPos[2], d[1], worldPos[1]);
            break;
        
        case 5:
            output[0] = EmitVertex(d[0], worldPos[0], d[1], worldPos[1]);
            output[1] = EmitVertex(d[2], worldPos[2], d[1], worldPos[1]);
            break;
        
        case 6:
            output[0] = EmitVertex(d[2], worldPos[2], d[0], worldPos[0]);
            output[1] = EmitVertex(d[1], worldPos[1], d[0], worldPos[0]);
            break;
    }
    
    // Calculate the normal.  The point of all the bitmask logic is to ensure
    // that we can calculate a normal that is pointing out from the edge (and the face of the triangle)
    float3 normal = cross(normalize(output[1].wpos.xyz - output[0].wpos.xyz), plane.xyz);
    normal = mul((float3x3) modelViewProjectionInvTransposeMatrix, normal);
    output[0].normal = normal;
    output[1].normal = normal;
    output[0].id = input[0].id;
    output[1].id = input[0].id;
        
    outputStream.Append(output[0]);
    outputStream.Append(output[1]);

    outputStream.RestartStrip();
}