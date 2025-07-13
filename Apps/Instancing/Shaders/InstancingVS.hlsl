//
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
// Author:  David Elder
//

[[vk::binding(0, 0)]]
cbuffer VSConstants
{
    float4x4 projectionMatrix;
    float4x4 modelViewMatrix;
    float4 lightPos;
    float localSpeed;
    float globalSpeed;
};


struct VSInput
{
	// Vertex attributes
    float3 pos : POSITION;
    float3 normal : NORMAL;
    float4 color : COLOR;
    float2 uv : TEXCOORD;

	// Instanced attributes
    float3 instancePos : INSTANCED_POSITION;
    float3 instanceRot : INSTANCED_ROTATION;
    float instanceScale : INSTANCED_SCALE;
    uint instanceTexIndex : INSTANCED_TEX_INDEX;
};


struct VSOutput
{
    float4 pos : SV_Position;
    float3 color : COLOR;
    float3 normal : NORMAL;
    float3 uv : TEXCOORD0;
    float3 viewVec : TEXCOORD1;
    float3 lightVec : TEXCOORD2;
};


VSOutput main(VSInput input)
{
    VSOutput output = (VSOutput) 0;

    output.color = input.color.rgb;
    output.uv = float3(input.uv, (float) input.instanceTexIndex);

	// Rotate around x
    float s, c;
    sincos(input.instanceRot.x + localSpeed, s, c);
    float3x3 xRotMat = float3x3(
		float3(c, -s, 0.0),
		float3(s, c, 0.0),
		float3(0.0, 0.0, 1.0));

	// Rotate around y
    sincos(input.instanceRot.y + localSpeed, s, c);
    float3x3 yRotMat = float3x3(
		float3(c, 0.0, -s),
		float3(0.0, 1.0, 0.0),
		float3(s, 0.0, c));

	// Rotate around z
    sincos(input.instanceRot.z + localSpeed, s, c);
    float3x3 zRotMat = float3x3(
		float3(1.0, 0.0, 0.0),
		float3(0.0, c, -s),
		float3(0.0, s, c));

    float3x3 localRotMat = mul(zRotMat, mul(yRotMat, xRotMat));

    sincos(input.instanceRot.y + globalSpeed, s, c);
    float4x4 globalRotMat = float4x4(
		float4(c, 0.0, -s, 0.0),
		float4(0.0, 1.0, 0.0, 0.0),
		float4(s, 0.0, c, 0.0),
		float4(0.0, 0.0, 0.0, 1.0));

    float4 localPos = float4(mul(localRotMat, input.pos.xyz), 1.0);
    float4 pos = float4(input.instanceScale * localPos.xyz + input.instancePos, 1.0);

    output.pos = mul(projectionMatrix, mul(modelViewMatrix, mul(globalRotMat, pos)));
    output.normal = mul((float3x3) (mul(modelViewMatrix, globalRotMat)), mul(localRotMat, input.normal));

    pos = mul(modelViewMatrix, float4(input.pos.xyz + input.instancePos, 1.0));
    float3 lpos = mul((float3x3) modelViewMatrix, lightPos.xyz);

    output.lightVec = lpos - pos.xyz;
    output.viewVec = -pos.xyz;

    return output;
}