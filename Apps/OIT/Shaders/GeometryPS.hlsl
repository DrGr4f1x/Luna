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
    float viewDepth : TEXCOORD;
};


struct Node
{
	float4 color;
	float depth;
	uint next;
};

struct GeometryInfo
{
	uint count;
	uint maxNodeCount;
};

RWStructuredBuffer<GeometryInfo> geometryBuffer : BINDING(u0, 2);
RWStructuredBuffer<Node> nodeBuffer : BINDING(u1, 2);
RWTexture2D<uint> headIndexImage : BINDING(u2, 2);

struct PushConstants
{
	float4x4 modelMatrix;
	float4 color;
};

VK_PUSH_CONSTANT
ConstantBuffer<PushConstants> pushConstants : register(b1);


[earlydepthstencil]
void main(PSInput input)
{
    // Increase the node count
    uint nodeIdx;
    InterlockedAdd(geometryBuffer[0].count, 1, nodeIdx);

    // Check LinkedListSBO is full
    if (nodeIdx < geometryBuffer[0].maxNodeCount)
    {
        // Exchange new head index and previous head index
        uint prevHeadIdx;
        InterlockedExchange(headIndexImage[uint2(input.pos.xy)], nodeIdx, prevHeadIdx);

        // Store node data
        nodeBuffer[nodeIdx].color = pushConstants.color;
        nodeBuffer[nodeIdx].depth = input.viewDepth;
        nodeBuffer[nodeIdx].next = prevHeadIdx;
    }
}