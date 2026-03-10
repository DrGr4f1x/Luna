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

Texture2D<float4> inputDataTex : BINDING(t0, 0);
Texture2D<uint> inputClassTex : BINDING(t1, 0);
RWTexture2D<float4> outputDataTex : BINDING(u0, 0);
RWTexture2D<uint> outputClassTex : BINDING(u1, 0);

struct Constants
{
    int2 texDimensions;
};

struct TexDims
{
    int2 offsets;
};

ConstantBuffer<Constants> Globals : BINDING(b0, 0);
[[vk::push_constant]]
ConstantBuffer<TexDims> Offsets : register(b1);


bool IsSamplePointValid(int2 samplePt)
{
    return (samplePt.x >= 0 && samplePt.x < Globals.texDimensions.x && samplePt.y >= 0 && samplePt.y < Globals.texDimensions.y);
}


[numthreads(8, 8, 1)]
void main(uint3 DTid : SV_DispatchThreadId)
{
    float4 localData = inputDataTex[DTid.xy]; 
    uint localClass = inputClassTex[DTid.xy];
    
    float bestDist = 10000.0;
    float2 localPos = (float2) DTid.xy;
    
    bool updated = false;
    
    for (int i = -1; i <= 1; ++i)
    {
        for (int j = -1; j <= 1; ++j)
        {
            if(i == 0 && j == 0)
                continue;
            
            int2 sampleCoords = (int2) DTid.xy + int2(i * Offsets.offsets.x, j * Offsets.offsets.y);
            
            if (IsSamplePointValid(sampleCoords))
            {
                float4 sampleData = inputDataTex[sampleCoords];
                uint sampleClass = inputClassTex[sampleCoords];
                
                if (localClass == 0 && sampleClass != 0)
                {
                    localClass = sampleClass;
                    localData = sampleData;
                    
                    updated = true;
                }
                else if (sampleClass != 0)
                {
                    // Distance check
                    float2 sampleVector = localPos - sampleData.xy;
                    float distance = length(localPos - sampleData.xy);
                    if (distance < bestDist && distance > 0.0)
                    {
                        localData = sampleData;
                        bestDist = distance;
                    
                        // Front/back check
                        sampleVector /= distance;
                    
                        if (dot(sampleData.zw, sampleVector) < 0.0)
                        {
                            localClass = 1;
                        }
                        else
                        {
                            localClass = 2;
                        }
                        updated = true;
                    }
                }                
            }
        }
    }

    outputDataTex[DTid.xy] = localData;
    outputClassTex[DTid.xy] = localClass;
}