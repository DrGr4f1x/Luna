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
    float3 viewVec : TEXCOORD0;
    float3 lightVec : TEXCOORD1;
};


float4 main(PSInput input) : SV_TARGET
{
    float3 N = normalize(input.normal);
    float3 L = normalize(input.lightVec);
    float3 V = normalize(input.viewVec);
    float3 R = reflect(-L, N);
    float3 diffuse = max(dot(N, L), 0.0) * input.color;
    float3 specular = pow(max(dot(R, V), 0.0), 8.0) * float3(0.75, 0.75, 0.75);
    return float4(diffuse + specular, 0.5);
}