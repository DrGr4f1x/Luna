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
    float3 position : POSITION;
    float3 normal   : NORMAL;
    float4 color    : COLOR;
    float2 uv       : TEXCOORD;
};


struct VSOutput
{
    float4 position : SV_Position;
    float3 normal   : NORMAL;
    float3 color    : COLOR;
    float3 viewVec  : TEXCOORD0;
    float3 lightVec : TEXCOORD1;
    float3 worldPos : TEXCOORD2;
};


struct SceneConstants
{
    float4x4 projection;
    float4x4 view;
    float4x4 model;
    float3 lightPos;
};

ConstantBuffer<SceneConstants> sceneConstants : BINDING(b0, 0);


VSOutput main(VSInput input)
{
    VSOutput output = (VSOutput)0;
    
    output.color = input.color.xyz;
    output.position = mul(sceneConstants.projection, mul(sceneConstants.view, mul(sceneConstants.model, float4(input.position, 1.0))));
    output.normal = mul((float3x3) sceneConstants.model, input.normal);
    output.lightVec = normalize(sceneConstants.lightPos - input.position);
    
    float4 pos = mul(sceneConstants.model, float4(input.position, 1.0));
    output.viewVec = -pos.xyz;
    output.worldPos = pos.xyz;
    
    return output;
}