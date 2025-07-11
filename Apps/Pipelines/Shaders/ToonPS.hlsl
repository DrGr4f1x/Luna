//
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
// Author:  David Elder
//

struct VSInput
{
    float4 pos : SV_Position;
    float3 normal : NORMAL;
    float3 color : COLOR;
    float2 uv : TEXCOORD0;
    float3 viewVec : TEXCOORD1;
    float3 lightVec : TEXCOORD2;
};


float4 main(VSInput input) : SV_TARGET
{
	// Desaturate color
    float3 color = lerp(input.color, dot(float3(0.2126, 0.7152, 0.0722), input.color).xxx, 0.65);

	// High ambient colors because mesh materials are pretty dark
    float3 ambient = color;
    float3 N = normalize(input.normal);
    float3 L = normalize(input.lightVec);
    float3 V = normalize(input.viewVec);
    float3 R = reflect(-L, N);
    float3 diffuse = max(dot(N, L), 0.0) * color;
    float3 specular = pow(max(dot(R, V), 0.0), 16.0) * 0.75.xxx;
    float4 outColor = float4(ambient + diffuse * 1.75 + specular, 1.0);

    float intensity = dot(N, L);
    float shade = 1.0;
    shade = intensity < 0.5 ? 0.75 : shade;
    shade = intensity < 0.35 ? 0.6 : shade;
    shade = intensity < 0.25 ? 0.5 : shade;
    shade = intensity < 0.1 ? 0.25 : shade;

    outColor.rgb = input.color * 3.0 * shade;
    return outColor;
}