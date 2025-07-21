//
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
// Author:  David Elder
//

struct HSInput
{
    float4 pos : SV_Position;
    float3 normal : NORMAL;
    float3 color : COLOR;
    float3 viewVec : TEXCOORD0;
    float3 lightVec : TEXCOORD1;
};


struct HSOutput
{
    float4 pos : SV_Position;
    float3 normal : NORMAL;
    float3 color : COLOR;
    float3 viewVec : TEXCOORD0;
    float3 lightVec : TEXCOORD1;
};


struct HSConstantOutput
{
    float tessLevelOuter[3] : SV_TessFactor;
    float tessLevelInner : SV_InsideTessFactor;
};


HSConstantOutput HSConstantMain(InputPatch<HSInput, 3> inputPatch, uint patchId : SV_PrimitiveID)
{
    HSConstantOutput output = (HSConstantOutput) 0;

    output.tessLevelOuter[0] = 1.0;
    output.tessLevelOuter[1] = 1.0;
    output.tessLevelOuter[2] = 1.0;
    output.tessLevelInner = 2.0;

    return output;
}


[domain("tri")]
[partitioning("integer")]
[outputtopology("triangle_ccw")]
[outputcontrolpoints(3)]
[patchconstantfunc("HSConstantMain")]
[maxtessfactor(20.0f)]
HSOutput main(InputPatch<HSInput, 3> inputPatch, uint i : SV_OutputControlPointID)
{
    HSOutput output = (HSOutput) 0;
    
    output.pos = inputPatch[i].pos;
    output.normal = inputPatch[i].normal;
    output.color = inputPatch[i].color;
    output.viewVec = inputPatch[i].viewVec;
    output.lightVec = inputPatch[i].lightVec;
    
    return output;
}