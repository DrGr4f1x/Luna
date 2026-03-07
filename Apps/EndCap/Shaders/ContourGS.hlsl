#include "Common.hlsli"

struct GSInput
{
    float3 pos : POSITION;
    float3 normal : NORMAL;
};


struct GSOutput
{
    float4 pos      : SV_Position;
    float3 normal   : NORMAL;
    float3 color    : COLOR;
};


cbuffer GSConstants : BINDING(b0, 0)
{
    float4x4 projectionMatrix;
    float4x4 modelMatrix;
    float4 plane;
};


[maxvertexcount(6)]
void main(triangle GSInput input[3], inout LineStream<GSOutput> outputStream)
{
    float3 worldPos[3];
    worldPos[0] = mul(modelMatrix, float4(input[0].pos, 1.0));
    worldPos[1] = mul(modelMatrix, float4(input[1].pos, 1.0));
    worldPos[2] = mul(modelMatrix, float4(input[2].pos, 1.0));
    
    float3 normal[3];
    normal[0] = mul((float3x3) modelMatrix, input[0].normal);
    normal[1] = mul((float3x3) modelMatrix, input[1].normal);
    normal[2] = mul((float3x3) modelMatrix, input[2].normal);
    
    float d[3];
    d[0] = dot(worldPos[0], plane.xyz) + plane.w;
    d[1] = dot(worldPos[1], plane.xyz) + plane.w;
    d[2] = dot(worldPos[2], plane.xyz) + plane.w;
    
    int intersectionCount = 0;
    
    GSOutput output[2];
    
    // Check edge 0-1
    if (sign(d[0]) != sign(d[1]))
    {
        float c = d[0] / (d[0] - d[1]);
        float3 interpPoint = lerp(worldPos[0], worldPos[1], c);
        float3 interpNormal = lerp(normal[0], normal[1], c);
        
        output[intersectionCount].pos = mul(projectionMatrix, float4(interpPoint, 1.0f));
        output[intersectionCount].normal = interpNormal;
        output[intersectionCount].color = float4(1.0, 0.0, 0.0, 1.0);
        
        intersectionCount++;
    }

    // Check edge 1-2
    if (sign(d[1]) != sign(d[2]))
    {
        float c = d[1] / (d[1] - d[2]);
        float3 interpPoint = lerp(worldPos[1], worldPos[2], c);
        float3 interpNormal = lerp(normal[1], normal[2], c);
        
        output[intersectionCount].pos = mul(projectionMatrix, float4(interpPoint, 1.0f));
        output[intersectionCount].normal = interpNormal;
        output[intersectionCount].color = float4(1.0, 0.0, 0.0, 1.0);
        
        intersectionCount++;
    }
    
    // Check edge 0-2
    if (intersectionCount < 2 && sign(d[0]) != sign(d[2]))
    {
        float c = d[0] / (d[0] - d[2]);
        float3 interpPoint = lerp(worldPos[0], worldPos[2], c);
        float3 interpNormal = lerp(normal[0], normal[2], c);
        
        output[intersectionCount].pos = mul(projectionMatrix, float4(interpPoint, 1.0f));
        output[intersectionCount].normal = interpNormal;
        output[intersectionCount].color = float4(1.0, 0.0, 0.0, 1.0);
        
        intersectionCount++;
    }
    
    if (intersectionCount == 2)
    {
        outputStream.Append(output[0]);
        outputStream.Append(output[1]);

        outputStream.RestartStrip();
    }
}