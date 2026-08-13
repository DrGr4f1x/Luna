//
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
// Author:  David Elder
//

#include "Constants.hlsli"

#define SHADOW_DEPTH_BIAS 0.00005f


// Sample normal map, convert to signed, apply tangent-to-world space transform.
float3 CalcPerPixelNormal(float2 texCoord, float3 vertexNormal, float3 vertexTangent)
{
    // Compute per-pixel normal.
    float3 bumpNormal = (float3)normalMap.Sample(sampleWrap, texCoord);
    bumpNormal = 2.0f * bumpNormal - 1.0f;

    // Compute tangent frame.
    vertexNormal = normalize(vertexNormal);
    vertexTangent = normalize(vertexTangent);

    float3 vertexBinormal = normalize(cross(vertexTangent, vertexNormal));
    float3x3 tangentToWorldMatrix = transpose(float3x3(vertexTangent, vertexBinormal, vertexNormal));

    return mul(tangentToWorldMatrix, bumpNormal);
}


// Diffuse lighting calculation, with angle and distance falloff.
float4 CalcLightingColor(float3 lightPos, float3 lightDir, float4 color, float4 falloffs, float3 worldPos, float3 pixelNormal)
{
    float3 lightToPixelDir = worldPos - lightPos;

    // Dist falloff = 0 at vFalloffs.x, 1 at vFalloffs.x - vFalloffs.y
    float dist = length(lightToPixelDir);

    float distFalloff = saturate((falloffs.x - dist) / falloffs.y);

    // Normalize from here on.
    float3 lightToPixelDirNormalized = lightToPixelDir / dist;

    // Angle falloff = 0 at vFalloffs.z, 1 at vFalloffs.z - vFalloffs.w
    float cosAngle = dot(lightToPixelDirNormalized, lightDir / length(lightDir));
    float angleFalloff = saturate((cosAngle - falloffs.z) / falloffs.w);

    // Diffuse contribution.
    float nDotL = saturate(-dot(lightToPixelDirNormalized, pixelNormal));

    // Ignore angle falloff for a point light.
    angleFalloff = 1.0f;

    return color * nDotL * distFalloff * angleFalloff;
}


// Test how much pixel is in shadow, using 2x2 percentage-closer filtering.
float4 CalcUnshadowedAmountPCF2x2(int lightIndex, float4 worldPos)
{
    // Compute pixel position in light space.
    float4 lightSpacePos = worldPos;
    lightSpacePos = mul(lights[lightIndex].view, lightSpacePos);
    lightSpacePos = mul(lights[lightIndex].projection, lightSpacePos);

    lightSpacePos.xyz /= lightSpacePos.w;

    // Translate from homogeneous coords to texture coords.
    float2 shadowTexCoord = 0.5f * lightSpacePos.xy + 0.5f;
    shadowTexCoord.y = 1.0f - shadowTexCoord.y;

    // Depth bias to avoid pixel self-shadowing.
    float lightSpaceDepth = lightSpacePos.z - SHADOW_DEPTH_BIAS;

    // Find sub-pixel weights.
    float2 shadowMapDims = float2(viewport.x, viewport.y); //float2(1280.0f, 720.0f);             // need to keep in sync with .cpp file
    float4 subPixelCoords = float4(1.0f, 1.0f, 1.0f, 1.0f);
    subPixelCoords.xy = frac(shadowMapDims * shadowTexCoord);
    subPixelCoords.zw = 1.0f - subPixelCoords.xy;
    float4 bilinearWeights = subPixelCoords.zxzx * subPixelCoords.wwyy;

    // 2x2 percentage closer filtering.
    float2 texelUnits = 1.0f / shadowMapDims;
    float4 shadowDepths;
    if (shadowTexCoord.x <= 0.0f || shadowTexCoord.x >= 1.0f || shadowTexCoord.y <= 0.0f || shadowTexCoord.y >= 1.0f || lightSpaceDepth > 1.0f)
    {
        // Hack for a point light with a shadow map only for a single face.
        // Don't apply any shadow outside the shadow map.
        return float4(1.0, 1.0, 1.0, 1.0);
    }
    else
    {
        shadowDepths.x = shadowMap.Sample(sampleClamp, shadowTexCoord).x;
        shadowDepths.y = shadowMap.Sample(sampleClamp, shadowTexCoord + float2(texelUnits.x, 0.0f)).x;
        shadowDepths.z = shadowMap.Sample(sampleClamp, shadowTexCoord + float2(0.0f, texelUnits.y)).x;
        shadowDepths.w = shadowMap.Sample(sampleClamp, shadowTexCoord + texelUnits).x;
    }
    // What weighted fraction of the 4 samples are nearer to the light than this pixel?
    float4 shadowTests = select(shadowDepths >= lightSpaceDepth, 1.0f, 0.0f);
    return dot(bilinearWeights, shadowTests);
}