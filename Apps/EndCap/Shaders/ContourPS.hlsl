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
    float4 pos      : SV_Position;
    float3 normal   : NORMAL;
    float3 color    : COLOR;
    float4 wpos     : TEXCOORD;
};


struct PSOutput
{
    float4 color    : SV_TARGET0;
    float4 normal   : SV_TARGET1;
};


cbuffer PSConstants : BINDING(b0, 1)
{
    float4x4 modelViewMatrix;
    float4 worldUpVector;
    float3 viewPos;
};


PSOutput main(PSInput input)
{
   PSOutput output = (PSOutput) 0;
    
    output.color = float4(input.color, 1.0);
    
    float3 normal = normalize(float3(input.normal.xy, 0.0));
    normal.y = -normal.y;
    output.normal = float4(normal, 1.0);
    
    return output;
}