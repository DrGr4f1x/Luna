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
#include "Graphics\DX12\DeviceManager12.h"

using namespace std;


namespace Luna::DX12
{

void FillCaps(ID3D12Device* device, DeviceCaps& caps)
{
	HRESULT hr{};

	caps.api = GraphicsApi::D3D12;

	D3D12_FEATURE_DATA_D3D12_OPTIONS options{};
	hr = device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &options, sizeof(options));
	if (FAILED(hr))
	{
		LogWarning(LogDirectX) << format("ID3D12Device::CheckFeatureSupport(options) failed, result = {:#x}", hr);
	}
	caps.tiers.memory = options.ResourceHeapTier == D3D12_RESOURCE_HEAP_TIER_2 ? 1 : 0;

	D3D12_FEATURE_DATA_D3D12_OPTIONS1 options1{};
	hr = device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS1, &options1, sizeof(options1));
	if (FAILED(hr))
	{
		LogWarning(LogDirectX) << format("ID3D12Device::CheckFeatureSupport(options1) failed, result = {:#x}", hr);
	}

	D3D12_FEATURE_DATA_D3D12_OPTIONS2 options2{};
	hr = device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS2, &options2, sizeof(options2));
	if (FAILED(hr))
	{
		LogWarning(LogDirectX) << format("ID3D12Device::CheckFeatureSupport(options2) failed, result = {:#x}", hr);
	}

	D3D12_FEATURE_DATA_D3D12_OPTIONS3 options3{};
	hr = device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS3, &options3, sizeof(options3));
	if (FAILED(hr))
	{
		LogWarning(LogDirectX) << format("ID3D12Device::CheckFeatureSupport(options3) failed, result = {:#x}", hr);
	}
	caps.features.copyQueueTimestamp = options3.CopyQueueTimestampQueriesSupported;

	D3D12_FEATURE_DATA_D3D12_OPTIONS4 options4{};
	hr = device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS4, &options4, sizeof(options4));
	if (FAILED(hr))
	{
		LogWarning(LogDirectX) << format("ID3D12Device::CheckFeatureSupport(options4) failed, result = {:#x}", hr);
	}

	// Windows 10 1809 (build 17763)
	D3D12_FEATURE_DATA_D3D12_OPTIONS5 options5{};
	hr = device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &options5, sizeof(options5));
	if (FAILED(hr))
	{
		LogWarning(LogDirectX) << format("ID3D12Device::CheckFeatureSupport(options5) failed, result = {:#x}", hr);
	}
	caps.features.rayTracing = options5.RaytracingTier != 0;
	caps.tiers.rayTracing = (uint8_t)(options5.RaytracingTier - D3D12_RAYTRACING_TIER_1_0 + 1);

	// Windows 10 1903 (build 18362)
	D3D12_FEATURE_DATA_D3D12_OPTIONS6 options6{};
	hr = device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS6, &options6, sizeof(options6));
	if (FAILED(hr))
	{
		LogWarning(LogDirectX) << format("ID3D12Device::CheckFeatureSupport(options6) failed, result = {:#x}", hr);
	}
	caps.tiers.shadingRate = (uint8_t)options6.VariableShadingRateTier;
	caps.other.shadingRateAttachmentTileSize = (uint8_t)options6.ShadingRateImageTileSize;
	caps.features.additionalShadingRates = options6.AdditionalShadingRatesSupported;

	// Windows 10 2004 (build 19041)
	D3D12_FEATURE_DATA_D3D12_OPTIONS7 options7{};
	hr = device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS7, &options7, sizeof(options7));
	if (FAILED(hr))
	{
		LogWarning(LogDirectX) << format("ID3D12Device::CheckFeatureSupport(options7) failed, result = {:#x}", hr);
	}
	caps.features.meshShader = options7.MeshShaderTier >= D3D12_MESH_SHADER_TIER_1;

#ifdef USE_AGILITY_SDK
	// Windows 11 21H2 (build 22000)
	D3D12_FEATURE_DATA_D3D12_OPTIONS8 options8{};
	hr = device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS8, &options8, sizeof(options8));
	if (FAILED(hr))
	{
		LogWarning(LogDirectX) << format("ID3D12Device::CheckFeatureSupport(options8) failed, result = {:#x}", hr);
	}

	D3D12_FEATURE_DATA_D3D12_OPTIONS9 options9{};
	hr = device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS9, &options9, sizeof(options9));
	if (FAILED(hr))
	{
		LogWarning(LogDirectX) << format("ID3D12Device::CheckFeatureSupport(options9) failed, result = {:#x}", hr);
	}
	caps.features.meshShaderPipelineStats = options9.MeshShaderPipelineStatsSupported;

	D3D12_FEATURE_DATA_D3D12_OPTIONS10 options10{};
	hr = device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS10, &options10, sizeof(options10));
	if (FAILED(hr))
	{
		LogWarning(LogDirectX) << format("ID3D12Device::CheckFeatureSupport(options10) failed, result = {:#x}", hr);
	}

	D3D12_FEATURE_DATA_D3D12_OPTIONS11 options11{};
	hr = device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS11, &options11, sizeof(options11));
	if (FAILED(hr))
	{
		LogWarning(LogDirectX) << format("ID3D12Device::CheckFeatureSupport(options11) failed, result = {:#x}", hr);
	}

	// Windows 11 22H2 (build 22621)
	D3D12_FEATURE_DATA_D3D12_OPTIONS12 options12{};
	hr = device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS12, &options12, sizeof(options12));
	if (FAILED(hr))
	{
		LogWarning(LogDirectX) << format("ID3D12Device::CheckFeatureSupport(options12) failed, result = {:#x}", hr);
	}
	caps.features.enhancedBarriers = options12.EnhancedBarriersSupported;

	D3D12_FEATURE_DATA_D3D12_OPTIONS13 options13{};
	hr = device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS13, &options13, sizeof(options13));
	if (FAILED(hr))
	{
		LogWarning(LogDirectX) << format("ID3D12Device::CheckFeatureSupport(options13) failed, result = {:#x}", hr);
	}
	caps.memoryAlignment.uploadBufferTextureRow = options13.UnrestrictedBufferTextureCopyPitchSupported ? 1 : D3D12_TEXTURE_DATA_PITCH_ALIGNMENT;
	caps.memoryAlignment.uploadBufferTextureSlice = options13.UnrestrictedBufferTextureCopyPitchSupported ? 1 : D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT;
	caps.features.viewportOriginBottomLeft = options13.InvertedViewportHeightFlipsYSupported ? 1 : 0;

	// Agility SDK
	D3D12_FEATURE_DATA_D3D12_OPTIONS14 options14{};
	hr = device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS14, &options14, sizeof(options14));
	if (FAILED(hr))
	{
		LogWarning(LogDirectX) << format("ID3D12Device::CheckFeatureSupport(options14) failed, result = {:#x}", hr);
	}
	caps.features.independentFrontAndBackStencilReferenceAndMasks = options14.IndependentFrontAndBackStencilRefMaskSupported ? true : false;

	D3D12_FEATURE_DATA_D3D12_OPTIONS15 options15{};
	hr = device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS15, &options15, sizeof(options15));
	if (FAILED(hr))
	{
		LogWarning(LogDirectX) << format("ID3D12Device::CheckFeatureSupport(options15) failed, result = {:#x}", hr);
	}

	D3D12_FEATURE_DATA_D3D12_OPTIONS16 options16{};
	hr = device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS16, &options16, sizeof(options16));
	if (FAILED(hr))
	{
		LogWarning(LogDirectX) << format("ID3D12Device::CheckFeatureSupport(options16) failed, result = {:#x}", hr);
	}
	caps.memory.deviceUploadHeapSize = options16.GPUUploadHeapSupported ? caps.adapterInfo.dedicatedVideoMemory : 0;
	caps.features.dynamicDepthBias = options16.DynamicDepthBiasSupported;

	D3D12_FEATURE_DATA_D3D12_OPTIONS17 options17{};
	hr = device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS17, &options17, sizeof(options17));
	if (FAILED(hr))
	{
		LogWarning(LogDirectX) << format("ID3D12Device::CheckFeatureSupport(options17) failed, result = {:#x}", hr);
	}

	D3D12_FEATURE_DATA_D3D12_OPTIONS18 options18{};
	hr = device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS18, &options18, sizeof(options18));
	if (FAILED(hr))
	{
		LogWarning(LogDirectX) << format("ID3D12Device::CheckFeatureSupport(options18) failed, result = {:#x}", hr);
	}

	D3D12_FEATURE_DATA_D3D12_OPTIONS19 options19{};
	hr = device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS19, &options19, sizeof(options19));
	if (FAILED(hr))
	{
		LogWarning(LogDirectX) << format("ID3D12Device::CheckFeatureSupport(options19) failed, result = {:#x}", hr);
	}
	caps.features.rasterizerDesc2 = options19.RasterizerDesc2Supported;

	D3D12_FEATURE_DATA_D3D12_OPTIONS20 options20{};
	hr = device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS20, &options20, sizeof(options20));
	if (FAILED(hr))
	{
		LogWarning(LogDirectX) << format("ID3D12Device::CheckFeatureSupport(options20) failed, result = {:#x}", hr);
	}

	D3D12_FEATURE_DATA_D3D12_OPTIONS21 options21{};
	hr = device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS21, &options21, sizeof(options21));
	if (FAILED(hr))
	{
		LogWarning(LogDirectX) << format("ID3D12Device::CheckFeatureSupport(options21) failed, result = {:#x}", hr);
	}
#else
	caps.memoryAlignment.uploadBufferTextureRow = D3D12_TEXTURE_DATA_PITCH_ALIGNMENT;
	caps.memoryAlignment.uploadBufferTextureSlice = D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT;
#endif

	// Feature level
	const std::array<D3D_FEATURE_LEVEL, 5> levelsList =
	{
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_12_0,
		D3D_FEATURE_LEVEL_12_1,
		D3D_FEATURE_LEVEL_12_2,
	};

	D3D12_FEATURE_DATA_FEATURE_LEVELS levels{};
	levels.NumFeatureLevels = (uint32_t)levelsList.size();
	levels.pFeatureLevelsRequested = levelsList.data();
	hr = device->CheckFeatureSupport(D3D12_FEATURE_FEATURE_LEVELS, &levels, sizeof(levels));
	if (FAILED(hr))
	{
		LogWarning(LogDirectX) << format("ID3D12Device::CheckFeatureSupport((D3D12_FEATURE_FEATURE_LEVELS) failed, result = {:#x}", hr);
	}

	// Shader model
#if (D3D12_SDK_VERSION >= 6)
	D3D12_FEATURE_DATA_SHADER_MODEL shaderModel = { (D3D_SHADER_MODEL)D3D_HIGHEST_SHADER_MODEL };
#else
	D3D12_FEATURE_DATA_SHADER_MODEL shaderModel = { (D3D_SHADER_MODEL)0x69 };
#endif
	for (; shaderModel.HighestShaderModel >= D3D_SHADER_MODEL_6_0; (*(uint32_t*)&shaderModel.HighestShaderModel)--)
	{
		hr = device->CheckFeatureSupport(D3D12_FEATURE_SHADER_MODEL, &shaderModel, sizeof(shaderModel));
		if (SUCCEEDED(hr))
		{
			break;
		}
	}
	if (shaderModel.HighestShaderModel < D3D_SHADER_MODEL_6_0)
	{
		shaderModel.HighestShaderModel = D3D_SHADER_MODEL_5_1;
	}

	caps.shaderModel = (uint8_t)((shaderModel.HighestShaderModel / 0xF) * 10 + (shaderModel.HighestShaderModel & 0xF));

	// Viewport
	caps.viewport.maxNum = D3D12_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
	caps.viewport.boundsMin = D3D12_VIEWPORT_BOUNDS_MIN;
	caps.viewport.boundsMax = D3D12_VIEWPORT_BOUNDS_MAX;

	// Dimensions
	caps.dimensions.attachmentMaxDim = D3D12_REQ_RENDER_TO_BUFFER_WINDOW_WIDTH;
	caps.dimensions.attachmentLayerMaxNum = D3D12_REQ_TEXTURE2D_ARRAY_AXIS_DIMENSION;
	caps.dimensions.texture1DMaxDim = D3D12_REQ_TEXTURE1D_U_DIMENSION;
	caps.dimensions.texture2DMaxDim = D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION;
	caps.dimensions.texture3DMaxDim = D3D12_REQ_TEXTURE3D_U_V_OR_W_DIMENSION;
	caps.dimensions.textureCubeMaxDim = D3D12_REQ_TEXTURECUBE_DIMENSION;
	caps.dimensions.textureLayerMaxNum = D3D12_REQ_TEXTURE2D_ARRAY_AXIS_DIMENSION;
	caps.dimensions.typedBufferMaxDim = 1 << D3D12_REQ_BUFFER_RESOURCE_TEXEL_COUNT_2_TO_EXP;

	// Precision
	caps.precision.viewportBits = D3D12_SUBPIXEL_FRACTIONAL_BIT_COUNT;
	caps.precision.subPixelBits = D3D12_SUBPIXEL_FRACTIONAL_BIT_COUNT;
	caps.precision.subTexelBits = D3D12_SUBTEXEL_FRACTIONAL_BIT_COUNT;
	caps.precision.mipmapBits = D3D12_MIP_LOD_FRACTIONAL_BIT_COUNT;

	// Memory
	caps.memory.allocationMaxNum = 0xFFFFFFFF;
	caps.memory.samplerAllocationMaxNum = D3D12_REQ_SAMPLER_OBJECT_COUNT_PER_DEVICE;
	caps.memory.constantBufferMaxRange = D3D12_REQ_IMMEDIATE_CONSTANT_BUFFER_ELEMENT_COUNT * 16;
	caps.memory.storageBufferMaxRange = 1 << D3D12_REQ_BUFFER_RESOURCE_TEXEL_COUNT_2_TO_EXP;
	caps.memory.bufferTextureGranularity = D3D12_SMALL_RESOURCE_PLACEMENT_ALIGNMENT;
	caps.memory.bufferMaxSize = D3D12_REQ_RESOURCE_SIZE_IN_MEGABYTES_EXPRESSION_C_TERM * 1024ull * 1024ull;

	// Memory alignment
	caps.memoryAlignment.shaderBindingTable = D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT;
	caps.memoryAlignment.bufferShaderResourceOffset = D3D12_RAW_UAV_SRV_BYTE_ALIGNMENT;
	caps.memoryAlignment.constantBufferOffset = D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT;
	caps.memoryAlignment.scratchBufferOffset = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT;
	caps.memoryAlignment.accelerationStructureOffset = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT;
#if D3D12_HAS_OPACITY_MICROMAP
	caps.memoryAlignment.micromapOffset = D3D12_RAYTRACING_OPACITY_MICROMAP_ARRAY_BYTE_ALIGNMENT;
#endif

	// Pipeline layout
	caps.pipelineLayout.descriptorSetMaxNum = ROOT_SIGNATURE_DWORD_NUM / 1;
	caps.pipelineLayout.rootConstantMaxSize = sizeof(uint32_t) * ROOT_SIGNATURE_DWORD_NUM / 1;
	caps.pipelineLayout.rootDescriptorMaxNum = ROOT_SIGNATURE_DWORD_NUM / 2;

	// Descriptor set
	// https://learn.microsoft.com/en-us/windows/win32/direct3d12/hardware-support
	if (options.ResourceBindingTier == D3D12_RESOURCE_BINDING_TIER_1)
	{
		caps.descriptorSet.samplerMaxNum = 16;
		caps.descriptorSet.constantBufferMaxNum = 14;
		caps.descriptorSet.storageBufferMaxNum = levels.MaxSupportedFeatureLevel >= D3D_FEATURE_LEVEL_11_1 ? 64 : 8;
		caps.descriptorSet.textureMaxNum = 128;
	}
	else if (options.ResourceBindingTier == D3D12_RESOURCE_BINDING_TIER_2)
	{
		caps.descriptorSet.samplerMaxNum = 2048;
		caps.descriptorSet.constantBufferMaxNum = 14;
		caps.descriptorSet.storageBufferMaxNum = 64;
		caps.descriptorSet.textureMaxNum = D3D12_MAX_SHADER_VISIBLE_DESCRIPTOR_HEAP_SIZE_TIER_2;
	}
	else
	{
		caps.descriptorSet.samplerMaxNum = 2048;
		caps.descriptorSet.constantBufferMaxNum = D3D12_MAX_SHADER_VISIBLE_DESCRIPTOR_HEAP_SIZE_TIER_2;
		caps.descriptorSet.storageBufferMaxNum = D3D12_MAX_SHADER_VISIBLE_DESCRIPTOR_HEAP_SIZE_TIER_2;
		caps.descriptorSet.textureMaxNum = D3D12_MAX_SHADER_VISIBLE_DESCRIPTOR_HEAP_SIZE_TIER_2;
	}
	caps.descriptorSet.storageTextureMaxNum = caps.descriptorSet.storageBufferMaxNum;

	caps.descriptorSet.updateAfterSet.samplerMaxNum = caps.descriptorSet.samplerMaxNum;
	caps.descriptorSet.updateAfterSet.constantBufferMaxNum = caps.descriptorSet.constantBufferMaxNum;
	caps.descriptorSet.updateAfterSet.storageBufferMaxNum = caps.descriptorSet.storageBufferMaxNum;
	caps.descriptorSet.updateAfterSet.textureMaxNum = caps.descriptorSet.textureMaxNum;
	caps.descriptorSet.updateAfterSet.storageTextureMaxNum = caps.descriptorSet.storageTextureMaxNum;

	// Zero out caps.descriptorBuffer (Vulkan-only)
	ZeroMemory(&caps.descriptorBuffer, sizeof(caps.descriptorBuffer));

	caps.shaderStage.descriptorSamplerMaxNum = caps.descriptorSet.samplerMaxNum;
	caps.shaderStage.descriptorConstantBufferMaxNum = caps.descriptorSet.constantBufferMaxNum;
	caps.shaderStage.descriptorStorageBufferMaxNum = caps.descriptorSet.storageBufferMaxNum;
	caps.shaderStage.descriptorTextureMaxNum = caps.descriptorSet.textureMaxNum;
	caps.shaderStage.descriptorStorageTextureMaxNum = caps.descriptorSet.storageTextureMaxNum;
	caps.shaderStage.resourceMaxNum = caps.descriptorSet.textureMaxNum;

	caps.shaderStage.updateAfterSet.descriptorSamplerMaxNum = caps.shaderStage.descriptorSamplerMaxNum;
	caps.shaderStage.updateAfterSet.descriptorConstantBufferMaxNum = caps.shaderStage.descriptorConstantBufferMaxNum;
	caps.shaderStage.updateAfterSet.descriptorStorageBufferMaxNum = caps.shaderStage.descriptorStorageBufferMaxNum;
	caps.shaderStage.updateAfterSet.descriptorTextureMaxNum = caps.shaderStage.descriptorTextureMaxNum;
	caps.shaderStage.updateAfterSet.descriptorStorageTextureMaxNum = caps.shaderStage.descriptorStorageTextureMaxNum;
	caps.shaderStage.updateAfterSet.resourceMaxNum = caps.shaderStage.resourceMaxNum;

	caps.shaderStage.vertex.attributeMaxNum = D3D12_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT;
	caps.shaderStage.vertex.streamMaxNum = D3D12_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT;
	caps.shaderStage.vertex.outputComponentMaxNum = D3D12_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT * 4;

	caps.shaderStage.hull.generationMaxLevel = D3D12_HS_MAXTESSFACTOR_UPPER_BOUND;
	caps.shaderStage.hull.patchPointMaxNum = D3D12_IA_PATCH_MAX_CONTROL_POINT_COUNT;
	caps.shaderStage.hull.perVertexInputComponentMaxNum = D3D12_HS_CONTROL_POINT_PHASE_INPUT_REGISTER_COUNT * D3D12_HS_CONTROL_POINT_REGISTER_COMPONENTS;
	caps.shaderStage.hull.perVertexOutputComponentMaxNum = D3D12_HS_CONTROL_POINT_PHASE_OUTPUT_REGISTER_COUNT * D3D12_HS_CONTROL_POINT_REGISTER_COMPONENTS;
	caps.shaderStage.hull.perPatchOutputComponentMaxNum = D3D12_HS_OUTPUT_PATCH_CONSTANT_REGISTER_SCALAR_COMPONENTS;
	caps.shaderStage.hull.totalOutputComponentMaxNum
		= caps.shaderStage.hull.patchPointMaxNum * caps.shaderStage.hull.perVertexOutputComponentMaxNum
		+ caps.shaderStage.hull.perPatchOutputComponentMaxNum;

	caps.shaderStage.domain.inputComponentMaxNum = D3D12_DS_INPUT_CONTROL_POINT_REGISTER_COUNT * D3D12_DS_INPUT_CONTROL_POINT_REGISTER_COMPONENTS;
	caps.shaderStage.domain.outputComponentMaxNum = D3D12_DS_INPUT_CONTROL_POINT_REGISTER_COUNT * D3D12_DS_INPUT_CONTROL_POINT_REGISTER_COMPONENTS;

	caps.shaderStage.geometry.invocationMaxNum = D3D12_GS_MAX_INSTANCE_COUNT;
	caps.shaderStage.geometry.inputComponentMaxNum = D3D12_GS_INPUT_REGISTER_COUNT * D3D12_GS_INPUT_REGISTER_COMPONENTS;
	caps.shaderStage.geometry.outputComponentMaxNum = D3D12_GS_OUTPUT_REGISTER_COUNT * D3D12_GS_INPUT_REGISTER_COMPONENTS;
	caps.shaderStage.geometry.outputVertexMaxNum = D3D12_GS_MAX_OUTPUT_VERTEX_COUNT_ACROSS_INSTANCES;
	caps.shaderStage.geometry.totalOutputComponentMaxNum = D3D12_REQ_GS_INVOCATION_32BIT_OUTPUT_COMPONENT_LIMIT;

	caps.shaderStage.pixel.inputComponentMaxNum = D3D12_PS_INPUT_REGISTER_COUNT * D3D12_PS_INPUT_REGISTER_COMPONENTS;
	caps.shaderStage.pixel.attachmentMaxNum = D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT;
	caps.shaderStage.pixel.dualSourceAttachmentMaxNum = 1;

	caps.shaderStage.compute.workGroupMaxNum[0] = D3D12_CS_DISPATCH_MAX_THREAD_GROUPS_PER_DIMENSION;
	caps.shaderStage.compute.workGroupMaxNum[1] = D3D12_CS_DISPATCH_MAX_THREAD_GROUPS_PER_DIMENSION;
	caps.shaderStage.compute.workGroupMaxNum[2] = D3D12_CS_DISPATCH_MAX_THREAD_GROUPS_PER_DIMENSION;
	caps.shaderStage.compute.workGroupMaxDim[0] = D3D12_CS_THREAD_GROUP_MAX_X;
	caps.shaderStage.compute.workGroupMaxDim[1] = D3D12_CS_THREAD_GROUP_MAX_Y;
	caps.shaderStage.compute.workGroupMaxDim[2] = D3D12_CS_THREAD_GROUP_MAX_Z;
	caps.shaderStage.compute.workGroupInvocationMaxNum = D3D12_CS_THREAD_GROUP_MAX_THREADS_PER_GROUP;
	caps.shaderStage.compute.sharedMemoryMaxSize = D3D12_CS_THREAD_LOCAL_TEMP_REGISTER_POOL;

	caps.shaderStage.rayTracing.shaderGroupIdentifierSize = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
	caps.shaderStage.rayTracing.tableMaxStride = D3D12_RAYTRACING_MAX_SHADER_RECORD_STRIDE;
	caps.shaderStage.rayTracing.recursionMaxDepth = D3D12_RAYTRACING_MAX_DECLARABLE_TRACE_RECURSION_DEPTH;

	caps.shaderStage.meshControl.sharedMemoryMaxSize = 32 * 1024;
	caps.shaderStage.meshControl.workGroupInvocationMaxNum = 128;
	caps.shaderStage.meshControl.payloadMaxSize = 16 * 1024;

	caps.shaderStage.meshEvaluation.outputVerticesMaxNum = 256;
	caps.shaderStage.meshEvaluation.outputPrimitiveMaxNum = 256;
	caps.shaderStage.meshEvaluation.outputComponentMaxNum = 128;
	caps.shaderStage.meshEvaluation.sharedMemoryMaxSize = 28 * 1024;
	caps.shaderStage.meshEvaluation.workGroupInvocationMaxNum = 128;

	caps.wave.laneMinNum = options1.WaveLaneCountMin;
	caps.wave.laneMaxNum = options1.WaveLaneCountMax;

	caps.wave.derivativeOpsStages = ShaderStage::Pixel;
	if (caps.shaderModel >= 66)
	{
		caps.wave.derivativeOpsStages |= ShaderStage::Compute;
#ifdef USE_AGILITY_SDK
		if (options9.DerivativesInMeshAndAmplificationShadersSupported)
		{
			caps.wave.derivativeOpsStages |= ShaderStage::Amplification | ShaderStage::Mesh;
		}
#endif
	}

	if (caps.shaderModel >= 60 && options1.WaveOps)
	{
		caps.wave.waveOpsStages = ShaderStage::All;
		caps.wave.quadOpsStages = ShaderStage::Pixel | ShaderStage::Compute;
	}

	caps.other.timestampFrequencyHz = GetD3D12DeviceManager()->GetTimestampFrequency();
#ifdef D3D12_HAS_OPACITY_MICROMAP
	caps.other.micromapSubdivisionMaxLevel = D3D12_RAYTRACING_OPACITY_MICROMAP_OC1_MAX_SUBDIVISION_LEVEL;
#endif
	caps.other.drawIndirectMaxNum = (1ull << D3D12_REQ_DRAWINDEXED_INDEX_COUNT_2_TO_EXP) - 1;
	caps.other.samplerLodBiasMax = D3D12_MIP_LOD_BIAS_MAX;
	caps.other.samplerAnisotropyMax = D3D12_DEFAULT_MAX_ANISOTROPY;
	caps.other.texelOffsetMin = D3D12_COMMONSHADER_TEXEL_OFFSET_MAX_NEGATIVE;
	caps.other.texelOffsetMax = D3D12_COMMONSHADER_TEXEL_OFFSET_MAX_POSITIVE;
	caps.other.texelGatherOffsetMin = D3D12_COMMONSHADER_TEXEL_OFFSET_MAX_NEGATIVE;
	caps.other.texelGatherOffsetMax = D3D12_COMMONSHADER_TEXEL_OFFSET_MAX_POSITIVE;
	caps.other.clipDistanceMaxNum = D3D12_CLIP_OR_CULL_DISTANCE_COUNT;
	caps.other.cullDistanceMaxNum = D3D12_CLIP_OR_CULL_DISTANCE_COUNT;
	caps.other.combinedClipAndCullDistanceMaxNum = D3D12_CLIP_OR_CULL_DISTANCE_COUNT;
	caps.other.viewMaxNum = options3.ViewInstancingTier != D3D12_VIEW_INSTANCING_TIER_NOT_SUPPORTED ? D3D12_MAX_VIEW_INSTANCE_COUNT : 1;

	caps.tiers.conservativeRaster = (uint8_t)options.ConservativeRasterizationTier;
	caps.tiers.sampleLocations = (uint8_t)options2.ProgrammableSamplePositionsTier;
	caps.tiers.workGraphs = options21.WorkGraphsTier == D3D12_WORK_GRAPHS_TIER_1_0 ? 1 : 0;
	switch (options21.ExecuteIndirectTier)
	{
	case D3D12_EXECUTE_INDIRECT_TIER_1_0:
		caps.tiers.executeIndirect = 1;
		break;
	case D3D12_EXECUTE_INDIRECT_TIER_1_1:
		caps.tiers.executeIndirect = 2;
		break;
	}

	if (options.ResourceBindingTier == D3D12_RESOURCE_BINDING_TIER_3 && shaderModel.HighestShaderModel >= D3D_SHADER_MODEL_6_6)
	{
		caps.tiers.bindless = 2;
	}
	else if (levels.MaxSupportedFeatureLevel >= D3D_FEATURE_LEVEL_12_0)
	{
		caps.tiers.bindless = 1;
	}

	if (options.ResourceBindingTier == D3D12_RESOURCE_BINDING_TIER_3)
	{
		caps.tiers.resourceBinding = 2;
	}
	else if (options.ResourceBindingTier == D3D12_RESOURCE_BINDING_TIER_2)
	{
		caps.tiers.resourceBinding = 1;
	}

	caps.features.getMemoryDesc2 = true;
	caps.features.swapChain = true; // TODO: HasOutput();
	caps.features.lowLatency = false; // TODO HasNvExt();
	caps.features.micromap = caps.tiers.rayTracing >= 3;

	caps.features.textureFilterMinMax = levels.MaxSupportedFeatureLevel >= D3D_FEATURE_LEVEL_11_1 ? true : false;
	caps.features.logicOp = options.OutputMergerLogicOp != 0;
	caps.features.depthBoundsTest = options2.DepthBoundsTestSupported != 0;
	caps.features.drawIndirectCount = true;
	caps.features.lineSmoothing = true;
	caps.features.regionResolve = true;
	caps.features.flexibleMultiview = options3.ViewInstancingTier != D3D12_VIEW_INSTANCING_TIER_NOT_SUPPORTED;
	caps.features.layerBasedMultiview = options3.ViewInstancingTier != D3D12_VIEW_INSTANCING_TIER_NOT_SUPPORTED;
	caps.features.viewportBasedMultiview = options3.ViewInstancingTier != D3D12_VIEW_INSTANCING_TIER_NOT_SUPPORTED;
	caps.features.waitableSwapChain = true; // TODO: swap chain version >= 2?
	caps.features.pipelineStatistics = true;

	bool isShaderAtomicsF16Supported = false;
	bool isShaderAtomicsF32Supported = false;
	// TODO:
#if NRI_ENABLE_NVAPI
	if (HasNvExt()) {
		REPORT_ERROR_ON_BAD_NVAPI_STATUS(this, NvAPI_D3D12_IsNvShaderExtnOpCodeSupported(device, NV_EXTN_OP_FP16_ATOMIC, &isShaderAtomicsF16Supported));
		REPORT_ERROR_ON_BAD_NVAPI_STATUS(this, NvAPI_D3D12_IsNvShaderExtnOpCodeSupported(device, NV_EXTN_OP_FP32_ATOMIC, &isShaderAtomicsF32Supported));
	}
#endif

	caps.shaderFeatures.nativeI16 = options4.Native16BitShaderOpsSupported;
	caps.shaderFeatures.nativeF16 = options4.Native16BitShaderOpsSupported;
	caps.shaderFeatures.nativeI64 = options1.Int64ShaderOps;
	caps.shaderFeatures.nativeF64 = options.DoublePrecisionFloatShaderOps;
	caps.shaderFeatures.atomicsF16 = isShaderAtomicsF16Supported;
	caps.shaderFeatures.atomicsF32 = isShaderAtomicsF32Supported;
#ifdef USE_AGILITY_SDK
	caps.shaderFeatures.atomicsI64 = caps.shaderFeatures.atomicsI64 || options9.AtomicInt64OnTypedResourceSupported || options9.AtomicInt64OnGroupSharedSupported || options11.AtomicInt64OnDescriptorHeapResourceSupported;
#endif
	caps.shaderFeatures.viewportIndex = options.VPAndRTArrayIndexFromAnyShaderFeedingRasterizerSupportedWithoutGSEmulation;
	caps.shaderFeatures.layerIndex = options.VPAndRTArrayIndexFromAnyShaderFeedingRasterizerSupportedWithoutGSEmulation;
	caps.shaderFeatures.rasterizedOrderedView = options.ROVsSupported;
	caps.shaderFeatures.barycentric = options3.BarycentricsSupported;
	caps.shaderFeatures.storageReadWithoutFormat = true; // All desktop GPUs support it since 2014
	caps.shaderFeatures.storageWriteWithoutFormat = true;

	if (caps.wave.waveOpsStages != ShaderStage::None)
	{
		caps.shaderFeatures.waveQuery = true;
		caps.shaderFeatures.waveVote = true;
		caps.shaderFeatures.waveShuffle = true;
		caps.shaderFeatures.waveArithmetic = true;
		caps.shaderFeatures.waveReduction = true;
		caps.shaderFeatures.waveQuad = true;
	}
}

} // namespace Luna::DX12