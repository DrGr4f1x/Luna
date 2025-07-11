//
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
// Author:  David Elder
//

struct VSOutput
{
    float4 position : SV_Position;
    float2 texcoord : TEXCOORD0;
};


VSOutput main(uint vertId : SV_VertexID)
{
    VSOutput output = (VSOutput) 0;

    output.texcoord = float2((vertId << 1) & 2, vertId & 2);
    output.position = float4(output.texcoord * 2.0f - 1.0f, 0.0f, 1.0f);

    output.position.y = -output.position.y;

    return output;
}