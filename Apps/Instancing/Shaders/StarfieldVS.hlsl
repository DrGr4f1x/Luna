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
    float4 pos : SV_Position;
    float3 uvw : TEXCOORD0;
};


VSOutput main(uint vertId : SV_VertexID)
{
    VSOutput output = (VSOutput) 0;

    output.uvw = float3((vertId << 1) & 2, vertId & 2, vertId & 2);
    output.pos = float4(output.uvw.xy * 2.0 - 1.0, 0.0, 1.0);

    return output;
}