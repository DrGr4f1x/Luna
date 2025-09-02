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
cbuffer ubo : register(b0)
{
    float4x4 projection;
    float4x4 model;
    float4x4 view;
}


struct VertexOutput
{
    float4 position : SV_Position;
    float4 color : COLOR0;
};


static const float4 positions[3] =
{
    float4(0.0, 1.0, 0.0, 1.0),
	float4(-1.0, -1.0, 0.0, 1.0),
	float4(1.0, -1.0, 0.0, 1.0)
};

static const float4 colors[3] =
{
    float4(0.0, 1.0, 0.0, 1.0),
	float4(0.0, 0.0, 1.0, 1.0),
	float4(1.0, 0.0, 0.0, 1.0)
};

struct DummyPayload
{
    uint dummyData;
};

[outputtopology("triangle")]
[numthreads(1, 1, 1)]
void main(in payload DummyPayload payload, out indices uint3 triangles[1], out vertices VertexOutput vertices[3], uint3 DispatchThreadID : SV_DispatchThreadID)
{
    float4x4 mvp = mul(projection, mul(view, model));

    float4 offset = float4(0.0, 0.0, (float) DispatchThreadID, 0.0);

    SetMeshOutputCounts(3, 1);
    
    for (uint i = 0; i < 3; i++)
    {
        vertices[i].position = mul(mvp, positions[i] + offset);
        vertices[i].color = colors[i];
    }

    triangles[0] = uint3(0, 1, 2);
}