//
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
// Author:  David Elder
//

struct PSInput
{
    float4 pos : SV_Position;
    float3 normal : NORMAL;
    float3 color : COLOR;
    float3 lightVec : TEXCOORD;
};


float4 main(PSInput input) : SV_TARGET
{
    float3 N = normalize(input.normal);
    float3 L = normalize(input.lightVec);

    float intensity = dot(N, L);
    float3 color = 0.0.xxx;

    if (intensity > 0.98)
        color = input.color * 1.5;
    else if (intensity > 0.9)
        color = input.color;
    else if (intensity > 0.5)
        color = input.color * 0.6;
    else if (intensity > 0.25)
        color = input.color * 0.4;
    else
        color = input.color * 0.2;

    float3 desatColor = dot(float3(0.2126, 0.7152, 0.0722), color);
    color = lerp(color, desatColor, 0.1);
    return float4(color, 1.0);
}