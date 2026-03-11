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
};


struct Model
{
    float3 posOffset;
};
[[vk::push_constant]]
ConstantBuffer<Model> Model : register(b0);


VSOutput main(VSInput input)
{
    VSOutput output = (VSOutput) 0;

    output.pos = input.pos + Model.posOffset;
    output.normal = input.normal;
    output.color = input.color;

    return output;
}