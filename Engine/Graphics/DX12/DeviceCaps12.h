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

#include "Graphics\DX12\DirectXCommon.h"

#include <bitset>


namespace Luna::DX12
{

struct DeviceBasicCaps
{
	D3D_FEATURE_LEVEL           maxFeatureLevel{};
	D3D_SHADER_MODEL            maxShaderModel{};
	D3D12_RESOURCE_BINDING_TIER resourceBindingTier{};
	D3D12_RESOURCE_HEAP_TIER    resourceHeapTier{};
	uint32_t                    numDeviceNodes{ 0 };
	bool                        bSupportsWaveOps{ false };
	bool                        bSupportsAtomic64{ false };
};


struct DeviceCaps
{
	DeviceBasicCaps basicCaps;

	D3D12_FEATURE_DATA_D3D12_OPTIONS caps{};
	D3D12_FEATURE_DATA_D3D12_OPTIONS1 caps1{};
	D3D12_FEATURE_DATA_D3D12_OPTIONS2 caps2{};
	D3D12_FEATURE_DATA_D3D12_OPTIONS3 caps3{};
	D3D12_FEATURE_DATA_D3D12_OPTIONS4 caps4{};
	D3D12_FEATURE_DATA_D3D12_OPTIONS5 caps5{};
	D3D12_FEATURE_DATA_D3D12_OPTIONS6 caps6{};
	D3D12_FEATURE_DATA_D3D12_OPTIONS7 caps7{};

#if defined(D3D12_SDK_VERSION) && (D3D12_SDK_VERSION >= 3)
	D3D12_FEATURE_DATA_D3D12_OPTIONS8 caps8{};
	D3D12_FEATURE_DATA_D3D12_OPTIONS9 caps9{};
#endif

#if defined(D3D12_SDK_VERSION) && (D3D12_SDK_VERSION >= 4)
	D3D12_FEATURE_DATA_D3D12_OPTIONS10 caps10{};
	D3D12_FEATURE_DATA_D3D12_OPTIONS11 caps11{};
#endif

#if defined(D3D12_SDK_VERSION) && (D3D12_SDK_VERSION >= 600)
	D3D12_FEATURE_DATA_D3D12_OPTIONS12 caps12{};
#endif

#if defined(D3D12_SDK_VERSION) && (D3D12_SDK_VERSION >= 602)
	D3D12_FEATURE_DATA_D3D12_OPTIONS13 caps13{};
#endif

#if defined(D3D12_SDK_VERSION) && (D3D12_SDK_VERSION >= 606)
	D3D12_FEATURE_DATA_D3D12_OPTIONS14 caps14{};
	D3D12_FEATURE_DATA_D3D12_OPTIONS15 caps15{};
#endif

#if defined(D3D12_SDK_VERSION) && (D3D12_SDK_VERSION >= 608)
	D3D12_FEATURE_DATA_D3D12_OPTIONS16 caps16{};
#endif

#if defined(D3D12_SDK_VERSION) && (D3D12_SDK_VERSION >= 609)
	D3D12_FEATURE_DATA_D3D12_OPTIONS17 caps17{};
	D3D12_FEATURE_DATA_D3D12_OPTIONS18 caps18{};
#endif

#if defined(D3D12_SDK_VERSION) && (D3D12_SDK_VERSION >= 610)
	D3D12_FEATURE_DATA_D3D12_OPTIONS19 caps19{};
#endif

#if defined(D3D12_SDK_VERSION) && (D3D12_SDK_VERSION >= 611)
	D3D12_FEATURE_DATA_D3D12_OPTIONS20 caps20{};
#endif

#if defined(D3D12_SDK_VERSION) && (D3D12_SDK_VERSION >= 612)
	D3D12_FEATURE_DATA_D3D12_OPTIONS21 caps21{};
#endif

#if defined(D3D12_SDK_VERSION) && (D3D12_SDK_VERSION >= 614)
	D3D12_FEATURE_DATA_D3D12_OPTIONS22 caps22{};
#endif

	void ReadBasicCaps(ID3D12Device* device, D3D_FEATURE_LEVEL minFeatureLevel);
	void ReadFullCaps(ID3D12Device* device, D3D_FEATURE_LEVEL minFeatureLevel, D3D_SHADER_MODEL bestShaderModel);

	bool HasCaps(int32_t capsNum) const;

	void LogCaps();

private:
	D3D_FEATURE_LEVEL GetHighestFeatureLevel(ID3D12Device* device, D3D_FEATURE_LEVEL minFeatureLevel);
	D3D_SHADER_MODEL GetHighestShaderModel(ID3D12Device* device);

private:
	std::bitset<23> m_validCaps;
	bool m_basicCapsRead{ false };
	bool m_capsRead{ false };
};

} // namespace Luna::DX12