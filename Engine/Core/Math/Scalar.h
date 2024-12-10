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

#include "CommonMath.h"

namespace Math
{

class Scalar
{
public:
	__forceinline Scalar() = default;
	__forceinline Scalar(const Scalar& s) = default;
	__forceinline Scalar(float f) noexcept { m_vec = XMVectorReplicate(f); }
	__forceinline explicit Scalar(FXMVECTOR vec) noexcept { m_vec = vec; }
	__forceinline explicit Scalar(EZeroTag) noexcept { m_vec = SplatZero(); }
	__forceinline explicit Scalar(EIdentityTag) noexcept { m_vec = SplatOne(); }

	__forceinline operator XMVECTOR() const noexcept { return m_vec; }
	__forceinline operator float() const noexcept { return XMVectorGetX(m_vec); }

private:
	XMVECTOR m_vec;
};


__forceinline Scalar operator-(Scalar s) noexcept { return Scalar(XMVectorNegate(s)); }
__forceinline Scalar operator+(Scalar s1, Scalar s2) noexcept { return Scalar(XMVectorAdd(s1, s2)); }
__forceinline Scalar operator-(Scalar s1, Scalar s2) noexcept { return Scalar(XMVectorSubtract(s1, s2)); }
__forceinline Scalar operator*(Scalar s1, Scalar s2) noexcept { return Scalar(XMVectorMultiply(s1, s2)); }
__forceinline Scalar operator/(Scalar s1, Scalar s2) noexcept { return Scalar(XMVectorDivide(s1, s2)); }
__forceinline Scalar operator+(Scalar s1, float s2) noexcept { return s1 + Scalar(s2); }
__forceinline Scalar operator-(Scalar s1, float s2) noexcept { return s1 - Scalar(s2); }
__forceinline Scalar operator*(Scalar s1, float s2) noexcept { return s1 * Scalar(s2); }
__forceinline Scalar operator/(Scalar s1, float s2) noexcept { return s1 / Scalar(s2); }
__forceinline Scalar operator+(float s1, Scalar s2) noexcept { return Scalar(s1) + s2; }
__forceinline Scalar operator-(float s1, Scalar s2) noexcept { return Scalar(s1) - s2; }
__forceinline Scalar operator*(float s1, Scalar s2) noexcept { return Scalar(s1) * s2; }
__forceinline Scalar operator/(float s1, Scalar s2) noexcept { return Scalar(s1) / s2; }

} // namespace Math