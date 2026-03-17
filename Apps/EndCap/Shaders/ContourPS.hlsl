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

RWBuffer<int> BoundsBuffer : BINDING(u0, 0);
RWBuffer<int> LeftBounds : BINDING(u1, 0);
RWBuffer<int> RightBounds : BINDING(u2, 0);
RWBuffer<int> TopBounds : BINDING(u3, 0);
RWBuffer<int> BottomBounds : BINDING(u4, 0);

struct PSInput
{
    float4 pos      : SV_Position;
    float3 normal   : NORMAL;
    float3 color    : COLOR;
    float4 wpos     : TEXCOORD;
    float id        : TEXCOORD1;
};


struct PSOutput
{
    float4 contourData    : SV_TARGET0;
};


PSOutput main(PSInput input)
{
    PSOutput output = (PSOutput) 0;
    
    float3 normal = normalize(float3(input.normal.xy, 0.0));
    normal.y = -normal.y;
    
    output.contourData = float4(normal.xy, 1.0, input.id);
    
    int2 st = (int2) input.pos.xy;
    
    // Calc bounding box
    InterlockedMin(BoundsBuffer[0], st.x); // Left edge     (first column)
    InterlockedMax(BoundsBuffer[1], st.x); // Right edge    (last column)
    InterlockedMin(BoundsBuffer[2], st.y); // Top edge      (first row)
    InterlockedMax(BoundsBuffer[3], st.y); // Bottom edge   (last row)
        
    // Left-right bounds
    InterlockedMin(LeftBounds[st.y], st.x);
    InterlockedMax(RightBounds[st.y], st.x);
    
    // Top-bottom bounds
    InterlockedMin(TopBounds[st.x], st.y);
    InterlockedMax(BottomBounds[st.x], st.y);
    
    return output;
}