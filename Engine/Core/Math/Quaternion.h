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

#include "Vector.h"


namespace Math
{

class Quaternion
{
public:
	__forceinline Quaternion() noexcept { m_vec = XMQuaternionIdentity(); }
	__forceinline Quaternion(const Vector3& axis, const Scalar& angle) noexcept { m_vec = XMQuaternionRotationAxis(axis, angle); }
	__forceinline Quaternion(float pitch, float yaw, float roll) noexcept { m_vec = XMQuaternionRotationRollPitchYaw(pitch, yaw, roll); }
	__forceinline explicit Quaternion(const XMMATRIX& matrix) noexcept { m_vec = XMQuaternionRotationMatrix(matrix); }
	__forceinline explicit Quaternion(FXMVECTOR vec) noexcept { m_vec = vec; }
	__forceinline explicit Quaternion(EIdentityTag) noexcept { m_vec = XMQuaternionIdentity(); }

	__forceinline operator XMVECTOR() const noexcept { return m_vec; }

	__forceinline Quaternion operator~() const noexcept { return Quaternion(XMQuaternionConjugate(m_vec)); }
	__forceinline Quaternion operator-() const noexcept { return Quaternion(XMVectorNegate(m_vec)); }

	__forceinline Quaternion operator*(Quaternion rhs) const noexcept { return Quaternion(XMQuaternionMultiply(rhs, m_vec)); }
	__forceinline Vector3 operator*(Vector3 rhs) const noexcept { return Vector3(XMVector3Rotate(rhs, m_vec)); }

	__forceinline Quaternion& operator=(Quaternion rhs) noexcept { m_vec = rhs; return *this; }
	__forceinline Quaternion& operator*=(Quaternion rhs) noexcept { *this = *this * rhs; return *this; }

protected:
	XMVECTOR m_vec;
};

} // namespace Math