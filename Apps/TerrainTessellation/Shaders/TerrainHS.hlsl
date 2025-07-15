//
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
// Author:  David Elder
//

#define INPUT_PATCH_SIZE 4
#define OUTPUT_PATCH_SIZE 4

struct HSInput
{
    float4 pos : POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD;
};


struct HSConstantOutput
{
    float edgeTesselation[4] : SV_TessFactor;
    float insideTesselation[2] : SV_InsideTessFactor;
};


[[vk::binding(0, 0)]]
cbuffer HSConstants : register(b0)
{
    float4x4 projectionMatrix;
    float4x4 modelViewMatrix;
    float4 lightPos;
    float4 frustumPlanes[6];
    float2 viewportDim;
    float displacementFactor;
    float tessellationFactor;
    float tessellatedEdgeSize;
};


[[vk::binding(1, 0)]]
Texture2D heightTex : register(t1);

[[vk::binding(0, 1)]]
SamplerState linearSamplerMirror : register(s0);


// Calculate the tessellation factor based on screen space
// dimensions of the edge
float ScreenSpaceTessFactor(float4 p0, float4 p1)
{
	// Calculate edge mid point
    float4 midPoint = 0.5 * (p0 + p1);
	// Sphere radius as distance between the control points
    float radius = distance(p0, p1) / 2.0;

	// View space
    float4 v0 = mul(modelViewMatrix, midPoint);

	// Project into clip space
    float4 clip0 = mul(projectionMatrix, (v0 - float4(radius, 0.0.xxx)));
    float4 clip1 = mul(projectionMatrix, (v0 + float4(radius, 0.0.xxx)));

	// Get normalized device coordinates
    clip0 /= clip0.w;
    clip1 /= clip1.w;

	// Convert to viewport coordinates
    clip0.xy *= viewportDim;
    clip1.xy *= viewportDim;

	// Return the tessellation factor based on the screen size 
	// given by the distance of the two edge control points in screen space
	// and a reference (min.) tessellation size for the edge set by the application
    return clamp(distance(clip0, clip1) / tessellatedEdgeSize * tessellationFactor, 1.0, 64.0);
}


// Checks the current's patch visibility against the frustum using a sphere check
// Sphere radius is given by the patch size
bool FrustumCheck(float4 pos, float2 uv)
{
	// Fixed radius (increase if patch size is increased in example)
    const float radius = 8.0f;
    pos.y = heightTex.SampleLevel(linearSamplerMirror, uv, 0.0).r * displacementFactor;

	// Check sphere against frustum planes
    for (int i = 0; i < 6; i++)
    {
        if (dot(pos, frustumPlanes[i]) + radius < 0.0)
        {
            return false;
        }
    }
    return true;
}


HSConstantOutput HSConstantMain(InputPatch<HSInput, INPUT_PATCH_SIZE> inputPatch)
{
    HSConstantOutput output = (HSConstantOutput) 0;

    if (!FrustumCheck(inputPatch[0].pos, inputPatch[0].uv))
    {
        output.edgeTesselation[0] = 0.0;
        output.edgeTesselation[1] = 0.0;
        output.edgeTesselation[2] = 0.0;
        output.edgeTesselation[3] = 0.0;
        output.insideTesselation[0] = 0.0;
        output.insideTesselation[1] = 0.0;
    }
    else
    {
        if (tessellationFactor > 0.0)
        {
            output.edgeTesselation[0] = ScreenSpaceTessFactor(inputPatch[3].pos, inputPatch[0].pos);
            output.edgeTesselation[1] = ScreenSpaceTessFactor(inputPatch[0].pos, inputPatch[1].pos);
            output.edgeTesselation[2] = ScreenSpaceTessFactor(inputPatch[1].pos, inputPatch[2].pos);
            output.edgeTesselation[3] = ScreenSpaceTessFactor(inputPatch[2].pos, inputPatch[3].pos);
            output.insideTesselation[0] = lerp(output.edgeTesselation[0], output.edgeTesselation[3], 0.5);
            output.insideTesselation[1] = lerp(output.edgeTesselation[2], output.edgeTesselation[1], 0.5);
        }
        else
        {
            output.edgeTesselation[0] = 1.0;
            output.edgeTesselation[1] = 1.0;
            output.edgeTesselation[2] = 1.0;
            output.edgeTesselation[3] = 1.0;
            output.insideTesselation[0] = 1.0;
            output.insideTesselation[1] = 1.0;
        }
    }

    return output;
}


struct HSOutput
{
    float4 pos : BEZIERPOS;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD;
};


[domain("quad")]
[partitioning("integer")]
[outputtopology("triangle_cw")]
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