//
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
// Author:  David Elder
//

struct DSInput
{
    float4 pos : SV_POSITION;
    float3 normal : NORMAL0;
    float3 color : COLOR0;
    float3 viewVec : TEXCOORD0;
    float3 lightVec : TEXCOORD1;
};


struct HSConstantOutput
{
    float tessLevelOuter[3] : SV_TessFactor;
    float tessLevelInner : SV_InsideTessFactor;
};


struct DSOutput
{
    float4 pos : SV_POSITION;
    float3 normal : NORMAL0;
    float3 color : COLOR0;
    float3 viewVec : TEXCOORD0;
    float3 lightVec : TEXCOORD1;
};


[domain("tri")]
DSOutput main(HSConstantOutput input, float3 tessCoord : SV_DomainLocation, const OutputPatch<DSInput, 3> patch)
{
    DSOutput output = (DSOutput) 0;
    
    output.pos = (tessCoord.x * patch[2].pos) + (tessCoord.y * patch[1].pos) + (tessCoord.z * patch[0].pos);
    output.normal = tessCoord.x * patch[2].normal + tessCoord.y * patch[1].normal + tessCoord.z * patch[0].normal;
    output.viewVec = tessCoord.x * patch[2].viewVec + tessCoord.y * patch[1].viewVec + tessCoord.z * patch[0].viewVec;
    output.lightVec = tessCoord.x * patch[2].lightVec + tessCoord.y * patch[1].lightVec + tessCoord.z * patch[0].lightVec;
    output.color = patch[0].color;
    
    return output;
}