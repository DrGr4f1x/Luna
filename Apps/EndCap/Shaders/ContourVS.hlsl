//
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
// Author:  David Elder
//

struct VSInput
{
    float3 pos      : POSITION;
    float3 normal   : NORMAL;
    float4 color    : COLOR;
};


struct VSOutput
{
    float3 pos      : POSITION;
    float3 normal   : NORMAL;
    float4 color    : COLOR;
    float id        : TEXCOORD;
};


struct PerModelData
{
    float4 posOffsetId;
};
[[vk::push_constant]]
ConstantBuffer<PerModelData> Model : register(b0);


VSOutput main(VSInput input)
{
    VSOutput output = (VSOutput) 0;

    output.pos = input.pos + Model.posOffsetId.xyz;
    output.normal = input.normal;
    output.color = input.color;
    output.id = Model.posOffsetId.w;

    return output;
}