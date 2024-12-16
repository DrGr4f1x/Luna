//
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
// Author:  David Elder
//

#include "Stdafx.h"

#include "DeviceCaps12.h"

using namespace std;


namespace Luna::DX12
{

void DeviceCaps::ReadBasicCaps(ID3D12Device* device, D3D_FEATURE_LEVEL minFeatureLevel)
{
	if (!m_basicCapsRead)
	{
		basicCaps.maxFeatureLevel = GetHighestFeatureLevel(device, minFeatureLevel);
		basicCaps.maxShaderModel = GetHighestShaderModel(device);

		D3D12_FEATURE_DATA_D3D12_OPTIONS dx12Caps{};
		device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &dx12Caps, sizeof(dx12Caps));
		basicCaps.resourceBindingTier = dx12Caps.ResourceBindingTier;
		basicCaps.resourceHeapTier = dx12Caps.ResourceHeapTier;

		basicCaps.numDeviceNodes = device->GetNodeCount();

		D3D12_FEATURE_DATA_D3D12_OPTIONS1 dx12Caps1{};
		device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS1, &dx12Caps1, sizeof(dx12Caps1));
		basicCaps.bSupportsWaveOps = dx12Caps1.WaveOps == TRUE;

		D3D12_FEATURE_DATA_D3D12_OPTIONS9 dx12Caps9{};
		device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS9, &dx12Caps9, sizeof(dx12Caps9));
		basicCaps.bSupportsAtomic64 = dx12Caps9.AtomicInt64OnTypedResourceSupported == TRUE;

		m_basicCapsRead = true;
	}
}


void DeviceCaps::ReadFullCaps(ID3D12Device* device, D3D_FEATURE_LEVEL minFeatureLevel, D3D_SHADER_MODEL bestShaderModel)
{
	ReadBasicCaps(device, minFeatureLevel);

	if (!m_capsRead)
	{
		m_validCaps[0] = SUCCEEDED(device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &caps, sizeof(caps)));
		m_validCaps[1] = SUCCEEDED(device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS1, &caps1, sizeof(caps1)));
		m_validCaps[2] = SUCCEEDED(device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS2, &caps2, sizeof(caps2)));
		m_validCaps[3] = SUCCEEDED(device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS3, &caps3, sizeof(caps3)));
		m_validCaps[4] = SUCCEEDED(device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS4, &caps4, sizeof(caps4)));
		m_validCaps[5] = SUCCEEDED(device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &caps5, sizeof(caps5)));
		m_validCaps[6] = SUCCEEDED(device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS6, &caps6, sizeof(caps6)));
		m_validCaps[7] = SUCCEEDED(device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS7, &caps7, sizeof(caps7)));
		m_validCaps[8] = SUCCEEDED(device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS8, &caps8, sizeof(caps8)));
		m_validCaps[9] = SUCCEEDED(device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS9, &caps9, sizeof(caps9)));
		m_validCaps[10] = SUCCEEDED(device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS10, &caps10, sizeof(caps10)));
		m_validCaps[11] = SUCCEEDED(device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS11, &caps11, sizeof(caps11)));
		m_validCaps[12] = SUCCEEDED(device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS12, &caps12, sizeof(caps12)));
		m_validCaps[13] = SUCCEEDED(device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS13, &caps13, sizeof(caps13)));

		m_capsRead = true;
	}
}


bool DeviceCaps::HasCaps(int32_t capsNum) const
{
	assert(capsNum >= 0 && capsNum < m_validCaps.size());
	return m_validCaps[capsNum];
}


void DeviceCaps::LogCaps()
{
	LogInfo(LogDirectX) << "  Device Caps" << endl;

	LogInfo(LogDirectX) << format("    {:56} {}", "Highest supported feature level:", basicCaps.maxFeatureLevel) << endl;
	LogInfo(LogDirectX) << format("    {:56} {}", "Highest supported shader model:", basicCaps.maxShaderModel) << endl;
	LogInfo(LogDirectX) << endl;

	constexpr const char* formatStr = "      {:54} {}";

	if (HasCaps(0))
	{
		LogInfo(LogDirectX) << "    D3D12_FEATURE_D3D12_OPTIONS" << endl;
		LogInfo(LogDirectX) << format(formatStr, "DoublePrecisionFloatShaderOps:", (bool)caps.DoublePrecisionFloatShaderOps) << endl;
		LogInfo(LogDirectX) << format(formatStr, "OutputMergerLogicOp:", (bool)caps.OutputMergerLogicOp) << endl;
		LogInfo(LogDirectX) << format(formatStr, "MinPrecisionSupport:", caps.MinPrecisionSupport) << endl;
		LogInfo(LogDirectX) << format(formatStr, "TiledResourcesTier:", caps.TiledResourcesTier) << endl;
		LogInfo(LogDirectX) << format(formatStr, "PSSpecifiedStencilRefSupported:", (bool)caps.PSSpecifiedStencilRefSupported) << endl;
		LogInfo(LogDirectX) << format(formatStr, "TypedUAVLoadAdditionalFormats:", (bool)caps.TypedUAVLoadAdditionalFormats) << endl;
		LogInfo(LogDirectX) << format(formatStr, "ROVsSupported:", (bool)caps.ROVsSupported) << endl;
		LogInfo(LogDirectX) << format(formatStr, "ConservativeRasterizationTier:", caps.ConservativeRasterizationTier) << endl;
		LogInfo(LogDirectX) << format(formatStr, "MaxGPUVirtualAddressBitsPerResource:", caps.MaxGPUVirtualAddressBitsPerResource) << endl;
		LogInfo(LogDirectX) << format(formatStr, "StandardSwizzle64KBSupported:", (bool)caps.StandardSwizzle64KBSupported) << endl;
		LogInfo(LogDirectX) << format(formatStr, "CrossNodeSharingTier:", caps.CrossNodeSharingTier) << endl;
		LogInfo(LogDirectX) << format(formatStr, "CrossAdapterRowMajorTextureSupported:", (bool)caps.CrossAdapterRowMajorTextureSupported) << endl;
		LogInfo(LogDirectX) << format(formatStr, "VPAndRTArrayIndexFromAnyShaderFeedingRasterizerSupportedWithoutGSEmulation:", (bool)caps.VPAndRTArrayIndexFromAnyShaderFeedingRasterizerSupportedWithoutGSEmulation) << endl;
		LogInfo(LogDirectX) << format(formatStr, "ResourceHeapTier:", caps.ResourceHeapTier) << endl;
		LogInfo(LogDirectX) << endl;
	}

	if (HasCaps(1))
	{
		LogInfo(LogDirectX) << "    D3D12_FEATURE_D3D12_OPTIONS1" << endl;
		LogInfo(LogDirectX) << format(formatStr, "WaveOps:", (bool)caps1.WaveOps) << endl;
		LogInfo(LogDirectX) << format(formatStr, "WaveLaneCountMin:", caps1.WaveLaneCountMin) << endl;
		LogInfo(LogDirectX) << format(formatStr, "WaveLaneCountMax:", caps1.WaveLaneCountMax) << endl;
		LogInfo(LogDirectX) << format(formatStr, "TotalLaneCount:", caps1.TotalLaneCount) << endl;
		LogInfo(LogDirectX) << format(formatStr, "ExpandedComputeResourceState:", (bool)caps1.ExpandedComputeResourceStates) << endl;
		LogInfo(LogDirectX) << format(formatStr, "Int64ShaderOps:", caps1.Int64ShaderOps) << endl;
		LogInfo(LogDirectX) << endl;
	}

	if (HasCaps(2))
	{
		LogInfo(LogDirectX) << "    D3D12_FEATURE_D3D12_OPTIONS2" << endl;
		LogInfo(LogDirectX) << format(formatStr, "DepthBoundsTestSupported:", (bool)caps2.DepthBoundsTestSupported) << endl;
		LogInfo(LogDirectX) << format(formatStr, "ProgrammableSamplePositionsTier:", caps2.ProgrammableSamplePositionsTier) << endl;
		LogInfo(LogDirectX) << endl;
	}

	if (HasCaps(3))
	{
		LogInfo(LogDirectX) << "    D3D12_FEATURE_D3D12_OPTIONS3" << endl;
		LogInfo(LogDirectX) << format(formatStr, "CopyQueueTimestampQueriesSupported:", (bool)caps3.CopyQueueTimestampQueriesSupported) << endl;
		LogInfo(LogDirectX) << format(formatStr, "CastingFullyTypedFormatSupported:", (bool)caps3.CastingFullyTypedFormatSupported) << endl;
		LogInfo(LogDirectX) << format(formatStr, "WriteBufferImmediateSupportFlags:", caps3.WriteBufferImmediateSupportFlags) << endl;
		LogInfo(LogDirectX) << format(formatStr, "ViewInstancingTier:", caps3.ViewInstancingTier) << endl;
		LogInfo(LogDirectX) << format(formatStr, "BarycentricsSupported:", (bool)caps3.BarycentricsSupported) << endl;
		LogInfo(LogDirectX) << endl;
	}

	if (HasCaps(4))
	{
		LogInfo(LogDirectX) << "    D3D12_FEATURE_D3D12_OPTIONS4" << endl;
		LogInfo(LogDirectX) << format(formatStr, "MSAA64KBAlignedTextureSupported:", (bool)caps4.MSAA64KBAlignedTextureSupported) << endl;
		LogInfo(LogDirectX) << format(formatStr, "SharedResourceCompatibilityTier:", caps4.SharedResourceCompatibilityTier) << endl;
		LogInfo(LogDirectX) << format(formatStr, "Native16BitShaderOpsSupported:", (bool)caps4.Native16BitShaderOpsSupported) << endl;
		LogInfo(LogDirectX) << endl;
	}

	if (HasCaps(5))
	{
		LogInfo(LogDirectX) << "    D3D12_FEATURE_D3D12_OPTIONS5" << endl;
		LogInfo(LogDirectX) << format(formatStr, "SRVOnlyTiledResourceTier3:", (bool)caps5.SRVOnlyTiledResourceTier3) << endl;
		LogInfo(LogDirectX) << format(formatStr, "RenderPassesTier:", caps5.RenderPassesTier) << endl;
		LogInfo(LogDirectX) << format(formatStr, "RaytracingTier:", caps5.RaytracingTier) << endl;
		LogInfo(LogDirectX) << endl;
	}

	if (HasCaps(6))
	{
		LogInfo(LogDirectX) << "    D3D12_FEATURE_D3D12_OPTIONS6" << endl;
		LogInfo(LogDirectX) << format(formatStr, "AdditionalShadingRatesSupported:", (bool)caps6.AdditionalShadingRatesSupported) << endl;
		LogInfo(LogDirectX) << format(formatStr, "PerPrimitiveShadingRateSupportedWithViewportIndexing:", (bool)caps6.PerPrimitiveShadingRateSupportedWithViewportIndexing) << endl;
		LogInfo(LogDirectX) << format(formatStr, "VariableShadingRateTier:", caps6.VariableShadingRateTier) << endl;
		LogInfo(LogDirectX) << format(formatStr, "ShadingRateImageTileSize:", caps6.ShadingRateImageTileSize) << endl;
		LogInfo(LogDirectX) << format(formatStr, "BackgroundProcessingSupported:", caps6.BackgroundProcessingSupported) << endl;
		LogInfo(LogDirectX) << endl;
	}

	if (HasCaps(7))
	{
		LogInfo(LogDirectX) << "    D3D12_FEATURE_D3D12_OPTIONS7" << endl;
		LogInfo(LogDirectX) << format(formatStr, "MeshShaderTier:", caps7.MeshShaderTier) << endl;
		LogInfo(LogDirectX) << format(formatStr, "SamplerFeedbackTier:", caps7.SamplerFeedbackTier) << endl;
		LogInfo(LogDirectX) << endl;
	}

	if (HasCaps(8))
	{
		LogInfo(LogDirectX) << "    D3D12_FEATURE_D3D12_OPTIONS8" << endl;
		LogInfo(LogDirectX) << format(formatStr, "UnalignedBlockTexturesSupported:", (bool)caps8.UnalignedBlockTexturesSupported) << endl;
		LogInfo(LogDirectX) << endl;
	}

	if (HasCaps(9))
	{
		LogInfo(LogDirectX) << "    D3D12_FEATURE_D3D12_OPTIONS9" << endl;
		LogInfo(LogDirectX) << format(formatStr, "MeshShaderPipelineStatsSupported:", (bool)caps9.MeshShaderPipelineStatsSupported) << endl;
		LogInfo(LogDirectX) << format(formatStr, "MeshShaderSupportsFullRangeRenderTargetArrayIndex:", (bool)caps9.MeshShaderSupportsFullRangeRenderTargetArrayIndex) << endl;
		LogInfo(LogDirectX) << format(formatStr, "AtomicInt64OnTypedResourceSupported:", (bool)caps9.AtomicInt64OnTypedResourceSupported) << endl;
		LogInfo(LogDirectX) << format(formatStr, "AtomicInt64OnGroupSharedSupported:", (bool)caps9.AtomicInt64OnGroupSharedSupported) << endl;
		LogInfo(LogDirectX) << format(formatStr, "DerivativesInMeshAndAmplificationShadersSupported:", (bool)caps9.DerivativesInMeshAndAmplificationShadersSupported) << endl;
		LogInfo(LogDirectX) << format(formatStr, "WaveMMATier:", caps9.WaveMMATier) << endl;
		LogInfo(LogDirectX) << endl;
	}

	if (HasCaps(10))
	{
		LogInfo(LogDirectX) << "    D3D12_FEATURE_D3D12_OPTIONS10" << endl;
		LogInfo(LogDirectX) << format(formatStr, "VariableRateShadingSumCombinerSupported:", (bool)caps10.VariableRateShadingSumCombinerSupported) << endl;
		LogInfo(LogDirectX) << format(formatStr, "MeshShaderPerPrimitiveShadingRateSupported:", (bool)caps10.MeshShaderPerPrimitiveShadingRateSupported) << endl;
		LogInfo(LogDirectX) << endl;

	}

	if (HasCaps(11))
	{
		LogInfo(LogDirectX) << "    D3D12_FEATURE_D3D12_OPTIONS11" << endl;
		LogInfo(LogDirectX) << format(formatStr, "AtomicInt64OnDescriptorHeapResourceSupported:", (bool)caps11.AtomicInt64OnDescriptorHeapResourceSupported) << endl;
		LogInfo(LogDirectX) << endl;
	}

	if (HasCaps(12))
	{
		LogInfo(LogDirectX) << "    D3D12_FEATURE_D3D12_OPTIONS12" << endl;
		LogInfo(LogDirectX) << format(formatStr, "MSPrimitivesPipelineStatisticIncludesCulledPrimitives:", caps12.MSPrimitivesPipelineStatisticIncludesCulledPrimitives) << endl;
		LogInfo(LogDirectX) << format(formatStr, "EnhancedBarriersSupported:", (bool)caps12.EnhancedBarriersSupported) << endl;
		LogInfo(LogDirectX) << format(formatStr, "RelaxedFormatCastingSupported:", (bool)caps12.RelaxedFormatCastingSupported) << endl;
		LogInfo(LogDirectX) << endl;

	}

	if (HasCaps(13))
	{
		LogInfo(LogDirectX) << "    D3D12_FEATURE_D3D12_OPTIONS13" << endl;
		LogInfo(LogDirectX) << format(formatStr, "UnrestrictedBufferTextureCopyPitchSupported:", (bool)caps13.UnrestrictedBufferTextureCopyPitchSupported) << endl;
		LogInfo(LogDirectX) << format(formatStr, "UnrestrictedVertexElementAlignmentSupported:", (bool)caps13.UnrestrictedVertexElementAlignmentSupported) << endl;
		LogInfo(LogDirectX) << format(formatStr, "InvertedViewportHeightFlipsYSupported:", (bool)caps13.InvertedViewportHeightFlipsYSupported) << endl;
		LogInfo(LogDirectX) << format(formatStr, "InvertedViewportDepthFlipsZSupported:", (bool)caps13.InvertedViewportDepthFlipsZSupported) << endl;
		LogInfo(LogDirectX) << format(formatStr, "TextureCopyBetweenDimensionsSupported:", (bool)caps13.TextureCopyBetweenDimensionsSupported) << endl;
		LogInfo(LogDirectX) << format(formatStr, "AlphaBlendFactorSupported:", (bool)caps13.AlphaBlendFactorSupported) << endl;
		LogInfo(LogDirectX) << endl;
	}
}


D3D_FEATURE_LEVEL DeviceCaps::GetHighestFeatureLevel(ID3D12Device* device, D3D_FEATURE_LEVEL minFeatureLevel)
{
	const D3D_FEATURE_LEVEL featureLevels[]
	{
		D3D_FEATURE_LEVEL_12_2,
		D3D_FEATURE_LEVEL_12_1,
		D3D_FEATURE_LEVEL_12_0,
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0
	};

	D3D12_FEATURE_DATA_FEATURE_LEVELS featureLevelCaps{};
	featureLevelCaps.pFeatureLevelsRequested = featureLevels;
	featureLevelCaps.NumFeatureLevels = _countof(featureLevels);

	if (SUCCEEDED(device->CheckFeatureSupport(D3D12_FEATURE_FEATURE_LEVELS, &featureLevelCaps, sizeof(featureLevelCaps))))
	{
		return featureLevelCaps.MaxSupportedFeatureLevel;
	}

	return minFeatureLevel;
}


D3D_SHADER_MODEL DeviceCaps::GetHighestShaderModel(ID3D12Device* device)
{
	const D3D_SHADER_MODEL shaderModels[]
	{
		D3D_SHADER_MODEL_6_7,
		D3D_SHADER_MODEL_6_6,
		D3D_SHADER_MODEL_6_5,
		D3D_SHADER_MODEL_6_4,
		D3D_SHADER_MODEL_6_3,
		D3D_SHADER_MODEL_6_2,
		D3D_SHADER_MODEL_6_1,
		D3D_SHADER_MODEL_6_0,
	};

	D3D12_FEATURE_DATA_SHADER_MODEL featureShaderModel{};
	for (const auto shaderModel : shaderModels)
	{
		featureShaderModel.HighestShaderModel = shaderModel;
		if (SUCCEEDED(device->CheckFeatureSupport(D3D12_FEATURE_SHADER_MODEL, &featureShaderModel, sizeof(featureShaderModel))))
		{
			return featureShaderModel.HighestShaderModel;
		}
	}

	return D3D_SHADER_MODEL_5_1;
}

} // namespace Luna::DX12