//
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
// Author:  David Elder
//

#ifndef CONSTANTS_HLSLI
#define CONSTANTS_HLSLI

#define NUM_LIGHTS 1

struct LightState
{
    float3 position;
    float3 direction;
    float4 color;
    float4 falloff;
    float4x4 view;
    float4x4 projection;
};


cbuffer SceneConstantBuffer : BINDING(b0, 0)
{
    float4x4 model;
    float4x4 view;
    float4x4 projection;
    float4 ambientColor;
    bool sampleShadowMap;
    LightState lights[NUM_LIGHTS];
    float4 viewport;
    float4 clipPlane;
};

#endif