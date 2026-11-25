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

#define MAX_FRAGMENT_COUNT 128


struct PSInput
{
	float4 pos : SV_POSITION;
};


struct Node
{
	float4 color;
	float depth;
	uint next;
};


Texture2D<uint> headIndexImage : BINDING(t0, 0);

// Binding 0 : Position storage buffer
StructuredBuffer<Node> nodes : BINDING(t1, 0);


float4 main(PSInput input) : SV_TARGET
{
    Node fragments[MAX_FRAGMENT_COUNT];
    int count = 0;

    uint nodeIdx = headIndexImage[uint2(input.pos.xy)].r;

    while (nodeIdx != 0xffffffff)
    {
        if (count >= MAX_FRAGMENT_COUNT)
            break;
        
        fragments[count] = nodes[nodeIdx];
        nodeIdx = fragments[count].next;
        ++count;
    }

    // Do the insertion sort
    for (uint i = 1; i < count; ++i)
    {
        Node insert = fragments[i];
        uint j = i;
        while (j > 0 && insert.depth > fragments[j - 1].depth)
        {
            fragments[j] = fragments[j - 1];
            --j;
        }
        fragments[j] = insert;
    }

    // Do blending
    float4 color = float4(0.025, 0.025, 0.025, 1.0f);
    for (uint f = 0; f < count; ++f)
    {
        color = lerp(color, fragments[f].color, fragments[f].color.a);
    }

    return color;
}