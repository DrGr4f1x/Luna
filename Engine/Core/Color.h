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

#include <DirectXColors.h>
#include <DirectXMath.h>
#include <intrin.h>

using namespace DirectX;

namespace Luna
{

class Color
{
public:
	Color() noexcept : m_value{ g_XMOne } {}
	Color(FXMVECTOR vec) noexcept;
	Color(const XMVECTORF32& vec) noexcept;
	Color(float r, float g, float b, float a = 1.0f) noexcept;
	Color(uint16_t r, uint16_t g, uint16_t b, uint16_t a = 255, uint16_t bitDepth = 8) noexcept;
	explicit Color(uint32_t rgbaLittleEndian) noexcept;

	float R() const noexcept { return XMVectorGetX(m_value); }
	float G() const noexcept { return XMVectorGetY(m_value); }
	float B() const noexcept { return XMVectorGetZ(m_value); }
	float A() const noexcept { return XMVectorGetW(m_value); }

	bool operator==(const Color& rhs) const noexcept { return XMVector4Equal(m_value, rhs.m_value); }
	bool operator!=(const Color& rhs) const noexcept { return !XMVector4Equal(m_value, rhs.m_value); }

	void SetR(float r) noexcept { m_value.f[0] = r; }
	void SetG(float g) noexcept { m_value.f[1] = g; }
	void SetB(float b) noexcept { m_value.f[2] = b; }
	void SetA(float a) noexcept { m_value.f[3] = a; }

	float* GetPtr(void) noexcept { return reinterpret_cast<float*>(this); }
	float& operator[](int idx) { return GetPtr()[idx]; }

	void SetRGB(float r, float g, float b) noexcept { m_value.v = XMVectorSelect(m_value, XMVectorSet(r, g, b, b), g_XMMask3); }

	Color ToSRGB() const noexcept;
	Color FromSRGB() const noexcept;
	Color ToREC709() const noexcept;
	Color FromREC709() const noexcept;

	// Probably want to convert to sRGB or Rec709 first
	uint32_t R10G10B10A2() const noexcept;
	uint32_t R8G8B8A8() const noexcept;

	// Pack an HDR color into 32-bits
	uint32_t R11G11B10F(bool RoundToEven = false) const noexcept;
	uint32_t R9G9B9E5() const noexcept;

	operator XMVECTOR() const noexcept { return m_value; }

private:
	XMVECTORF32 m_value{};
};

__forceinline Color Max(Color a, Color b) noexcept { return Color(XMVectorMax(a, b)); }
__forceinline Color Min(Color a, Color b) noexcept { return Color(XMVectorMin(a, b)); }
__forceinline Color Clamp(Color x, Color a, Color b) noexcept { return Color(XMVectorClamp(x, a, b)); }


inline Color::Color(FXMVECTOR vec) noexcept
{
	m_value.v = vec;
}

inline Color::Color(const XMVECTORF32& vec) noexcept
{
	m_value = vec;
}

inline Color::Color(float r, float g, float b, float a) noexcept
{
	m_value.v = XMVectorSet(r, g, b, a);
}

inline Color::Color(uint16_t r, uint16_t g, uint16_t b, uint16_t a, uint16_t bitDepth) noexcept
{
	m_value.v = XMVectorScale(XMVectorSet(r, g, b, a), 1.0f / ((1 << bitDepth) - 1));
}

inline Color::Color(uint32_t u32) noexcept
{
	float r = (float)((u32 >> 0) & 0xFF);
	float g = (float)((u32 >> 8) & 0xFF);
	float b = (float)((u32 >> 16) & 0xFF);
	float a = (float)((u32 >> 24) & 0xFF);
	m_value.v = XMVectorScale(XMVectorSet(r, g, b, a), 1.0f / 255.0f);
}

inline Color Color::ToSRGB() const noexcept
{
	XMVECTOR T = XMVectorSaturate(m_value);
	XMVECTOR result = XMVectorSubtract(XMVectorScale(XMVectorPow(T, XMVectorReplicate(1.0f / 2.4f)), 1.055f), XMVectorReplicate(0.055f));
	result = XMVectorSelect(result, XMVectorScale(T, 12.92f), XMVectorLess(T, XMVectorReplicate(0.0031308f)));
	return XMVectorSelect(T, result, g_XMSelect1110);
}

inline Color Color::FromSRGB() const noexcept
{
	XMVECTOR T = XMVectorSaturate(m_value);
	XMVECTOR result = XMVectorPow(XMVectorScale(XMVectorAdd(T, XMVectorReplicate(0.055f)), 1.0f / 1.055f), XMVectorReplicate(2.4f));
	result = XMVectorSelect(result, XMVectorScale(T, 1.0f / 12.92f), XMVectorLess(T, XMVectorReplicate(0.0031308f)));
	return XMVectorSelect(T, result, g_XMSelect1110);
}

inline Color Color::ToREC709() const noexcept
{
	XMVECTOR T = XMVectorSaturate(m_value);
	XMVECTOR result = XMVectorSubtract(XMVectorScale(XMVectorPow(T, XMVectorReplicate(0.45f)), 1.099f), XMVectorReplicate(0.099f));
	result = XMVectorSelect(result, XMVectorScale(T, 4.5f), XMVectorLess(T, XMVectorReplicate(0.0018f)));
	return XMVectorSelect(T, result, g_XMSelect1110);
}

inline Color Color::FromREC709() const noexcept
{
	XMVECTOR T = XMVectorSaturate(m_value);
	XMVECTOR result = XMVectorPow(XMVectorScale(XMVectorAdd(T, XMVectorReplicate(0.099f)), 1.0f / 1.099f), XMVectorReplicate(1.0f / 0.45f));
	result = XMVectorSelect(result, XMVectorScale(T, 1.0f / 4.5f), XMVectorLess(T, XMVectorReplicate(0.0081f)));
	return XMVectorSelect(T, result, g_XMSelect1110);
}

inline uint32_t Color::R10G10B10A2() const noexcept
{
	XMVECTOR result = XMVectorRound(XMVectorMultiply(XMVectorSaturate(m_value), XMVectorSet(1023.0f, 1023.0f, 1023.0f, 3.0f)));
	result = _mm_castsi128_ps(_mm_cvttps_epi32(result));
	uint32_t r = XMVectorGetIntX(result);
	uint32_t g = XMVectorGetIntY(result);
	uint32_t b = XMVectorGetIntZ(result);
	uint32_t a = XMVectorGetIntW(result) >> 8;
	return a << 30 | b << 20 | g << 10 | r;
}

inline uint32_t Color::R8G8B8A8() const noexcept
{
	XMVECTOR result = XMVectorRound(XMVectorMultiply(XMVectorSaturate(m_value), XMVectorReplicate(255.0f)));
	result = _mm_castsi128_ps(_mm_cvttps_epi32(result));
	uint32_t r = XMVectorGetIntX(result);
	uint32_t g = XMVectorGetIntY(result);
	uint32_t b = XMVectorGetIntZ(result);
	uint32_t a = XMVectorGetIntW(result);
	return a << 24 | b << 16 | g << 8 | r;
}

} // namespace Luna