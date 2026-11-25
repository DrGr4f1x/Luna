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


cbuffer SceneConstants : BINDING(b0, 0)
{
	float4x4 viewMatrix;
	float4x4 projectionMatrix;
};


struct PushConstants
{
	float4x4 modelMatrix;
	float4 color;
};

VK_PUSH_CONSTANT
ConstantBuffer<PushConstants> pushConstants : register(b1);


struct VSInput
{
	float3 pos : POSITION;
};


struct VSOutput
{
	float4 pos : SV_POSITION;
    float viewDepth : TEXCOORD0;
};


VSOutput main(VSInput input)
{
	VSOutput output = (VSOutput)0;

    float4 viewPos = mul(viewMatrix, mul(pushConstants.modelMatrix, float4(input.pos, 1.0)));
	output.pos = mul(projectionMatrix, viewPos);
    output.viewDepth = -viewPos.z; // RH, camera looks down -Z

	return output;
}