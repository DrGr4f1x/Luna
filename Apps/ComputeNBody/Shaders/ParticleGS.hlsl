//
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
// Author:  David Elder
//

struct GSInput
{
    float4 pos : POSITION;
    float gradientPos : TEXCOORD0;
    float pointSize : TEXCOORD1;
};


struct GSOutput
{
    float4 pos : SV_Position;
    float2 uv : TEXCOORD0;
    float gradientPos : TEXCOORD1;
};


[[vk::binding(0, 1)]]
cbuffer GSConstants : register(b0)
{
    float4x4 projectionMatrix;
    float4x4 modelViewMatrix;
    float4x4 invViewMatrix;
    float2 screenDim;
};


[maxvertexcount(4)]
void main(point GSInput input[1], inout TriangleStream<GSOutput> spriteStream)
{
    static float3 positions[4] =
    {
        float3(0.5, -0.5, 0.0),
		float3(0.5, 0.5, 0.0),
		float3(-0.5, -0.5, 0.0),
		float3(-0.5, 0.5, 0.0),
    };

    static float2 texcoords[4] =
    {
        float2(1.0, 1.0),
		float2(1.0, 0.0),
		float2(0.0, 1.0),
		float2(0.0, 0.0),
    };

    GSOutput output = (GSOutput) 0;
    float4 pos = input[0].pos;

    float2 viewportScale = float2(1.0 / screenDim.x, 1.0 / screenDim.y) * pos.w;
    float2 scale = viewportScale * input[0].pointSize;

	// Emit two new triangles
    for (int i = 0; i < 4; ++i)
    {
        float3 position = float3(positions[i].xy * scale, positions[i].z) + pos.xyz;

        output.pos = float4(position, pos.w);
        output.uv = texcoords[i];
        output.gradientPos = input[0].gradientPos;

        spriteStream.Append(output);
    }
    spriteStream.RestartStrip();
}