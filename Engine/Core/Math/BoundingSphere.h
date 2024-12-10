//
// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
// Developed by Minigraph
//
// Author:  James Stanard 
//

#pragma once


#include "BoundingBox.h"
#include "Vector.h"


namespace Math
{

class BoundingSphere
{
public:
	BoundingSphere() = default;
	BoundingSphere(Vector3 center, Scalar radius) noexcept;
	explicit BoundingSphere(Vector4 sphere) noexcept;
	explicit BoundingSphere(const BoundingBox& box) noexcept;

	Vector3 GetCenter() const noexcept;
	Scalar GetRadius() const noexcept;

private:
	Vector4 m_repr;
};


//=======================================================================================================
// Inline implementations
//

inline BoundingSphere::BoundingSphere(Vector3 center, Scalar radius) noexcept
{
	m_repr = Vector4(center);
	m_repr.SetW(radius);
}


inline BoundingSphere::BoundingSphere(Vector4 sphere) noexcept
	: m_repr(sphere)
{}


inline BoundingSphere::BoundingSphere(const BoundingBox& box) noexcept
{
	m_repr = Vector4(box.GetCenter());
	m_repr.SetW(Length(box.GetExtents()));
}


inline Vector3 BoundingSphere::GetCenter() const noexcept
{
	return Vector3(m_repr);
}


inline Scalar BoundingSphere::GetRadius() const noexcept
{
	return m_repr.GetW();
}


} // namespace Math