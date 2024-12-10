//
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
// Author:  David Elder
//

#pragma once

#include "Matrix3.h"
#include "Vector.h"


namespace Math
{

class BoundingBox
{
public:
	BoundingBox() = default;
	BoundingBox(Vector3 center, Vector3 extents)
		: m_center(center)
		, m_extents(extents)
	{}

	Vector3 GetCenter() const noexcept { return m_center; }
	Vector3 GetExtents() const noexcept { return m_extents; }

	Vector3 GetMin() const noexcept { return m_center - m_extents; }
	Vector3 GetMax() const noexcept { return m_center + m_extents; }

private:
	Vector3 m_center{ Math::kZero };
	Vector3 m_extents{ Math::kOne };
};


inline BoundingBox BoundingBoxFromMinMax(Vector3 minExtents, Vector3 maxExtents) noexcept
{
	Vector3 center = 0.5f * (maxExtents + minExtents);
	Vector3 extents = maxExtents - center;

	return BoundingBox(center, extents);
}


BoundingBox BoundingBoxUnion(const std::vector<BoundingBox>& boxes) noexcept;
BoundingBox operator*(Matrix4 mat, BoundingBox box) noexcept;

} // namespace Math
