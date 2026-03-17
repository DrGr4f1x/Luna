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

Texture2D<float4> contourDataTex : BINDING(t0, 0);
RWTexture2D<float4> outEdgeCrossingTex : BINDING(u0, 0);
RWTexture2D<uint> outEdgeId : BINDING(u1, 0);
RWTexture2D<uint> outEdgeCrossingTex2 : BINDING(u2, 0);
SamplerState clampSampler : register(s0);

struct Constants
{
    float2 texDimensions;
    float2 invTexDimensions;
};

ConstantBuffer<Constants> Globals : BINDING(b0, 0);

int Quantize(float2 vec)
{
    float dx = dot(vec, float2(1.0, 0.0));
    float dy = dot(vec, float2(0.0, 1.0));
    if (abs(dx) > abs(dy))
    {
        return sign(dx) == 1 ? 0 : 2;
    }
    else
    {
        return sign(dy) == 1 ? 1 : 3;
    }
}

uint GetEdgeState2(float2 vec)
{
    if (dot(vec, vec) == 0.0)
        return 0;
    
    float dx = dot(vec, float2(1.0, 0.0));
    float dy = dot(vec, float2(0.0, 1.0));
    if (abs(dx) > abs(dy))
    {
        return sign(dx) == 1 ? (1 << 1) : (1 << 0);
    }
    else
    {
        return sign(dy) == 1 ? (1 << 3) : (1 << 2);
    }
}

uint GetEdgeState(float2 vec)
{
    if (dot(vec, vec) == 0.0)
        return 0;
    
    float dx = dot(vec, float2(1.0, 0.0));
    float dy = dot(vec, float2(0.0, 1.0));
    
    uint ret = 0;
    
    // Left-right direction
    if (dx < 0.0)
        ret |= (1 << 0);
    else if (dx > 0.0)
        ret |= (1 << 1);
    
    // Top-bottom direction
    if (dy < 0.0)
        ret |= (1 << 2);
    else if (dy > 0.0)
        ret |= (1 << 3);
      
    return ret;
}

int DecodeEdgeState(uint state, uint direction)
{
    switch (direction)
    {
        // Left-to-right
        case 0:
            if ((state & (1 << 0)) == (1 << 0))
                return 1;
            else if ((state & (1 << 1)) == (1 << 1))
                return -1;
            break;
        
        // Right-to-left
        case 1:
            if ((state & (1 << 0)) == (1 << 0))
                return -1;
            else if ((state & (1 << 1)) == (1 << 1))
                return 1;
            break;
        
        // Top-to-bottom
        case 2:
            if ((state & (1 << 2)) == (1 << 2))
                return 1;
            else if ((state & (1 << 3)) == (1 << 3))
                return -1;
            break;
        
        // Bottom-to-top
        case 3:
            if ((state & (1 << 2)) == (1 << 2))
                return -1;
            else if ((state & (1 << 3)) == (1 << 3))
                return 1;
            break;
    }
    
    return 0;
}

struct SEdgeState
{
    int center;
    int east;
    int west;
    int north;
    int south;
};


groupshared uint gs_EdgeState[100];

void FillGroupShared(uint3 Gid, uint3 GTid, uint3 DTid)
{
    uint2 tile = 8 * Gid.xy;
    uint2 st = tile + GTid.xy;
    
    if (GTid.x < 5 && GTid.y < 5)
    {
        float2 prefetchUV = (tile + 2.0 * GTid.xy) * Globals.invTexDimensions;
        uint destIdx = GTid.x * 2 + GTid.y * 2 * 10;
        
        float4 R = contourDataTex.GatherRed(clampSampler, prefetchUV);
        float4 G = contourDataTex.GatherGreen(clampSampler, prefetchUV);
        
        gs_EdgeState[destIdx] = GetEdgeState(float2(R.w, G.w));
        gs_EdgeState[destIdx + 1] = GetEdgeState(float2(R.z, G.z));
        gs_EdgeState[destIdx + 10] = GetEdgeState(float2(R.x, G.x));
        gs_EdgeState[destIdx + 11] = GetEdgeState(float2(R.y, G.y));
    }
}

SEdgeState GetEdgeState(uint3 GTid, uint direction)
{
    SEdgeState edgeState = (SEdgeState) 0;
    
    uint centerIdx = (GTid.x + GTid.y * 10) + 11;
    
    edgeState.center = DecodeEdgeState(gs_EdgeState[centerIdx], direction);
    
    edgeState.north = DecodeEdgeState(gs_EdgeState[centerIdx - 10], direction);
    edgeState.south = DecodeEdgeState(gs_EdgeState[centerIdx + 10], direction);
    edgeState.east = DecodeEdgeState(gs_EdgeState[centerIdx + 1], direction);
    edgeState.west = DecodeEdgeState(gs_EdgeState[centerIdx - 1], direction);
    
    return edgeState;
}

[numthreads(8, 8, 1)]
void main(uint3 Gid : SV_GroupID, uint3 GTid : SV_GroupThreadID, uint3 DTid : SV_DispatchThreadID)
{ 
    FillGroupShared(Gid, GTid, DTid);
    
    GroupMemoryBarrierWithGroupSync();
    
    uint direction = 0;
    
    SEdgeState edgeState = GetEdgeState(GTid, direction);
    
    float4 outColor = float4(0.0, 0.0, 0.0, 0.0);
    
    int edgeStateFinal_LR = 0;
    
    
    switch (direction)
    {
        // Left-to-right
        case 0:
            if (edgeState.center == 1)
                edgeStateFinal_LR = (edgeState.west == 0) ? edgeState.center : 0;
            else if (edgeState.center == -1)
                edgeStateFinal_LR = (edgeState.east == 0) ? edgeState.center : 0;
            break;
        
        // Right-to-left
        case 1:
            if (edgeState.center == 1)
                edgeStateFinal_LR = (edgeState.east == 0) ? edgeState.center : 0;
            else if (edgeState.center == -1)
                edgeStateFinal_LR = (edgeState.west == 0) ? edgeState.center : 0;
            break;
        
        // Top-to-bottom
        case 2:
            if (edgeState.center == 1)
                edgeStateFinal_LR = (edgeState.north == 0) ? edgeState.center : 0;
            else if (edgeState.center == -1)
                edgeStateFinal_LR = (edgeState.south == 0) ? edgeState.center : 0;
            break;
        
        // Bottom-to-top
        case 3:
            if (edgeState.center == 1)
                edgeStateFinal_LR = (edgeState.south == 0) ? edgeState.center : 0;
            else if (edgeState.center == -1)
                edgeStateFinal_LR = (edgeState.north == 0) ? edgeState.center : 0;
            break;
    } 
    
    if (edgeStateFinal_LR == 1)
        outColor.r = 1.0;
    else if (edgeStateFinal_LR == -1)
        outColor.g = 1.0;
    
    outEdgeCrossingTex[DTid.xy] = outColor;
    
    int edgeEvidence = 0;
    
    // Left-to-right
    edgeState = GetEdgeState(GTid, 0);
    if (edgeState.center == 1)
        edgeEvidence += (edgeState.west == 0) ? 1 : 0;
    else if (edgeState.center == -1)
        edgeEvidence += (edgeState.east == 0) ? 1 : 0;
    
    // Top-to-bottom
    edgeState = GetEdgeState(GTid, 2);
    if (edgeState.center == 1)
        edgeEvidence += (edgeState.north == 0) ? 1 : 0;
    else if (edgeState.center == -1)
        edgeEvidence += (edgeState.south == 0) ? 1 : 0;
    
    if (edgeEvidence > 0)
        outEdgeId[DTid.xy] = (uint) contourDataTex[DTid.xy].w;
    else
        outEdgeId[DTid.xy] = 0;
    
    // Final actual important output
    uint finalOutput = 0;
    
    // Bit 0 = horizontal enter, bit 1 = horizontal exit
    edgeState = GetEdgeState(GTid, 0);
    if (edgeState.center == 1)
        finalOutput |= (edgeState.west == 0) ? (1 << 0) : 0;
    else if (edgeState.center == -1)
        finalOutput |= (edgeState.east == 0) ? (1 << 1) : 0;
    
    // Bit 2 = vertical enter, bit 3 = vertical exit
    edgeState = GetEdgeState(GTid, 2);
    if (edgeState.center == 1)
        finalOutput |= (edgeState.north == 0) ? (1 << 2) : 0;
    else if (edgeState.center == -1)
        finalOutput |= (edgeState.south == 0) ? (1 << 3) : 0;
    
    outEdgeCrossingTex2[DTid.xy] = finalOutput;
}