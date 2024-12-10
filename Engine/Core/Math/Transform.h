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

#include "Matrix3.h"

namespace Math
{

// This transform strictly prohibits non-uniform scale.  Scale itself is barely tolerated.
__declspec(align(16)) class OrthogonalTransform
{
public:
	__forceinline OrthogonalTransform() noexcept : m_rotation(kIdentity), m_translation(kZero) {}
	__forceinline OrthogonalTransform(Quaternion rotate) noexcept : m_rotation(rotate), m_translation(kZero) {}
	__forceinline OrthogonalTransform(Vector3 translate) noexcept : m_rotation(kIdentity), m_translation(translate) {}
	__forceinline OrthogonalTransform(Quaternion rotate, Vector3 translate) noexcept : m_rotation(rotate), m_translation(translate) {}
	__forceinline OrthogonalTransform(const Matrix3& mat) noexcept : m_rotation(mat), m_translation(kZero) {}
	__forceinline OrthogonalTransform(const Matrix3& mat, Vector3 translate) noexcept : m_rotation(mat), m_translation(translate) {}
	__forceinline OrthogonalTransform(EIdentityTag) noexcept : m_rotation(kIdentity), m_translation(kZero) {}
	__forceinline explicit OrthogonalTransform(const XMMATRIX& mat) noexcept { *this = OrthogonalTransform(Matrix3(mat), Vector3(mat.r[3])); }

	__forceinline void SetRotation(Quaternion q) noexcept { m_rotation = q; }
	__forceinline void SetTranslation(Vector3 v) noexcept { m_translation = v; }

	__forceinline Quaternion GetRotation() const noexcept { return m_rotation; }
	__forceinline Vector3 GetTranslation() const noexcept { return m_translation; }

	static __forceinline OrthogonalTransform MakeXRotation(float angle) noexcept { return OrthogonalTransform(Quaternion(Vector3(kXUnitVector), angle)); }
	static __forceinline OrthogonalTransform MakeYRotation(float angle) noexcept { return OrthogonalTransform(Quaternion(Vector3(kYUnitVector), angle)); }
	static __forceinline OrthogonalTransform MakeZRotation(float angle) noexcept { return OrthogonalTransform(Quaternion(Vector3(kZUnitVector), angle)); }
	static __forceinline OrthogonalTransform MakeTranslation(Vector3 translate) noexcept { return OrthogonalTransform(translate); }

	__forceinline Vector3 operator*(Vector3 vec) const noexcept { return m_rotation * vec + m_translation; }
	__forceinline Vector4 operator*(Vector4 vec) const noexcept {
		return
			Vector4(SetWToZero(m_rotation * Vector3((XMVECTOR)vec))) +
			Vector4(SetWToOne(m_translation)) * vec.GetW();
	}
	__forceinline OrthogonalTransform operator* (const OrthogonalTransform& xform) const noexcept {
		return OrthogonalTransform(m_rotation * xform.m_rotation, m_rotation * xform.m_translation + m_translation);
	}

	__forceinline OrthogonalTransform operator~ () const noexcept {
		Quaternion invertedRotation = ~m_rotation;
		return OrthogonalTransform(invertedRotation, invertedRotation * -m_translation);
	}

private:
	Quaternion m_rotation;
	Vector3 m_translation;
};


// An AffineTransform is a 3x4 matrix with an implicit 4th row = [0,0,0,1].  This is used to perform a change of
// basis on 3D points.  An affine transformation does not have to have orthonormal basis vectors.
__declspec(align(64)) class AffineTransform
{
public:
	__forceinline AffineTransform() = default;
	__forceinline AffineTransform(Vector3 x, Vector3 y, Vector3 z, Vector3 w) noexcept
		: m_basis(x, y, z), m_translation(w) {}
	__forceinline AffineTransform(Vector3 translate) noexcept
		: m_basis(kIdentity), m_translation(translate) {}
	__forceinline AffineTransform(const Matrix3& mat, Vector3 translate = Vector3(kZero)) noexcept
		: m_basis(mat), m_translation(translate) {}
	__forceinline AffineTransform(Quaternion rot, Vector3 translate = Vector3(kZero)) noexcept
		: m_basis(rot), m_translation(translate) {}
	__forceinline AffineTransform(const OrthogonalTransform& xform) noexcept
		: m_basis(xform.GetRotation()), m_translation(xform.GetTranslation()) {}
	__forceinline AffineTransform(EIdentityTag) noexcept
		: m_basis(kIdentity), m_translation(kZero) {}
	__forceinline explicit AffineTransform(const XMMATRIX& mat) noexcept
		: m_basis(mat), m_translation(mat.r[3]) {}

	__forceinline operator XMMATRIX() const noexcept { return (XMMATRIX&)*this; }

	__forceinline void SetX(Vector3 x) noexcept { m_basis.SetX(x); }
	__forceinline void SetY(Vector3 y) noexcept { m_basis.SetY(y); }
	__forceinline void SetZ(Vector3 z) noexcept { m_basis.SetZ(z); }
	__forceinline void SetTranslation(Vector3 w) noexcept { m_translation = w; }

	__forceinline Vector3 GetX() const noexcept { return m_basis.GetX(); }
	__forceinline Vector3 GetY() const noexcept { return m_basis.GetY(); }
	__forceinline Vector3 GetZ() const noexcept { return m_basis.GetZ(); }
	__forceinline Vector3 GetTranslation() const noexcept { return m_translation; }
	__forceinline const Matrix3& GetBasis() const noexcept { return (const Matrix3&)*this; }

	static __forceinline AffineTransform MakeXRotation(float angle) noexcept { return AffineTransform(Matrix3::MakeXRotation(angle)); }
	static __forceinline AffineTransform MakeYRotation(float angle) noexcept { return AffineTransform(Matrix3::MakeYRotation(angle)); }
	static __forceinline AffineTransform MakeZRotation(float angle) noexcept { return AffineTransform(Matrix3::MakeZRotation(angle)); }
	static __forceinline AffineTransform MakeScale(float scale) noexcept { return AffineTransform(Matrix3::MakeScale(scale)); }
	static __forceinline AffineTransform MakeScale(Vector3 scale) noexcept { return AffineTransform(Matrix3::MakeScale(scale)); }
	static __forceinline AffineTransform MakeTranslation(Vector3 translate) noexcept { return AffineTransform(translate); }

	__forceinline Vector3 operator*(Vector3 vec) const noexcept { return m_basis * vec + m_translation; }
	__forceinline AffineTransform operator*(const AffineTransform& mat) const noexcept
	{
		return AffineTransform(m_basis * mat.m_basis, *this * mat.GetTranslation());
	}

private:
	Matrix3 m_basis;
	Vector3 m_translation;
};

} // namespace Math