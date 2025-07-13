//
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
// Author:  David Elder
//

#define INPUT_PATCH_SIZE 3
#define OUTPUT_PATCH_SIZE 3

struct HSInput
{
    float4 pos : POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD;
};


struct HSConstantOutput
{
    float edgeTesselation[3] : SV_TessFactor;
    float insideTesselation[1] : SV_InsideTessFactor;
};


[[vk::binding(0,0)]]
cbuffer HSConstants : register(b0)
{
    float tessLevel;
};


HSConstantOutput HSConstantMain(InputPatch<HSInput, INPUT_PATCH_SIZE> inputPatch, uint patchId : SV_PrimitiveID)
{
    HSConstantOutput output = (HSConstantOutput) 0;

    output.edgeTesselation[0] = tessLevel;
    output.edgeTesselation[1] = tessLevel;
    output.edgeTesselation[2] = tessLevel;
    output.insideTesselation[0] = tessLevel;

    return output;
}


struct HSOutput
{
    float4 pos : BEZIERPOS;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD;
};


[domain("tri")]
[partitioning("integer")]
[outputtopology("triangle_ccw")]
[outputcontrolpoints(OUTPUT_PATCH_SIZE)]
[patchconstantfunc("HSConstantMain")]
HSOutput main(InputPatch<HSInput, INPUT_PATCH_SIZE> inputPatch, uint i : SV_OutputControlPointID)
{
    HSOutput output = (HSOutput) 0;

    output.pos = inputPatch[i].pos;
    output.normal = inputPatch[i].normal;
    output.uv = inputPatch[i].uv;

    return output;
}