//
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
// Author:  David Elder
//

#include "Stdafx.h"

#include "BoundingBox.h"


using namespace DirectX;
using namespace std;


namespace Math
{

static const Vector3 s_boxCorners[] =
{
	{ -1.0f, -1.0f,  1.0f },
	{  1.0f, -1.0f,  1.0f },
	{  1.0f,  1.0f,  1.0f },
	{ -1.0f,  1.0f,  1.0f },
	{ -1.0f, -1.0f, -1.0f },
	{  1.0f, -1.0f, -1.0f },
	{  1.0f,  1.0f, -1.0f },
	{ -1.0f,  1.0f, -1.0f }
};


BoundingBox BoundingBoxUnion(const vector<BoundingBox>& boxes) noexcept
{
	float maxF = numeric_limits<float>::max();
	Vector3 minExtents(maxF, maxF, maxF);
	Vector3 maxExtents(-maxF, -maxF, -maxF);

	for (const auto& box : boxes)
	{
		minExtents = Min(minExtents, box.GetMin());
		maxExtents = Max(maxExtents, box.GetMax());
	}

	return BoundingBoxFromMinMax(minExtents, maxExtents);
}


BoundingBox operator*(Matrix4 mat, BoundingBox box) noexcept
{
	// Load center and extents.
	Vector3 center = box.GetCenter();
	Vector3 extents = box.GetExtents();

	// Compute and transform the corners and find new min/max bounds.
	Vector3 corner = MultiplyAdd(extents, s_boxCorners[0], center);
	corner = mat * corner;

	Vector3 minExtents, maxExtents;
	minExtents = maxExtents = corner;

	for (size_t i = 1; i < 8; ++i)
	{
		corner = MultiplyAdd(extents, s_boxCorners[i], center);
		corner = mat * corner;

		minExtents = Min(minExtents, corner);
		maxExtents = Max(maxExtents, corner);
	}

	return BoundingBoxFromMinMax(Vector3(minExtents), Vector3(maxExtents));
}

} // namespace Math