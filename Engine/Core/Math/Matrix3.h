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

#include "Quaternion.h"


namespace Math
{

// Represents a 3x3 matrix while occupying a 4x4 memory footprint.  The unused row and column are undefined but implicitly
// (0, 0, 0, 1).  Constructing a Matrix4 will make those values explicit.
__declspec(align(16)) class Matrix3
{
public:
	__forceinline Matrix3() noexcept = default;
	__forceinline Matrix3(Vector3 x, Vector3 y, Vector3 z) noexcept { m_mat[0] = x; m_mat[1] = y; m_mat[2] = z; }
	__forceinline Matrix3(const Matrix3& m) noexcept = default;
	__forceinline Matrix3(Quaternion q) noexcept { *this = Matrix3(XMMatrixRotationQuaternion(q)); }
	__forceinline explicit Matrix3(const XMMATRIX& m) noexcept { m_mat[0] = Vector3(m.r[0]); m_mat[1] = Vector3(m.r[1]); m_mat[2] = Vector3(m.r[2]); }
	__forceinline explicit Matrix3(EIdentityTag) noexcept { m_mat[0] = Vector3(kXUnitVector); m_mat[1] = Vector3(kYUnitVector); m_mat[2] = Vector3(kZUnitVector); }
	__forceinline explicit Matrix3(EZeroTag) noexcept { m_mat[0] = m_mat[1] = m_mat[2] = Vector3(kZero); }

	__forceinline void SetX(Vector3 x) noexcept { m_mat[0] = x; }
	__forceinline void SetY(Vector3 y) noexcept { m_mat[1] = y; }
	__forceinline void SetZ(Vector3 z) noexcept { m_mat[2] = z; }

	__forceinline Vector3 GetX() const noexcept { return m_mat[0]; }
	__forceinline Vector3 GetY() const noexcept { return m_mat[1]; }
	__forceinline Vector3 GetZ() const noexcept { return m_mat[2]; }

	static __forceinline Matrix3 MakeXRotation(float angle) noexcept { return Matrix3(XMMatrixRotationX(angle)); }
	static __forceinline Matrix3 MakeYRotation(float angle) noexcept { return Matrix3(XMMatrixRotationY(angle)); }
	static __forceinline Matrix3 MakeZRotation(float angle) noexcept { return Matrix3(XMMatrixRotationZ(angle)); }
	static __forceinline Matrix3 MakeScale(float scale) noexcept { return Matrix3(XMMatrixScaling(scale, scale, scale)); }
	static __forceinline Matrix3 MakeScale(float sx, float sy, float sz) noexcept { return Matrix3(XMMatrixScaling(sx, sy, sz)); }
	static __forceinline Matrix3 MakeScale(Vector3 scale) noexcept { return Matrix3(XMMatrixScalingFromVector(scale)); }

	__forceinline operator XMMATRIX() const noexcept { return (const XMMATRIX&)m_mat; }
	__forceinline operator XMFLOAT3X3() const noexcept
	{
		XMFLOAT3X3 ret;
		XMStoreFloat3x3(&ret, *this);
		return ret;
	}

	__forceinline Vector3 operator*(Vector3 vec) const noexcept { return Vector3(XMVector3TransformNormal(vec, *this)); }
	__forceinline Matrix3 operator*(const Matrix3& mat) const noexcept { return Matrix3(*this * mat.GetX(), *this * mat.GetY(), *this * mat.GetZ()); }

private:
	Vector3 m_mat[3];
};

} // namespace Math