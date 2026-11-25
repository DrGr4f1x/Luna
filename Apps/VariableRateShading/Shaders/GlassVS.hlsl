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

struct VSInput
{
    float3 position     : POSITION;
    float3 normal       : NORMAL;
    float2 uv           : TEXCOORD0;
    float3 tangent      : TANGENT;
};


struct VSOutput
{
    float4 position             : SV_POSITION;
    float4 worldpos             : POSITION;
    float2 uv                   : TEXCOORD0;
    float3 normal               : NORMAL;
    float3 tangent              : TANGENT;
    float4 refractionPosition   : TEXCOORD1;
};

#include "Constants.hlsli"

VSOutput main(VSInput input)
{
    VSOutput result = (VSOutput) 0;

    float4 inputPosition = float4(input.position, 1.0f);

    input.normal.z *= -1.0f;

    float4 newPosition = mul(model, inputPosition);
    result.worldpos = newPosition;

    newPosition = mul(view, newPosition);
    newPosition = mul(projection, newPosition);

    result.position = newPosition;
    result.uv = input.uv;
    result.normal = input.normal;
    result.tangent = input.tangent;

    result.refractionPosition = result.position;

    return result;
}