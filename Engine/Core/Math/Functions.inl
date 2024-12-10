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

namespace Math
{

// To allow floats to implicitly construct Scalars, we need to clarify these operators and suppress
// upconversion.
__forceinline bool operator<(Scalar lhs, float rhs) noexcept { return (float)lhs < rhs; }
__forceinline bool operator<=(Scalar lhs, float rhs) noexcept { return (float)lhs <= rhs; }
__forceinline bool operator>(Scalar lhs, float rhs) noexcept { return (float)lhs > rhs; }
__forceinline bool operator>=(Scalar lhs, float rhs) noexcept { return (float)lhs >= rhs; }
__forceinline bool operator==(Scalar lhs, float rhs) noexcept { return (float)lhs == rhs; }
__forceinline bool operator<(float lhs, Scalar rhs) noexcept { return lhs < (float)rhs; }
__forceinline bool operator<=(float lhs, Scalar rhs) noexcept { return lhs <= (float)rhs; }
__forceinline bool operator>(float lhs, Scalar rhs) noexcept { return lhs > (float)rhs; }
__forceinline bool operator>=(float lhs, Scalar rhs) noexcept { return lhs >= (float)rhs; }
__forceinline bool operator==(float lhs, Scalar rhs) noexcept { return lhs == (float)rhs; }


#define CREATE_SIMD_FUNCTIONS( TYPE ) \
	__forceinline TYPE Sqrt( TYPE s ) noexcept { return TYPE(XMVectorSqrt(s)); } \
	__forceinline TYPE Recip( TYPE s ) noexcept { return TYPE(XMVectorReciprocal(s)); } \
	__forceinline TYPE RecipSqrt( TYPE s ) noexcept { return TYPE(XMVectorReciprocalSqrt(s)); } \
	__forceinline TYPE Floor( TYPE s ) noexcept { return TYPE(XMVectorFloor(s)); } \
	__forceinline TYPE Ceiling( TYPE s ) noexcept { return TYPE(XMVectorCeiling(s)); } \
	__forceinline TYPE Round( TYPE s ) noexcept { return TYPE(XMVectorRound(s)); } \
	__forceinline TYPE Abs( TYPE s ) noexcept { return TYPE(XMVectorAbs(s)); } \
	__forceinline TYPE Exp( TYPE s ) noexcept { return TYPE(XMVectorExp(s)); } \
	__forceinline TYPE Pow( TYPE b, TYPE e ) noexcept { return TYPE(XMVectorPow(b, e)); } \
	__forceinline TYPE Log( TYPE s ) noexcept { return TYPE(XMVectorLog(s)); } \
	__forceinline TYPE Sin( TYPE s ) noexcept { return TYPE(XMVectorSin(s)); } \
	__forceinline TYPE Cos( TYPE s ) noexcept { return TYPE(XMVectorCos(s)); } \
	__forceinline TYPE Tan( TYPE s ) noexcept { return TYPE(XMVectorTan(s)); } \
	__forceinline TYPE ASin( TYPE s ) noexcept { return TYPE(XMVectorASin(s)); } \
	__forceinline TYPE ACos( TYPE s ) noexcept { return TYPE(XMVectorACos(s)); } \
	__forceinline TYPE ATan( TYPE s ) noexcept { return TYPE(XMVectorATan(s)); } \
	__forceinline TYPE ATan2( TYPE y, TYPE x ) noexcept { return TYPE(XMVectorATan2(y, x)); } \
	__forceinline TYPE Lerp( TYPE a, TYPE b, TYPE t ) noexcept { return TYPE(XMVectorLerpV(a, b, t)); } \
	__forceinline TYPE Max( TYPE a, TYPE b ) noexcept { return TYPE(XMVectorMax(a, b)); } \
	__forceinline TYPE Min( TYPE a, TYPE b ) noexcept { return TYPE(XMVectorMin(a, b)); } \
	__forceinline TYPE Clamp( TYPE v, TYPE a, TYPE b ) noexcept { return Min(Max(v, a), b); } \
	__forceinline TYPE MultiplyAdd( TYPE v, TYPE a, TYPE b ) noexcept { return TYPE(XMVectorMultiplyAdd(v, a, b)); } \
	__forceinline BoolVector operator<  ( TYPE lhs, TYPE rhs ) noexcept { return XMVectorLess(lhs, rhs); } \
	__forceinline BoolVector operator<= ( TYPE lhs, TYPE rhs ) noexcept { return XMVectorLessOrEqual(lhs, rhs); } \
	__forceinline BoolVector operator>  ( TYPE lhs, TYPE rhs ) noexcept { return XMVectorGreater(lhs, rhs); } \
	__forceinline BoolVector operator>= ( TYPE lhs, TYPE rhs ) noexcept { return XMVectorGreaterOrEqual(lhs, rhs); } \
	__forceinline BoolVector operator== ( TYPE lhs, TYPE rhs ) noexcept { return XMVectorEqual(lhs, rhs); } \
	__forceinline TYPE Select( TYPE lhs, TYPE rhs, BoolVector mask ) noexcept { return TYPE(XMVectorSelect(lhs, rhs, mask)); }


CREATE_SIMD_FUNCTIONS(Scalar)
CREATE_SIMD_FUNCTIONS(Vector3)
CREATE_SIMD_FUNCTIONS(Vector4)

#undef CREATE_SIMD_FUNCTIONS


__forceinline float Sqrt(float s) noexcept { return Sqrt(Scalar(s)); }
__forceinline float Recip(float s) noexcept { return Recip(Scalar(s)); }
__forceinline float RecipSqrt(float s) noexcept { return RecipSqrt(Scalar(s)); }
__forceinline float Floor(float s) noexcept { return Floor(Scalar(s)); }
__forceinline float Ceiling(float s) noexcept { return Ceiling(Scalar(s)); }
__forceinline float Round(float s) noexcept { return Round(Scalar(s)); }
__forceinline float Abs(float s) noexcept { return s < 0.0f ? -s : s; }
__forceinline float Exp(float s) noexcept { return Exp(Scalar(s)); }
__forceinline float Pow(float b, float e) noexcept { return Pow(Scalar(b), Scalar(e)); }
__forceinline float Log(float s) noexcept { return Log(Scalar(s)); }
__forceinline float Sin(float s) noexcept { return Sin(Scalar(s)); }
__forceinline float Cos(float s) noexcept { return Cos(Scalar(s)); }
__forceinline float Tan(float s) noexcept { return Tan(Scalar(s)); }
__forceinline float ASin(float s) noexcept { return ASin(Scalar(s)); }
__forceinline float ACos(float s) noexcept { return ACos(Scalar(s)); }
__forceinline float ATan(float s) noexcept { return ATan(Scalar(s)); }
__forceinline float ATan2(float y, float x) noexcept { return ATan2(Scalar(y), Scalar(x)); }
__forceinline float Lerp(float a, float b, float t) noexcept { return a + (b - a) * t; }
__forceinline float Max(float a, float b) noexcept { return a > b ? a : b; }
__forceinline float Min(float a, float b) noexcept { return a < b ? a : b; }
__forceinline float Clamp(float v, float a, float b) noexcept { return Min(Max(v, a), b); }


__forceinline Scalar Length(Vector3 v) noexcept { return Scalar(XMVector3Length(v)); }
__forceinline Scalar LengthSquare(Vector3 v) noexcept { return Scalar(XMVector3LengthSq(v)); }
__forceinline Scalar LengthRecip(Vector3 v) noexcept { return Scalar(XMVector3ReciprocalLength(v)); }
__forceinline Scalar Dot(Vector3 v1, Vector3 v2) noexcept { return Scalar(XMVector3Dot(v1, v2)); }
__forceinline Scalar Dot(Vector4 v1, Vector4 v2) noexcept { return Scalar(XMVector4Dot(v1, v2)); }
__forceinline Vector3 Cross(Vector3 v1, Vector3 v2) noexcept { return Vector3(XMVector3Cross(v1, v2)); }
__forceinline Vector3 Normalize(Vector3 v) noexcept { return Vector3(XMVector3Normalize(v)); }
__forceinline Vector4 Normalize(Vector4 v) noexcept { return Vector4(XMVector4Normalize(v)); }
__forceinline Quaternion Normalize(Quaternion q) noexcept { return Quaternion(XMQuaternionNormalize(q)); }

__forceinline Matrix3 Transpose(const Matrix3& mat) noexcept { return Matrix3(XMMatrixTranspose(mat)); }


// inline Matrix3 Inverse( const Matrix3& mat ) { TBD }
// inline Transform Inverse( const Transform& mat ) { TBD }

// This specialized matrix invert assumes that the 3x3 matrix is orthogonal (and normalized).
__forceinline AffineTransform OrthoInvert(const AffineTransform& xform) noexcept
{
	Matrix3 basis = Transpose(xform.GetBasis());
	return AffineTransform(basis, basis * -xform.GetTranslation());
}

__forceinline OrthogonalTransform Invert(const OrthogonalTransform& xform) noexcept { return ~xform; }

__forceinline Matrix4 Transpose(const Matrix4& mat) noexcept { return Matrix4(XMMatrixTranspose(mat)); }
__forceinline Matrix4 Invert(const Matrix4& mat) noexcept { return Matrix4(XMMatrixInverse(nullptr, mat)); }

__forceinline Matrix4 OrthoInvert(const Matrix4& xform) noexcept
{
	Matrix3 basis = Transpose(xform.Get3x3());
	Vector3 translate = basis * -Vector3(xform.GetW());
	return Matrix4(basis, translate);
}

} // namespace Math