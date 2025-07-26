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
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD0;
};


VSOutput main(uint vertexIndex : SV_VertexID)
{
    VSOutput output = (VSOutput) 0;
    
    output.uv = float2((vertexIndex << 1) & 2, vertexIndex & 2);
    output.pos = float4(output.uv * 2.0 - 1.0, 0.0, 1.0);
    
    return output;
}