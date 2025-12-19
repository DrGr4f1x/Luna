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

#include "Scalar.h"

namespace Math
{

class Vector4;

// A 3-vector with an unspecified fourth component.  Depending on the context, the W can be 0 or 1, but both are implicit.
// The actual value of the fourth component is undefined for performance reasons.
class Vector3
{
public:

	__forceinline Vector3() = default;
	__forceinline Vector3(float x, float y, float z) noexcept { m_vec = XMVectorSet(x, y, z, z); }
	__forceinline Vector3(const XMFLOAT3& v) noexcept { m_vec = XMLoadFloat3(&v); }
	__forceinline Vector3(const Vector3& v) = default;
	__forceinline Vector3(Scalar s) noexcept { m_vec = s; }
	__forceinline explicit Vector3(Vector4 v) noexcept;
	__forceinline explicit Vector3(FXMVECTOR vec) noexcept { m_vec = vec; }
	__forceinline explicit Vector3(EZeroTag) noexcept { m_vec = SplatZero(); }
	__forceinline explicit Vector3(EIdentityTag) noexcept { m_vec = SplatOne(); }
	__forceinline explicit Vector3(EXUnitVector) noexcept { m_vec = CreateXUnitVector(); }
	__forceinline explicit Vector3(EYUnitVector) noexcept { m_vec = CreateYUnitVector(); }
	__forceinline explicit Vector3(EZUnitVector) noexcept { m_vec = CreateZUnitVector(); }

	__forceinline operator XMVECTOR() const noexcept { return m_vec; }
	__forceinline operator XMFLOAT3() const noexcept 
	{
		XMFLOAT3 ret;
		XMStoreFloat3(&ret, m_vec);
		return ret;
	}
	__forceinline operator XMFLOAT4() const noexcept
	{
		XMFLOAT4 ret;
		XMStoreFloat4(&ret, m_vec);
		return ret;
	}

	__forceinline Scalar GetX() const noexcept { return Scalar(XMVectorSplatX(m_vec)); }
	__forceinline Scalar GetY() const noexcept { return Scalar(XMVectorSplatY(m_vec)); }
	__forceinline Scalar GetZ() const noexcept { return Scalar(XMVectorSplatZ(m_vec)); }
	__forceinline void SetX(Scalar x) noexcept { m_vec = XMVectorPermute<4, 1, 2, 3>(m_vec, x); }
	__forceinline void SetY(Scalar y) noexcept { m_vec = XMVectorPermute<0, 5, 2, 3>(m_vec, y); }
	__forceinline void SetZ(Scalar z) noexcept { m_vec = XMVectorPermute<0, 1, 6, 3>(m_vec, z); }

	__forceinline Vector3 operator-() const noexcept { return Vector3(XMVectorNegate(m_vec)); }
	__forceinline Vector3 operator+(Vector3 v2) const noexcept { return Vector3(XMVectorAdd(m_vec, v2)); }
	__forceinline Vector3 operator-(Vector3 v2) const noexcept { return Vector3(XMVectorSubtract(m_vec, v2)); }
	__forceinline Vector3 operator*(Vector3 v2) const noexcept { return Vector3(XMVectorMultiply(m_vec, v2)); }
	__forceinline Vector3 operator/(Vector3 v2) const noexcept { return Vector3(XMVectorDivide(m_vec, v2)); }
	__forceinline Vector3 operator*(Scalar  v2) const noexcept { return *this * Vector3(v2); }
	__forceinline Vector3 operator/(Scalar  v2) const noexcept { return *this / Vector3(v2); }
	__forceinline Vector3 operator*(float  v2) const noexcept { return *this * Scalar(v2); }
	__forceinline Vector3 operator/(float  v2) const noexcept { return *this / Scalar(v2); }

	__forceinline Vector3& operator+=(Vector3 v) noexcept { *this = *this + v; return *this; }
	__forceinline Vector3& operator-=(Vector3 v) noexcept { *this = *this - v; return *this; }
	__forceinline Vector3& operator*=(Vector3 v) noexcept { *this = *this * v; return *this; }
	__forceinline Vector3& operator/=(Vector3 v) noexcept { *this = *this / v; return *this; }

	__forceinline friend Vector3 operator*(Scalar  v1, Vector3 v2) noexcept { return Vector3(v1) * v2; }
	__forceinline friend Vector3 operator/(Scalar  v1, Vector3 v2) noexcept { return Vector3(v1) / v2; }
	__forceinline friend Vector3 operator*(float   v1, Vector3 v2) noexcept { return Scalar(v1) * v2; }
	__forceinline friend Vector3 operator/(float   v1, Vector3 v2) noexcept { return Scalar(v1) / v2; }

protected:
	XMVECTOR m_vec;
};


inline Vector3 operator*(const Vector3& v, float s) noexcept
{
	return s * v;
}


// A 4-vector, completely defined.
class Vector4
{
public:
	__forceinline Vector4() noexcept = default;
	__forceinline Vector4(float x, float y, float z, float w) noexcept { m_vec = XMVectorSet(x, y, z, w); }
	__forceinline Vector4(Vector3 xyz, float w) noexcept { m_vec = XMVectorSetW(xyz, w); }
	__forceinline Vector4(const Vector4& v) noexcept = default;
	__forceinline Vector4(const Scalar& s) noexcept { m_vec = s; }
	__forceinline explicit Vector4(Vector3 xyz) noexcept { m_vec = SetWToOne(xyz); }
	__forceinline explicit Vector4(FXMVECTOR vec) noexcept { m_vec = vec; }
	__forceinline explicit Vector4(EZeroTag) noexcept { m_vec = SplatZero(); }
	__forceinline explicit Vector4(EIdentityTag) noexcept { m_vec = SplatOne(); }
	__forceinline explicit Vector4(EXUnitVector) noexcept { m_vec = CreateXUnitVector(); }
	__forceinline explicit Vector4(EYUnitVector) noexcept { m_vec = CreateYUnitVector(); }
	__forceinline explicit Vector4(EZUnitVector) noexcept { m_vec = CreateZUnitVector(); }
	__forceinline explicit Vector4(EWUnitVector) noexcept { m_vec = CreateWUnitVector(); }

	__forceinline operator XMVECTOR() const noexcept { return m_vec; }
	__forceinline operator XMFLOAT3() const noexcept
	{
		XMFLOAT3 ret;
		XMStoreFloat3(&ret, m_vec);
		return ret;
	}
	__forceinline operator XMFLOAT4() const noexcept
	{
		XMFLOAT4 ret;
		XMStoreFloat4(&ret, m_vec);
		return ret;
	}

	__forceinline Scalar GetX() const noexcept { return Scalar(XMVectorSplatX(m_vec)); }
	__forceinline Scalar GetY() const noexcept { return Scalar(XMVectorSplatY(m_vec)); }
	__forceinline Scalar GetZ() const noexcept { return Scalar(XMVectorSplatZ(m_vec)); }
	__forceinline Scalar GetW() const noexcept { return Scalar(XMVectorSplatW(m_vec)); }
	__forceinline void SetX(Scalar x) noexcept { m_vec = XMVectorPermute<4, 1, 2, 3>(m_vec, x); }
	__forceinline void SetY(Scalar y) noexcept { m_vec = XMVectorPermute<0, 5, 2, 3>(m_vec, y); }
	__forceinline void SetZ(Scalar z) noexcept { m_vec = XMVectorPermute<0, 1, 6, 3>(m_vec, z); }
	__forceinline void SetW(Scalar w) noexcept { m_vec = XMVectorPermute<0, 1, 2, 7>(m_vec, w); }

	__forceinline Vector4 operator-() const noexcept { return Vector4(XMVectorNegate(m_vec)); }
	__forceinline Vector4 operator+(Vector4 v2) const noexcept { return Vector4(XMVectorAdd(m_vec, v2)); }
	__forceinline Vector4 operator-(Vector4 v2) const noexcept { return Vector4(XMVectorSubtract(m_vec, v2)); }
	__forceinline Vector4 operator*(Vector4 v2) const noexcept { return Vector4(XMVectorMultiply(m_vec, v2)); }
	__forceinline Vector4 operator/(Vector4 v2) const noexcept { return Vector4(XMVectorDivide(m_vec, v2)); }
	__forceinline Vector4 operator*(Scalar v2) const noexcept { return *this * Vector4(v2); }
	__forceinline Vector4 operator/(Scalar v2) const noexcept { return *this / Vector4(v2); }
	__forceinline Vector4 operator*(float v2) const noexcept { return *this * Scalar(v2); }
	__forceinline Vector4 operator/(float v2) const noexcept { return *this / Scalar(v2); }

	__forceinline void operator*=(float v2) noexcept { *this = *this * Scalar(v2); }
	__forceinline void operator/=(float v2) noexcept { *this = *this / Scalar(v2); }

	__forceinline friend Vector4 operator*(Scalar v1, Vector4 v2) noexcept { return Vector4(v1) * v2; }
	__forceinline friend Vector4 operator/(Scalar v1, Vector4 v2) noexcept { return Vector4(v1) / v2; }
	__forceinline friend Vector4 operator*(float v1, Vector4 v2) noexcept { return Scalar(v1) * v2; }
	__forceinline friend Vector4 operator/(float v1, Vector4 v2) noexcept { return Scalar(v1) / v2; }

protected:
	XMVECTOR m_vec;
};


__forceinline Vector3::Vector3(Vector4 v) noexcept
{
	Scalar W = v.GetW();
	m_vec = XMVectorSelect(XMVectorDivide(v, W), v, XMVectorEqual(W, SplatZero()));
}


class BoolVector
{
public:
	__forceinline BoolVector(FXMVECTOR vec) noexcept { m_vec = vec; }
	__forceinline operator XMVECTOR() const noexcept { return m_vec; }
protected:
	XMVECTOR m_vec;
};

} // namespace Math