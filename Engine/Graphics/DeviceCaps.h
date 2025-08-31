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

#include "Graphics\GraphicsCommon.h"


namespace Luna
{

struct DeviceCaps
{
	AdapterInfo adapterInfo;

	GraphicsApi api;
	uint32_t shaderModel; // major * 10 + minor

	// Viewport
	struct
	{
		uint32_t maxNum;
		uint16_t boundsMin;
		uint16_t boundsMax;
	} viewport;

	// Dimensions
	struct 
	{
		uint32_t typedBufferMaxDim;
		uint16_t attachmentMaxDim;
		uint16_t attachmentLayerMaxNum;
		uint16_t texture1DMaxDim;
		uint16_t texture2DMaxDim;
		uint16_t texture3DMaxDim;
		uint16_t textureCubeMaxDim;
		uint16_t textureLayerMaxNum;
	} dimensions;

	// Precision bits
	struct 
	{
		uint32_t viewportBits;
		uint32_t subPixelBits;
		uint32_t subTexelBits;
		uint32_t mipmapBits;
	} precision;

	// Memory
	struct 
	{
		uint64_t deviceUploadHeapSize; // ReBAR
		uint32_t allocationMaxNum;
		uint32_t samplerAllocationMaxNum;
		uint32_t constantBufferMaxRange;
		uint32_t storageBufferMaxRange;
		uint32_t bufferTextureGranularity;
		uint64_t bufferMaxSize;
	} memory;

	// Memory alignment
	struct 
	{
		uint32_t uploadBufferTextureRow;
		uint32_t uploadBufferTextureSlice;
		uint32_t shaderBindingTable;
		uint32_t bufferShaderResourceOffset;
		uint32_t constantBufferOffset;
		uint32_t scratchBufferOffset;
		uint32_t accelerationStructureOffset;
		uint32_t micromapOffset;
	} memoryAlignment;

	// Pipeline layout
	// D3D12 only: rootConstantSize + descriptorSetNum * 4 + rootDescriptorNum * 8 <= 256 (see "FitPipelineLayoutSettingsIntoDeviceLimits")
	struct {
		uint32_t descriptorSetMaxNum;
		uint32_t rootConstantMaxSize;
		uint32_t rootDescriptorMaxNum;
	} pipelineLayout;

	// Descriptor set
	struct 
	{
		uint32_t samplerMaxNum;
		uint32_t constantBufferMaxNum;
		uint32_t storageBufferMaxNum;
		uint32_t textureMaxNum;
		uint32_t storageTextureMaxNum;

		struct 
		{
			uint32_t samplerMaxNum;
			uint32_t constantBufferMaxNum;
			uint32_t storageBufferMaxNum;
			uint32_t textureMaxNum;
			uint32_t storageTextureMaxNum;
		} updateAfterSet;
	} descriptorSet;

	// Shader stages
	struct 
	{
		// Per stage resources
		uint32_t descriptorSamplerMaxNum;
		uint32_t descriptorConstantBufferMaxNum;
		uint32_t descriptorStorageBufferMaxNum;
		uint32_t descriptorTextureMaxNum;
		uint32_t descriptorStorageTextureMaxNum;
		uint32_t resourceMaxNum;

		struct 
		{
			uint32_t descriptorSamplerMaxNum;
			uint32_t descriptorConstantBufferMaxNum;
			uint32_t descriptorStorageBufferMaxNum;
			uint32_t descriptorTextureMaxNum;
			uint32_t descriptorStorageTextureMaxNum;
			uint32_t resourceMaxNum;
		} updateAfterSet;

		// Vertex
		struct 
		{
			uint32_t attributeMaxNum;
			uint32_t streamMaxNum;
			uint32_t outputComponentMaxNum;
		} vertex;

		// Tessellation control
		struct 
		{
			float generationMaxLevel;
			uint32_t patchPointMaxNum;
			uint32_t perVertexInputComponentMaxNum;
			uint32_t perVertexOutputComponentMaxNum;
			uint32_t perPatchOutputComponentMaxNum;
			uint32_t totalOutputComponentMaxNum;
		} tesselationControl;

		// Tessellation evaluation
		struct 
		{
			uint32_t inputComponentMaxNum;
			uint32_t outputComponentMaxNum;
		} tesselationEvaluation;

		// Geometry
		struct 
		{
			uint32_t invocationMaxNum;
			uint32_t inputComponentMaxNum;
			uint32_t outputComponentMaxNum;
			uint32_t outputVertexMaxNum;
			uint32_t totalOutputComponentMaxNum;
		} geometry;

		// Pixel
		struct 
		{
			uint32_t inputComponentMaxNum;
			uint32_t attachmentMaxNum;
			uint32_t dualSourceAttachmentMaxNum;
		} fragment;

		// Compute
		//  - a "dispatch" consists of "work groups" (aka "thread groups")
		//  - a "work group" consists of "waves" (aka "subgroups" or "warps")
		//  - a "wave" consists of "lanes", which can can be active, inactive or a helper:
		//    - active: the "lane" is performing its computations
		//    - inactive: the "lane" is part of the "wave" but is currently masked out
		//    - helper: these "lanes" are executed to provide auxiliary information (like derivatives) for active threads in the same 2x2 quad
		//  - "invocation" or "thread" is the more general term for a single shader instance, "lane" specifically refers to the position of that thread within a hardware "wave"
		//  - the concept of "wave/lane" execution applies to all shader stages
		struct 
		{
			uint32_t workGroupMaxNum[3];
			uint32_t workGroupMaxDim[3];
			uint32_t workGroupInvocationMaxNum;
			uint32_t sharedMemoryMaxSize;
		} compute;

		// Ray tracing
		struct 
		{
			uint32_t shaderGroupIdentifierSize;
			uint32_t tableMaxStride;
			uint32_t recursionMaxDepth;
		} rayTracing;

		// Mesh control
		struct 
		{
			uint32_t sharedMemoryMaxSize;
			uint32_t workGroupInvocationMaxNum;
			uint32_t payloadMaxSize;
		} meshControl;

		// Mesh evaluation
		struct 
		{
			uint32_t outputVerticesMaxNum;
			uint32_t outputPrimitiveMaxNum;
			uint32_t outputComponentMaxNum;
			uint32_t sharedMemoryMaxSize;
			uint32_t workGroupInvocationMaxNum;
		} meshEvaluation;
	} shaderStage;

	// Wave (subgroup)
	// https://github.com/microsoft/directxshadercompiler/wiki/wave-intrinsics
	// https://microsoft.github.io/DirectX-Specs/d3d/HLSL_SM_6_6_Derivatives.html
	struct 
	{
		uint32_t laneMinNum;
		uint32_t laneMaxNum;
		ShaderStage waveOpsStages;       // SM 6.0+ (see "shaderFeatures.waveX")
		ShaderStage quadOpsStages;       // SM 6.0+ (see "shaderFeatures.waveQuad")
		ShaderStage derivativeOpsStages; // SM 6.6+ (https://microsoft.github.io/DirectX-Specs/d3d/HLSL_SM_6_6_Derivatives.html#derivative-functions)
	} wave;

	// Other
	struct 
	{
		uint64_t timestampFrequencyHz;
		uint32_t micromapSubdivisionMaxLevel;
		uint32_t drawIndirectMaxNum;
		float samplerLodBiasMax;
		float samplerAnisotropyMax;
		int8_t texelGatherOffsetMin;
		int8_t texelOffsetMin;
		uint8_t texelOffsetMax;
		uint8_t texelGatherOffsetMax;
		uint8_t clipDistanceMaxNum;
		uint8_t cullDistanceMaxNum;
		uint8_t combinedClipAndCullDistanceMaxNum;
		uint8_t viewMaxNum;                         // multiview is supported if > 1
		uint8_t shadingRateAttachmentTileSize;      // square size
	} other;

	// Tiers (0 - unsupported)
	struct 
	{
		// https://microsoft.github.io/DirectX-Specs/d3d/ConservativeRasterization.html#tiered-support
		// 1 - 1/2 pixel uncertainty region and does not support post-snap degenerates
		// 2 - reduces the maximum uncertainty region to 1/256 and requires post-snap degenerates not be culled
		// 3 - maintains a maximum 1/256 uncertainty region and adds support for inner input coverage, aka "SV_InnerCoverage"
		uint8_t conservativeRaster;

		// https://microsoft.github.io/DirectX-Specs/d3d/ProgrammableSamplePositions.html#hardware-tiers
		// 1 - a single sample pattern can be specified to repeat for every pixel ("locationNum / sampleNum" ratio must be 1 in "CmdSetSampleLocations"),
		//     1x and 16x sample counts do not support programmable locations
		// 2 - four separate sample patterns can be specified for each pixel in a 2x2 grid ("locationNum / sampleNum" ratio can be 1 or 4 in "CmdSetSampleLocations"),
		//     all sample counts support programmable positions
		uint8_t sampleLocations;

		// https://microsoft.github.io/DirectX-Specs/d3d/Raytracing.html#checkfeaturesupport-structures
		// 1 - DXR 1.0: full raytracing functionality, except features below
		// 2 - DXR 1.1: adds - ray query, "CmdDispatchRaysIndirect", "GeometryIndex()" intrinsic, additional ray flags & vertex formats
		// 3 - DXR 1.2: adds - micromap, shader execution reordering
		uint8_t rayTracing;

		// https://microsoft.github.io/DirectX-Specs/d3d/VariableRateShading.html#feature-tiering
		// 1 - shading rate can be specified only per draw
		// 2 - adds: per primitive shading rate, per "shadingRateAttachmentTileSize" shading rate, combiners, "SV_ShadingRate" support
		uint8_t shadingRate;

		// https://microsoft.github.io/DirectX-Specs/d3d/ResourceBinding.html#limitations-on-static-samplers
		// 0 - ALL descriptors in range must be valid by the time the command list executes
		// 1 - only "CONSTANT_BUFFER" and "STORAGE" descriptors in range must be valid
		// 2 - only referenced descriptors must be valid
		uint8_t resourceBinding;

		// 1 - unbound arrays with dynamic indexing
		// 2 - D3D12 dynamic resources: https://microsoft.github.io/DirectX-Specs/d3d/HLSL_SM_6_6_DynamicResources.html
		uint8_t bindless;

		// https://learn.microsoft.com/en-us/windows/win32/api/d3d12/ne-d3d12-d3d12_resource_heap_tier
		// 1 - a "Memory" can support resources from all 3 categories: buffers, attachments, all other textures
		uint8_t memory;
	} tiers;

	// Features
	struct 
	{
		// Bigger
		uint32_t getMemoryDesc2 : 1; // "GetXxxMemoryDesc2" support (VK: requires "maintenance4", D3D: supported)
		uint32_t enhancedBarriers : 1; // VK: supported, D3D12: requires "AgilitySDK", D3D11: unsupported
		uint32_t swapChain : 1; // NRISwapChain
		uint32_t rayTracing : 1; // NRIRayTracing
		uint32_t meshShader : 1; // NRIMeshShader
		uint32_t lowLatency : 1; // NRILowLatency
		uint32_t micromap : 1; // see "Micromap"

		// Smaller
		uint32_t independentFrontAndBackStencilReferenceAndMasks : 1; // see "StencilAttachmentDesc::back"
		uint32_t textureFilterMinMax : 1; // see "ReductionMode"
		uint32_t logicOp : 1; // see "LogicOp"
		uint32_t depthBoundsTest : 1; // see "DepthAttachmentDesc::boundsTest"
		uint32_t drawIndirectCount : 1; // see "countBuffer" and "countBufferOffset"
		uint32_t lineSmoothing : 1; // see "RasterizationDesc::lineSmoothing"
		uint32_t copyQueueTimestamp : 1; // see "QueryType::TIMESTAMP_COPY_QUEUE"
		uint32_t meshShaderPipelineStats : 1; // see "PipelineStatisticsDesc"
		uint32_t dynamicDepthBias : 1; // see "CmdSetDepthBias"
		uint32_t additionalShadingRates : 1; // see "ShadingRate"
		uint32_t viewportOriginBottomLeft : 1; // see "Viewport"
		uint32_t regionResolve : 1; // see "CmdResolveTexture"
		uint32_t flexibleMultiview : 1; // see "Multiview::FLEXIBLE"
		uint32_t layerBasedMultiview : 1; // see "Multiview::LAYRED_BASED"
		uint32_t viewportBasedMultiview : 1; // see "Multiview::VIEWPORT_BASED"
		uint32_t presentFromCompute : 1; // see "SwapChainDesc::queue"
		uint32_t waitableSwapChain : 1; // see "SwapChainDesc::waitable"
		uint32_t pipelineStatistics : 1; // see "QueryType::PIPELINE_STATISTICS"
		uint32_t rasterizerDesc2 : 1; // Is D3D12_RASTERIZER_DESC2 available?
	} features;

	// Shader features
	// https://github.com/Microsoft/DirectXShaderCompiler/blob/main/docs/SPIR-V.rst
	struct 
	{
		uint32_t viewportIndex : 1; // SV_ViewportArrayIndex, always can be used in geometry shaders
		uint32_t layerIndex : 1; // SV_RenderTargetArrayIndex, always can be used in geometry shaders
		uint32_t clock : 1; // https://github.com/Microsoft/DirectXShaderCompiler/blob/main/docs/SPIR-V.rst#readclock
		uint32_t rasterizedOrderedView : 1; // https://microsoft.github.io/DirectX-Specs/d3d/RasterOrderViews.html (aka fragment shader interlock)
		uint32_t barycentric : 1; // https://github.com/microsoft/DirectXShaderCompiler/wiki/SV_Barycentrics
		uint32_t rayTracingPositionFetch : 1; // https://docs.vulkan.org/features/latest/features/proposals/VK_KHR_ray_tracing_position_fetch.html

		// https://learn.microsoft.com/en-us/windows/win32/direct3dhlsl/dx-graphics-hlsl-scalar
		// I32 and F32 are always supported
		uint32_t nativeI16 : 1; // "(u)int16_t"
		uint32_t nativeF16 : 1; // "float16_t"
		uint32_t nativeI64 : 1; // "(u)int64_t"
		uint32_t nativeF64 : 1; // "double"

		// https://learn.microsoft.com/en-us/windows/win32/direct3d11/direct3d-11-advanced-stages-cs-atomic-functions
		// https://microsoft.github.io/DirectX-Specs/d3d/HLSL_SM_6_6_Int64_and_Float_Atomics.html
		// I32 atomics are always supported (for others it can be partial support of SMEM, texture or buffer atomics)
		uint32_t atomicsI16 : 1; // "(u)int16_t" atomics
		uint32_t atomicsF16 : 1; // "float16_t" atomics
		uint32_t atomicsF32 : 1; // "float" atomics
		uint32_t atomicsI64 : 1; // "(u)int64_t" atomics
		uint32_t atomicsF64 : 1; // "double" atomics

		// https://learn.microsoft.com/en-us/windows/win32/direct3d12/typed-unordered-access-view-loads#using-unorm-and-snorm-typed-uav-loads-from-hlsl
		uint32_t storageReadWithoutFormat : 1; // NRI_FORMAT("unknown") is allowed for storage reads
		uint32_t storageWriteWithoutFormat : 1; // NRI_FORMAT("unknown") is allowed for storage writes

		// https://github.com/microsoft/directxshadercompiler/wiki/wave-intrinsics
		uint32_t waveQuery : 1; // WaveIsFirstLane, WaveGetLaneCount, WaveGetLaneIndex
		uint32_t waveVote : 1; // WaveActiveAllTrue, WaveActiveAnyTrue, WaveActiveAllEqual
		uint32_t waveShuffle : 1; // WaveReadLaneFirst, WaveReadLaneAt
		uint32_t waveArithmetic : 1; // WaveActiveSum, WaveActiveProduct, WaveActiveMin, WaveActiveMax, WavePrefixProduct, WavePrefixSum
		uint32_t waveReduction : 1; // WaveActiveCountBits, WaveActiveBitAnd, WaveActiveBitOr, WaveActiveBitXor, WavePrefixCountBits
		uint32_t waveQuad : 1; // QuadReadLaneAt, QuadReadAcrossX, QuadReadAcrossY, QuadReadAcrossDiagonal
	} shaderFeatures;

	void LogCaps();
};

} // namespace Luna