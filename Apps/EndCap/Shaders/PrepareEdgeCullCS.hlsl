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

Buffer<int> BoundsBuffer : BINDING(t0, 0);

struct EdgeCullWorkItem
{
    uint st;
    uint direction; // 0 = left-to-right, 1 = right-to-left, 2 = top-to-bottom, 3 = bottom-to-top
};

RWStructuredBuffer<EdgeCullWorkItem> WorkQueue : BINDING(u0, 0);

struct Constants_t
{
    int width;
    int height;
};

[[vk::push_constant]]
ConstantBuffer<Constants_t> Constants : register(b0);

[numthreads(64, 1, 1)]
void main(uint3 DTid : SV_GroupThreadID)
{
    if(DTid.x > 0)
        return;
    
    // Determine the first column
    int firstCol = BoundsBuffer[0];
    if (firstCol == Constants.width)
        firstCol = 0;
    
    // Determine the last column
    int lastCol = BoundsBuffer[1];
    if (lastCol == -1)
        lastCol = Constants.width - 1;
    
    // Determine the first row
    int firstRow = BoundsBuffer[2];
    if (firstRow == Constants.height)
        firstRow = 0;
    
    // Determine the last row
    int lastRow = BoundsBuffer[3];
    if (lastRow == -1)
        lastRow = Constants.height - 1;
    
    // Figure out the left-to-right and right-to-left work items
    for (int curRow = firstRow; curRow <= lastRow; curRow += 64)
    {
        // Left-to-right
        EdgeCullWorkItem workItem = (EdgeCullWorkItem) 0;
        workItem.st = (uint) firstCol | (uint) curRow << 16;
        workItem.direction = 0;
        WorkQueue[WorkQueue.IncrementCounter()] = workItem;
        
        // Right-to-left
        workItem.st = (uint) lastCol | (uint) curRow << 16;
        workItem.direction = 1;
        WorkQueue[WorkQueue.IncrementCounter()] = workItem;
    }

    // Figure out the top-to-bottom and bottom-to-top work items
    for (int curCol = firstCol; curCol <= lastCol; curCol += 64)
    {
        // Top-to-bottom
        EdgeCullWorkItem workItem = (EdgeCullWorkItem) 0;
        workItem.st = (uint) curCol | (uint) firstRow << 16;
        workItem.direction = 2;
        WorkQueue[WorkQueue.IncrementCounter()] = workItem;
        
        // Right-to-left
        workItem.st = (uint) curCol | (uint) lastRow << 16;
        workItem.direction = 3;
        WorkQueue[WorkQueue.IncrementCounter()] = workItem;
    }
}