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
};


VSOutput main(VSInput input)
{
	VSOutput output = (VSOutput)0;

	output.pos = mul(projectionMatrix, mul(viewMatrix, mul(pushConstants.modelMatrix, float4(input.pos, 1.0))));

	return output;
}