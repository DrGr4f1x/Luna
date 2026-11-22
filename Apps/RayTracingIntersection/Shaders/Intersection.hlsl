//
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
// Author:  David Elder
//

#include "RayTracingCommon.hlsli"


// Ray-sphere intersection
// By Inigo Quilez, from https://iquilezles.org/articles/spherefunctions/
float IntersectSphere(const Sphere s, float3 ro, float3 rd)
{
    float3 oc = ro - s.center;
    float b = dot(oc, rd);
    float c = dot(oc, oc) - s.radius * s.radius;
    float h = b * b - c;
    if (h < 0.0)
    {
        return -1.0;
    }
    h = sqrt(h);
    return -b - h;
}


[shader("intersection")]
void intersection_main()
{
    Sphere sphere = spheres[PrimitiveIndex()];
    float hit = IntersectSphere(sphere, WorldRayOrigin(), WorldRayDirection());

    if (hit > 0)
    {
        ProceduralPrimitiveAttributes attr = (ProceduralPrimitiveAttributes)0;
        attr.dummy = 0.0;
        ReportHit(hit, 0, attr);
    }
}