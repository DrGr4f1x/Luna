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

RWTexture2D<uint> endCapMaskTex : BINDING(u0, 0);
RWTexture2D<uint> fillDebugTex  : BINDING(u1, 1);
Texture2D<uint> edgeCrossingTex : BINDING(t0, 0);
Buffer<int> LeftBounds          : BINDING(t1, 0);
Buffer<int> RightBounds         : BINDING(t2, 0);
Buffer<int> TopBounds           : BINDING(t3, 0);
Buffer<int> BottomBounds        : BINDING(t4, 0);

struct Constants_t
{
    int width;
    int height;
    int direction; // 0 = left-to-right, 1 = top-to-bottom
};

[[vk::push_constant]]
ConstantBuffer<Constants_t> Constants : register(b0);


[numthreads(64, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadId)
{
    // Horizontal fill
    if (Constants.direction == 0)
    {
        if (DTid.x >= Constants.height)
            return;
        
        // Check left and right bounds
        int leftCol = LeftBounds[DTid.x];
        int rightCol = RightBounds[DTid.x];
        
        if (leftCol >= rightCol)
            return;
        
        int edgeCount = 0;
        for (int col = leftCol; col <= rightCol; ++col)
        {
            const int2 st = int2(col, DTid.x);
            const uint edgeStatePacked = (uint) edgeCrossingTex[st].r;
            
            bool isLeftEdge = (edgeStatePacked & (1 << 0)) == (1 << 0);
            bool isRightEdge = (edgeStatePacked & (1 << 1)) == (1 << 1);
            
            bool shouldDraw = edgeCount > 0;
            
            if (isLeftEdge)
                edgeCount += 1;
            else if (isRightEdge)
            {
                if (edgeCount > 0)
                    edgeCount -= 1;
            }

            if (edgeCount > 1 || shouldDraw)
            {
                endCapMaskTex[st] += 1;
            }
            fillDebugTex[st] = (uint) edgeCount;
        }
    }
    // Vertical
    else
    {
        if (DTid.x >= Constants.width)
            return;
        
        int topRow = TopBounds[DTid.x];
        int bottomRow = BottomBounds[DTid.x];
        
        if (topRow >= bottomRow)
            return;
        
        int edgeCount = 0;
        for (int row = topRow; row <= bottomRow; ++row)
        {
            int2 st = int2(DTid.x, row);
                
            const uint edgeStatePacked = (uint) edgeCrossingTex[st].r;
            
            bool isTopEdge = (edgeStatePacked & (1 << 2)) == (1 << 2);
            bool isBottomEdge = (edgeStatePacked & (1 << 3)) == (1 << 3);
            
            bool shouldDraw = edgeCount > 0;
            
            if (isTopEdge)
                edgeCount += 1;
            else if (isBottomEdge)
            {
                if (edgeCount > 0)
                    edgeCount -= 1;
            }

            if (edgeCount > 1 || shouldDraw)
            {
                endCapMaskTex[st] += 1;
            }
            fillDebugTex[st] = (uint) edgeCount;
        }
    }
}