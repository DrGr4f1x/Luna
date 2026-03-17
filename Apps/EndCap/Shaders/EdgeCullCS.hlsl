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

RWTexture2D<float4> contourDataTex  : BINDING(u0, 0);
RWTexture2D<float4> debugTex        : BINDING(u1, 0);

Texture2D<uint4> edgeCrossingTex    : BINDING(t0, 0);
Texture2D<uint> edgeIdTex           : BINDING(t1, 0);
Texture2D<uint> edgeCrossingTex2    : BINDING(t2, 0);
Buffer<int> LeftBounds              : BINDING(t3, 0);
Buffer<int> RightBounds             : BINDING(t4, 0);
Buffer<int> TopBounds               : BINDING(t5, 0);
Buffer<int> BottomBounds            : BINDING(t6, 0);


struct Constants_t
{
    int width;
    int height;
    int direction; // 0 = left-to-right, 1 = right-to-left, 2 = top-to-bottom, 3 = bottom-to-top
};

[[vk::push_constant]]
ConstantBuffer<Constants_t> Constants : register(b0);

struct TraversalContext
{
    int contourId;
    int lastOrientation;
    int state;

};

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

int GetEdgeState(float2 vec)
{
    if (dot(vec, vec) == 0.0)
        return 0;
    
    int q = Quantize(vec);
    
    // Traversing left-to-right
    if (Constants.direction == 0)
    {
        // Entering from the left
        if (q == 2)
            return 1;
        // Exiting to the right
        else if (q == 0)
            return -1;
    }
    // Traversing right-to-left
    else if (Constants.direction == 1)
    {
        // Entering from the right
        if (q == 0)
            return 1;
        // Exiting to the left
        else if (q == 2)
            return -1;
    }
    // Traversing top-to-bottom
    else if (Constants.direction == 2)
    {
        // Entering from the top
        if (q == 1)
            return 1;
        // Exiting to the bottom
        else if (q == 3)
            return -1;
    }
    // Traversing bottom-to-top
    else
    {
        // Entering from the bottom
        if (q == 3)
            return 1;
        // Exiting to the top
        else if (q == 1)
            return -1;
    }

    // No decision
    return 0;
}

int GetEdgeState2(float2 vec)
{
    float dx = dot(vec, float2(1.0, 0.0));
    float dy = dot(vec, float2(0.0, 1.0));
    
    switch (Constants.direction)
    {
        // Left-to-right
        case 0:
            if (dx < 0.0)
                return 1;
            else if (dx > 0.0)
                return -1;
            break;
        
        // Right-to-left
        case 1:
            if (dx > 0.0)
                return 1;
            else if (dx < 0.0)
                return -1;
            break;
        
        // Top-to-bottom
        case 2:
            if (dy < 0.0)
                return 1;
            else if (dy > 0.0)
                return -1;
            break;
        
        // Bottom-to-top
        case 3:
            if (dy > 0.0)
                return 1;
            else if (dy < 0.0)
                return -1;
            break;
    }
    
    return 0;
}

void ProcessPixel(int2 st, float2 axis, inout TraversalContext context)
{
    
    //int contourId = (int) contourDataTex[st].w;
    //int orientation = Quantize(normal);
    //uint4 d = edgeCrossingTex[st];
    //bool stateChanged = false;
    //bool deleteEdge = false;
    
    float2 normalCenter = contourDataTex[st].xy;
    int edgeStateCenter = GetEdgeState2(normalCenter);
    int edgeStatePrev = 0;
    int edgeStateNext = 0;
    
    if (Constants.direction == 0)
    {
        if (st.x > 0)
            edgeStatePrev = GetEdgeState2(contourDataTex[int2(st.x - 1, st.y)].xy);
        if (st.x < Constants.width-1)
            edgeStateNext = GetEdgeState2(contourDataTex[int2(st.x + 1, st.y)].xy);
    }
    else if (Constants.direction == 1)
    {
        if (st.x > 0)
            edgeStateNext = GetEdgeState2(contourDataTex[int2(st.x - 1, st.y)].xy);
        if (st.x < Constants.width - 1)
            edgeStatePrev = GetEdgeState2(contourDataTex[int2(st.x + 1, st.y)].xy);
    }
    else if (Constants.direction == 2)
    {
        if (st.y > 0)
            edgeStatePrev = GetEdgeState2(contourDataTex[int2(st.x, st.y - 1)].xy);
        if (st.y < Constants.height - 1)
            edgeStateNext = GetEdgeState2(contourDataTex[int2(st.x, st.y + 1)].xy);
    }
    else if (Constants.direction == 3)
    {
        if (st.y > 0)
            edgeStateNext = GetEdgeState2(contourDataTex[int2(st.x, st.y - 1)].xy);
        if (st.y < Constants.height - 1)
            edgeStatePrev = GetEdgeState2(contourDataTex[int2(st.x, st.y + 1)].xy);
    }
    
    int finalEdgeState = 0;
    if (edgeStateCenter == 1)
        finalEdgeState = (edgeStatePrev == 0) ? edgeStateCenter : 0;
    else if(edgeStateCenter == -1)
        finalEdgeState = (edgeStateNext == 0) ? edgeStateCenter : 0;
    
    // Show just the edgeState
    if (finalEdgeState == 1)
        debugTex[st] = float4(1.0, 0.0, 0.0, 0.0);
    else if (finalEdgeState == -1)
        debugTex[st] = float4(0.0, 1.0, 0.0, 0.0);
    else
        debugTex[st] = float4(0.0, 0.0, 0.0, 0.0);
    return;
    
    #if 0
    if (context.state == 0)
    {
        if (edgeState == 1)
        {
            context.state = 1;
            stateChanged = true;
            context.contourId = contourId;
        }
        else if (edgeState == -1)
        {
            context.state = 2;
            stateChanged = true;
            context.contourId = 0;
        }
    }
    else
    {
        TraversalContext prevContext = context;
        
        // If we were outside
        if (prevContext.state == 2)
        {
            // Still outside, do nothing
            if (contourId == 0)
            {
                debugTex[st] = float4(0.0, 0.0, 0.0, 0.0);
                return;
            }
            // Entered a new contour
            else if (edgeState == 1 && contourId > 0)
            {
                context.contourId = contourId;
                context.state = 1;
                context.lastOrientation = orientation;
                stateChanged = 1;
            }
        }
        // If we were inside
        else
        {
            // Contour id is 0, or contour id equals previous, then we're still inside
            if (contourId == 0 || (contourId == prevContext.contourId))
            {
                debugTex[st] = float4(1.0, 0.0, 0.0, 0.0);
                
                context.contourId = prevContext.contourId;
                context.state = 1;
                
                 // If edge state is -1, we're exiting
                context.lastOrientation = (edgeState == -1) ? orientation : prevContext.lastOrientation;
            }
            // Contour id is now 0 but we were exiting
            else if (contourId == 0 && (prevContext.lastOrientation == -1))
            {
                debugTex[st] = float4(1.0, 0.0, 0.0, 0.0);
                
                context.state = 2;
                context.lastOrientation = 0;
                context.contourId = 0;
            }
            // Encountered a new contour, stomp it
            else if (contourId != 0)
            {
                debugTex[st] = float4(1.0, 0.0, 0.0, 0.0);
                context.state = 1;
                context.lastOrientation = prevContext.lastOrientation;
                context.contourId = prevContext.contourId;
            }
        }
    }
#endif
}

[numthreads(64, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadId)
{
    // Left-to-right or right-to-left
    if (Constants.direction == 0 || Constants.direction == 1)
    {
        if (DTid.x >= Constants.height)
            return;
        
        // Check left and right bounds
        int leftCol = LeftBounds[DTid.x];
        int rightCol = RightBounds[DTid.x];
        
        if (leftCol >= rightCol)
            return;
        
        TraversalContext context = (TraversalContext) 0;
        context.contourId = 0;
        context.lastOrientation = 0;
        context.state = 0;
        
        // Left-to-right
        if (Constants.direction == 0)
        {         
            int curRegionId = 0;
            int prevPixelId = -1;
            int lastEdgeId = -1;
            int lastEdgeCol = -1;
            
            for (int col = leftCol; col <= rightCol; ++col)
            {
                const int2 st = int2(col, DTid.x);
                const int edgeId = (int) edgeIdTex[st].r;
                
                bool firstEntered = false;
                bool entered = false;
                bool exited = false;
                
                // First pixel
                if (prevPixelId == -1)
                {
                    if (edgeId > 0)
                    {
                        lastEdgeId = edgeId;
                        lastEdgeCol = col;
                    }
                    curRegionId = edgeId;
                    firstEntered = true;
                }
                else
                {
                    if ((prevPixelId == lastEdgeId) && (edgeId == 0))
                    {
                        if ((lastEdgeCol != -1) && (lastEdgeCol < (col - 1)))
                        {
                            lastEdgeCol = -1;
                            lastEdgeId = -1;
                            curRegionId = 0;
                            exited = true;
                        }
                    }
                    else if ((prevPixelId == 0) && (edgeId > 0))
                    {
                        if (lastEdgeId == -1)
                        {
                            lastEdgeId = edgeId;
                            lastEdgeCol = col;
                            curRegionId = edgeId;
                            entered = true;
                        }
                    }
                }              
                
                //if (curRegion != 0)
                //    debugTex[st] = float4(1.0, 0.0, 0.0, 0.0);
                //else
                //    debugTex[st] = float4(0.0, 0.0, 0.0, 0.0);
                if (firstEntered)
                    debugTex[st] = float4(1.0, 1.0, 0.0, 0.0);
                else if (entered)
                    debugTex[st] = float4(0.0, 1.0, 0.0, 0.0);
                else if(exited)
                    debugTex[st] = float4(1.0, 0.0, 0.0, 0.0);
                
                prevPixelId = edgeId;
                
                //ProcessPixel(st, float2(-1.0, 0.0), context);
            }
        }
        // Right-to-left
        else
        {
            int expectedOrientation = 0;
            for (int col = rightCol; col >= leftCol; --col)
            {
                int2 st = int2(col, DTid.x);
                
                ProcessPixel(st, float2(1.0, 0.0), context);
            }
        }
    }
    // Top-to-bottom or bottom-to-top
    else
    {
        if (DTid.x >= Constants.width)
            return;
        
        int topRow = TopBounds[DTid.x];
        int bottomRow = BottomBounds[DTid.x];
        
        if (topRow >= bottomRow)
            return;
        
        TraversalContext context = (TraversalContext) 0;
        context.contourId = 0;
        context.lastOrientation = 0;
        context.state = 0;
        
        // Top-to-bottom
        if (Constants.direction == 2)
        {
            for (int row = topRow; row <= bottomRow; ++row)
            {
                int2 st = int2(DTid.x, row);
                
                ProcessPixel(st, float2(0.0, -1.0), context);
            }
        }
        // Bottom-to-top
        else
        {
            for (int row = bottomRow; row >= topRow; --row)
            {
                int2 st = int2(DTid.x, row);
                
                ProcessPixel(st, float2(0.0, 1.0), context);
            }
        }
    }
}