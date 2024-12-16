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
	D3D12_FEATURE_DATA_D3D12_OPTIONS8 caps8{};
	D3D12_FEATURE_DATA_D3D12_OPTIONS9 caps9{};
	D3D12_FEATURE_DATA_D3D12_OPTIONS10 caps10{};
	D3D12_FEATURE_DATA_D3D12_OPTIONS11 caps11{};
	D3D12_FEATURE_DATA_D3D12_OPTIONS12 caps12{};
	D3D12_FEATURE_DATA_D3D12_OPTIONS13 caps13{};

	void ReadBasicCaps(ID3D12Device* device, D3D_FEATURE_LEVEL minFeatureLevel);
	void ReadFullCaps(ID3D12Device* device, D3D_FEATURE_LEVEL minFeatureLevel, D3D_SHADER_MODEL bestShaderModel);

	bool HasCaps(int32_t capsNum) const;

	void LogCaps();

private:
	D3D_FEATURE_LEVEL GetHighestFeatureLevel(ID3D12Device* device, D3D_FEATURE_LEVEL minFeatureLevel);
	D3D_SHADER_MODEL GetHighestShaderModel(ID3D12Device* device);

private:
	std::bitset<14> m_validCaps;
	bool m_basicCapsRead{ false };
	bool m_capsRead{ false };
};

} // namespace Luna::DX12