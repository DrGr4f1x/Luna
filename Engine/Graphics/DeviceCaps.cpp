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

#include "DeviceCaps.h"

#include "GraphicsCommon.h"

using namespace std;


namespace Luna
{

void DeviceCaps::LogCaps() const
{
	// TODO: Log basic device info here (driver, vendor, device name, etc)

	constexpr const char* formatStr = "    {:50} {}";
	constexpr const char* formatStr2 = "      {:48} {}";

	LogInfo(LogGraphics) << "[Device Caps]" << endl;

	LogInfo(LogGraphics) << "  [Viewport]" << endl;
	LogInfo(LogGraphics) << format(formatStr, "maxNum", viewport.maxNum) << endl;
	LogInfo(LogGraphics) << format(formatStr, "boundsMin", viewport.boundsMin) << endl;
	LogInfo(LogGraphics) << format(formatStr, "boundsMax", viewport.boundsMax) << endl;

	LogInfo(LogGraphics) << "  [Dimensions]" << endl;
	LogInfo(LogGraphics) << format(formatStr, "typedBufferMaxDim", dimensions.typedBufferMaxDim) << endl;
	LogInfo(LogGraphics) << format(formatStr, "attachmentMaxDim", dimensions.attachmentMaxDim) << endl;
	LogInfo(LogGraphics) << format(formatStr, "attachmentLayerMaxNum", dimensions.attachmentLayerMaxNum) << endl;
	LogInfo(LogGraphics) << format(formatStr, "texture1DMaxDim", dimensions.texture1DMaxDim) << endl;
	LogInfo(LogGraphics) << format(formatStr, "texture2DMaxDim", dimensions.texture2DMaxDim) << endl;
	LogInfo(LogGraphics) << format(formatStr, "texture3DMaxDim", dimensions.texture3DMaxDim) << endl;
	LogInfo(LogGraphics) << format(formatStr, "textureCubeMaxDim", dimensions.textureCubeMaxDim) << endl;
	LogInfo(LogGraphics) << format(formatStr, "textureLayerMaxNum", dimensions.textureLayerMaxNum) << endl;

	LogInfo(LogGraphics) << "  [Precision]" << endl;
	LogInfo(LogGraphics) << format(formatStr, "viewportBits", precision.viewportBits) << endl;
	LogInfo(LogGraphics) << format(formatStr, "subPixelBits", precision.subPixelBits) << endl;
	LogInfo(LogGraphics) << format(formatStr, "subTexelBits", precision.subTexelBits) << endl;
	LogInfo(LogGraphics) << format(formatStr, "mipmapBits", precision.mipmapBits) << endl;

	LogInfo(LogGraphics) << "  [Memory]" << endl;
	LogInfo(LogGraphics) << format(formatStr, "deviceUploadHeapSize", memory.deviceUploadHeapSize) << endl;
	LogInfo(LogGraphics) << format(formatStr, "allocationMaxNum", memory.allocationMaxNum) << endl;
	LogInfo(LogGraphics) << format(formatStr, "samplerAllocationMaxNum", memory.samplerAllocationMaxNum) << endl;
	LogInfo(LogGraphics) << format(formatStr, "constantBufferMaxRange", memory.constantBufferMaxRange) << endl;
	LogInfo(LogGraphics) << format(formatStr, "storageBufferMaxRange", memory.storageBufferMaxRange) << endl;
	LogInfo(LogGraphics) << format(formatStr, "bufferTextureGranularity", memory.bufferTextureGranularity) << endl;
	LogInfo(LogGraphics) << format(formatStr, "bufferMaxSize", memory.bufferMaxSize) << endl;

	LogInfo(LogGraphics) << "  [Memory Alignment]" << endl;
	LogInfo(LogGraphics) << format(formatStr, "uploadBufferTextureRow", memoryAlignment.uploadBufferTextureRow) << endl;
	LogInfo(LogGraphics) << format(formatStr, "uploadBufferTextureSlice", memoryAlignment.uploadBufferTextureSlice) << endl;
	LogInfo(LogGraphics) << format(formatStr, "shaderBindingTable", memoryAlignment.shaderBindingTable) << endl;
	LogInfo(LogGraphics) << format(formatStr, "bufferShaderResourceOffset", memoryAlignment.bufferShaderResourceOffset) << endl;
	LogInfo(LogGraphics) << format(formatStr, "constantBufferOffset", memoryAlignment.constantBufferOffset) << endl;
	LogInfo(LogGraphics) << format(formatStr, "scratchBufferOffset", memoryAlignment.scratchBufferOffset) << endl;
	LogInfo(LogGraphics) << format(formatStr, "accelerationStructureOffset", memoryAlignment.accelerationStructureOffset) << endl;
	LogInfo(LogGraphics) << format(formatStr, "micromapOffset", memoryAlignment.micromapOffset) << endl;

	LogInfo(LogGraphics) << "  [Pipeline Layout]" << endl;
	LogInfo(LogGraphics) << format(formatStr, "descriptorSetMaxNum", pipelineLayout.descriptorSetMaxNum) << endl;
	LogInfo(LogGraphics) << format(formatStr, "rootConstantMaxSize", pipelineLayout.rootConstantMaxSize) << endl;
	LogInfo(LogGraphics) << format(formatStr, "rootDescriptorMaxNum", pipelineLayout.rootDescriptorMaxNum) << endl;

	LogInfo(LogGraphics) << "  [Descriptor Set]" << endl;
	LogInfo(LogGraphics) << format(formatStr, "samplerMaxNum", descriptorSet.samplerMaxNum) << endl;
	LogInfo(LogGraphics) << format(formatStr, "constantBufferMaxNum", descriptorSet.constantBufferMaxNum) << endl;
	LogInfo(LogGraphics) << format(formatStr, "storageBufferMaxNum", descriptorSet.storageBufferMaxNum) << endl;
	LogInfo(LogGraphics) << format(formatStr, "textureMaxNum", descriptorSet.textureMaxNum) << endl;
	LogInfo(LogGraphics) << format(formatStr, "storageTextureMaxNum", descriptorSet.storageTextureMaxNum) << endl;
	LogInfo(LogGraphics) << "    [Update After Set]" << endl;
	LogInfo(LogGraphics) << format(formatStr2, "samplerMaxNum", descriptorSet.updateAfterSet.samplerMaxNum) << endl;
	LogInfo(LogGraphics) << format(formatStr2, "constantBufferMaxNum", descriptorSet.updateAfterSet.constantBufferMaxNum) << endl;
	LogInfo(LogGraphics) << format(formatStr2, "storageBufferMaxNum", descriptorSet.updateAfterSet.storageBufferMaxNum) << endl;
	LogInfo(LogGraphics) << format(formatStr2, "textureMaxNum", descriptorSet.updateAfterSet.textureMaxNum) << endl;
	LogInfo(LogGraphics) << format(formatStr2, "storageTextureMaxNum", descriptorSet.updateAfterSet.storageTextureMaxNum) << endl;

	LogInfo(LogGraphics) << "  [Descriptor Buffer]" << endl;
	LogInfo(LogGraphics) << format(formatStr, "combinedImageSamplerDescriptorSingleArray", descriptorBuffer.combinedImageSamplerDescriptorSingleArray) << endl;
	LogInfo(LogGraphics) << format(formatStr, "bufferlessPushDescriptors", descriptorBuffer.bufferlessPushDescriptors) << endl;
	LogInfo(LogGraphics) << format(formatStr, "allowSamplerImageViewPostSubmitCreation", descriptorBuffer.allowSamplerImageViewPostSubmitCreation) << endl;
	LogInfo(LogGraphics) << format(formatStr, "descriptorBufferOffsetAlignment", descriptorBuffer.descriptorBufferOffsetAlignment) << endl;
	LogInfo(LogGraphics) << format(formatStr, "maxDescriptorBufferBindings", descriptorBuffer.maxDescriptorBufferBindings) << endl;
	LogInfo(LogGraphics) << format(formatStr, "maxResourceDescriptorBufferBindings", descriptorBuffer.maxResourceDescriptorBufferBindings) << endl;
	LogInfo(LogGraphics) << format(formatStr, "maxSamplerDescriptorBufferBindings", descriptorBuffer.maxSamplerDescriptorBufferBindings) << endl;
	LogInfo(LogGraphics) << format(formatStr, "maxEmbeddedImmutableSamplerBindings", descriptorBuffer.maxEmbeddedImmutableSamplerBindings) << endl;
	LogInfo(LogGraphics) << format(formatStr, "maxEmbeddedImmutableSamplers", descriptorBuffer.maxEmbeddedImmutableSamplers) << endl;
	LogInfo(LogGraphics) << format(formatStr, "maxSamplerDescriptorBufferRange", descriptorBuffer.maxSamplerDescriptorBufferRange) << endl;
	LogInfo(LogGraphics) << format(formatStr, "maxResourceDescriptorBufferRange", descriptorBuffer.maxResourceDescriptorBufferRange) << endl;
	LogInfo(LogGraphics) << format(formatStr, "samplerDescriptorBufferAddressSpaceSize", descriptorBuffer.samplerDescriptorBufferAddressSpaceSize) << endl;
	LogInfo(LogGraphics) << format(formatStr, "resourceDescriptorBufferAddressSpaceSize", descriptorBuffer.resourceDescriptorBufferAddressSpaceSize) << endl;
	LogInfo(LogGraphics) << format(formatStr, "descriptorBufferAddressSpaceSize", descriptorBuffer.descriptorBufferAddressSpaceSize) << endl;
	LogInfo(LogGraphics) << "    [Descriptor Size]" << endl;
	LogInfo(LogGraphics) << format(formatStr2, "sampler", descriptorBuffer.descriptorSize.sampler) << endl;
	LogInfo(LogGraphics) << format(formatStr2, "combinedImageSampler", descriptorBuffer.descriptorSize.combinedImageSampler) << endl;
	LogInfo(LogGraphics) << format(formatStr2, "sampledImage", descriptorBuffer.descriptorSize.sampledImage) << endl;
	LogInfo(LogGraphics) << format(formatStr2, "storageImage", descriptorBuffer.descriptorSize.storageImage) << endl;
	LogInfo(LogGraphics) << format(formatStr2, "uniformTexelBuffer", descriptorBuffer.descriptorSize.uniformTexelBuffer) << endl;
	LogInfo(LogGraphics) << format(formatStr2, "robustUniformTexelBuffer", descriptorBuffer.descriptorSize.robustUniformTexelBuffer) << endl;
	LogInfo(LogGraphics) << format(formatStr2, "storageTexelBuffer", descriptorBuffer.descriptorSize.storageTexelBuffer) << endl;
	LogInfo(LogGraphics) << format(formatStr2, "robustStorageTexelBuffer", descriptorBuffer.descriptorSize.robustStorageTexelBuffer) << endl;
	LogInfo(LogGraphics) << format(formatStr2, "uniformBuffer", descriptorBuffer.descriptorSize.uniformBuffer) << endl;
	LogInfo(LogGraphics) << format(formatStr2, "robustUniformBuffer", descriptorBuffer.descriptorSize.robustUniformBuffer) << endl;
	LogInfo(LogGraphics) << format(formatStr2, "storageBuffer", descriptorBuffer.descriptorSize.storageBuffer) << endl;
	LogInfo(LogGraphics) << format(formatStr2, "robustStorageBuffer", descriptorBuffer.descriptorSize.robustStorageBuffer) << endl;
	LogInfo(LogGraphics) << format(formatStr2, "inputAttachment", descriptorBuffer.descriptorSize.inputAttachment) << endl;
	LogInfo(LogGraphics) << format(formatStr2, "accelerationStructure", descriptorBuffer.descriptorSize.accelerationStructure) << endl;


	LogInfo(LogGraphics) << "  [Shader Stage]" << endl;
	LogInfo(LogGraphics) << format(formatStr, "descriptorSamplerMaxNum", shaderStage.descriptorSamplerMaxNum) << endl;
	LogInfo(LogGraphics) << format(formatStr, "descriptorConstantBufferMaxNum", shaderStage.descriptorConstantBufferMaxNum) << endl;
	LogInfo(LogGraphics) << format(formatStr, "descriptorStorageBufferMaxNum", shaderStage.descriptorStorageBufferMaxNum) << endl;
	LogInfo(LogGraphics) << format(formatStr, "descriptorTextureMaxNum", shaderStage.descriptorTextureMaxNum) << endl;
	LogInfo(LogGraphics) << format(formatStr, "descriptorStorageTextureMaxNum", shaderStage.descriptorStorageTextureMaxNum) << endl;
	LogInfo(LogGraphics) << format(formatStr, "resourceMaxNum", shaderStage.resourceMaxNum) << endl;
	LogInfo(LogGraphics) << "    [Update After Set]" << endl;
	LogInfo(LogGraphics) << format(formatStr2, "descriptorSamplerMaxNum", shaderStage.updateAfterSet.descriptorSamplerMaxNum) << endl;
	LogInfo(LogGraphics) << format(formatStr2, "descriptorConstantBufferMaxNum", shaderStage.updateAfterSet.descriptorConstantBufferMaxNum) << endl;
	LogInfo(LogGraphics) << format(formatStr2, "descriptorStorageBufferMaxNum", shaderStage.updateAfterSet.descriptorStorageBufferMaxNum) << endl;
	LogInfo(LogGraphics) << format(formatStr2, "descriptorTextureMaxNum", shaderStage.updateAfterSet.descriptorTextureMaxNum) << endl;
	LogInfo(LogGraphics) << format(formatStr2, "descriptorStorageTextureMaxNum", shaderStage.updateAfterSet.descriptorStorageTextureMaxNum) << endl;
	LogInfo(LogGraphics) << format(formatStr2, "resourceMaxNum", shaderStage.updateAfterSet.resourceMaxNum) << endl;
	LogInfo(LogGraphics) << "    [Vertex]" << endl;
	LogInfo(LogGraphics) << format(formatStr2, "attributeMaxNum", shaderStage.vertex.attributeMaxNum) << endl;
	LogInfo(LogGraphics) << format(formatStr2, "streamMaxNum", shaderStage.vertex.streamMaxNum) << endl;
	LogInfo(LogGraphics) << format(formatStr2, "outputComponentMaxNum", shaderStage.vertex.outputComponentMaxNum) << endl;
	LogInfo(LogGraphics) << "    [Hull]" << endl;
	LogInfo(LogGraphics) << format(formatStr2, "patchPointMaxNum", shaderStage.hull.patchPointMaxNum) << endl;
	LogInfo(LogGraphics) << format(formatStr2, "perVertexInputComponentMaxNum", shaderStage.hull.perVertexInputComponentMaxNum) << endl;
	LogInfo(LogGraphics) << format(formatStr2, "perVertexOutputComponentMaxNum", shaderStage.hull.perVertexOutputComponentMaxNum) << endl;
	LogInfo(LogGraphics) << format(formatStr2, "perPatchOutputComponentMaxNum", shaderStage.hull.perPatchOutputComponentMaxNum) << endl;
	LogInfo(LogGraphics) << format(formatStr2, "totalOutputComponentMaxNum", shaderStage.hull.totalOutputComponentMaxNum) << endl;
	LogInfo(LogGraphics) << "    [Domain]" << endl;
	LogInfo(LogGraphics) << format(formatStr2, "inputComponentMaxNum", shaderStage.domain.inputComponentMaxNum) << endl;
	LogInfo(LogGraphics) << format(formatStr2, "outputComponentMaxNum", shaderStage.domain.outputComponentMaxNum) << endl;
	LogInfo(LogGraphics) << "    [Geometry]" << endl;
	LogInfo(LogGraphics) << format(formatStr2, "invocationMaxNum", shaderStage.geometry.invocationMaxNum) << endl;
	LogInfo(LogGraphics) << format(formatStr2, "inputComponentMaxNum", shaderStage.geometry.inputComponentMaxNum) << endl;
	LogInfo(LogGraphics) << format(formatStr2, "outputComponentMaxNum", shaderStage.geometry.outputComponentMaxNum) << endl;
	LogInfo(LogGraphics) << format(formatStr2, "outputVertexMaxNum", shaderStage.geometry.outputVertexMaxNum) << endl;
	LogInfo(LogGraphics) << format(formatStr2, "totalOutputComponentMaxNum", shaderStage.geometry.totalOutputComponentMaxNum) << endl;
	LogInfo(LogGraphics) << "    [Pixel]" << endl;
	LogInfo(LogGraphics) << format(formatStr2, "inputComponentMaxNum", shaderStage.pixel.inputComponentMaxNum) << endl;
	LogInfo(LogGraphics) << format(formatStr2, "attachmentMaxNum", shaderStage.pixel.attachmentMaxNum) << endl;
	LogInfo(LogGraphics) << format(formatStr2, "dualSourceAttachmentMaxNum", shaderStage.pixel.dualSourceAttachmentMaxNum) << endl;
	LogInfo(LogGraphics) << "    [Compute]" << endl;
	LogInfo(LogGraphics) << format("      {:48} [{}, {}, {}]", "workGroupMaxNum", shaderStage.compute.workGroupMaxNum[0], shaderStage.compute.workGroupMaxNum[1], shaderStage.compute.workGroupMaxNum[2]) << endl;
	LogInfo(LogGraphics) << format("      {:48} [{}, {}, {}]", "workGroupMaxDim", shaderStage.compute.workGroupMaxDim[0], shaderStage.compute.workGroupMaxDim[1], shaderStage.compute.workGroupMaxDim[2]) << endl;
	LogInfo(LogGraphics) << "      workGroupInvocationMaxNum: " << shaderStage.compute.workGroupInvocationMaxNum << endl;
	LogInfo(LogGraphics) << "      sharedMemoryMaxSize:       " << shaderStage.compute.sharedMemoryMaxSize << endl;
	LogInfo(LogGraphics) << format(formatStr2, "workGroupInvocationMaxNum", shaderStage.compute.workGroupInvocationMaxNum) << endl;
	LogInfo(LogGraphics) << format(formatStr2, "sharedMemoryMaxSize", shaderStage.compute.sharedMemoryMaxSize) << endl;
	LogInfo(LogGraphics) << "    [Ray Tracing]" << endl;
	LogInfo(LogGraphics) << format(formatStr2, "shaderGroupIdentifierSize", shaderStage.rayTracing.shaderGroupIdentifierSize) << endl;
	LogInfo(LogGraphics) << format(formatStr2, "tableMaxStride", shaderStage.rayTracing.tableMaxStride) << endl;
	LogInfo(LogGraphics) << format(formatStr2, "recursionMaxDepth", shaderStage.rayTracing.recursionMaxDepth) << endl;
	LogInfo(LogGraphics) << "    [Mesh Control]" << endl;
	LogInfo(LogGraphics) << format(formatStr2, "sharedMemoryMaxSize", shaderStage.meshControl.sharedMemoryMaxSize) << endl;
	LogInfo(LogGraphics) << format(formatStr2, "workGroupInvocationMaxNum", shaderStage.meshControl.workGroupInvocationMaxNum) << endl;
	LogInfo(LogGraphics) << format(formatStr2, "payloadMaxSize", shaderStage.meshControl.payloadMaxSize) << endl;
	LogInfo(LogGraphics) << "    [Mesh Evaluation]" << endl;
	LogInfo(LogGraphics) << format(formatStr2, "outputVerticesMaxNum", shaderStage.meshEvaluation.outputVerticesMaxNum) << endl;
	LogInfo(LogGraphics) << format(formatStr2, "outputPrimitiveMaxNum", shaderStage.meshEvaluation.outputPrimitiveMaxNum) << endl;
	LogInfo(LogGraphics) << format(formatStr2, "outputComponentMaxNum", shaderStage.meshEvaluation.outputComponentMaxNum) << endl;
	LogInfo(LogGraphics) << format(formatStr2, "sharedMemoryMaxSize", shaderStage.meshEvaluation.sharedMemoryMaxSize) << endl;
	LogInfo(LogGraphics) << format(formatStr2, "workGroupInvocationMaxNum", shaderStage.meshEvaluation.workGroupInvocationMaxNum) << endl;
	LogInfo(LogGraphics) << "  [Wave]" << endl;
	LogInfo(LogGraphics) << format(formatStr, "laneMinNum", wave.laneMinNum) << endl;
	LogInfo(LogGraphics) << format(formatStr, "laneMaxNum", wave.laneMaxNum) << endl;
	LogInfo(LogGraphics) << format(formatStr, "waveOpsStages", (uint32_t)wave.waveOpsStages) << endl; // TODO: Make a string like: Pixel | Compute
	LogInfo(LogGraphics) << format(formatStr, "quadOpsStages", (uint32_t)wave.quadOpsStages) << endl;
	LogInfo(LogGraphics) << format(formatStr, "derivativeOpsStages", (uint32_t)wave.derivativeOpsStages) << endl;
	LogInfo(LogGraphics) << "  [Other]" << endl;
	LogInfo(LogGraphics) << format(formatStr, "timestampFrequencyHz", other.timestampFrequencyHz) << endl;
	LogInfo(LogGraphics) << format(formatStr, "micromapSubdivisionMaxLevel", other.micromapSubdivisionMaxLevel) << endl;
	LogInfo(LogGraphics) << format(formatStr, "drawIndirectMaxNum", other.drawIndirectMaxNum) << endl;
	LogInfo(LogGraphics) << format(formatStr, "samplerLodBiasMax", other.samplerLodBiasMax) << endl;
	LogInfo(LogGraphics) << format(formatStr, "samplerAnisotropyMax", other.samplerAnisotropyMax) << endl;
	LogInfo(LogGraphics) << format(formatStr, "texelGatherOffsetMin", other.texelGatherOffsetMin) << endl;
	LogInfo(LogGraphics) << format(formatStr, "texelOffsetMin", other.texelOffsetMin) << endl;
	LogInfo(LogGraphics) << format(formatStr, "texelOffsetMax", other.texelOffsetMax) << endl;
	LogInfo(LogGraphics) << format(formatStr, "texelGatherOffsetMax", other.texelGatherOffsetMax) << endl;
	LogInfo(LogGraphics) << format(formatStr, "clipDistanceMaxNum", other.clipDistanceMaxNum) << endl;
	LogInfo(LogGraphics) << format(formatStr, "cullDistanceMaxNum", other.cullDistanceMaxNum) << endl;
	LogInfo(LogGraphics) << format(formatStr, "combinedClipAndCullDistanceMaxNum", other.combinedClipAndCullDistanceMaxNum) << endl;
	LogInfo(LogGraphics) << format(formatStr, "viewMaxNum", other.viewMaxNum) << endl;
	LogInfo(LogGraphics) << format(formatStr, "shadingRateAttachmentTileSize", other.shadingRateAttachmentTileSize) << endl;
	LogInfo(LogGraphics) << "  [Tiers]" << endl;
	LogInfo(LogGraphics) << format(formatStr, "conservativeRaster", tiers.conservativeRaster) << endl;
	LogInfo(LogGraphics) << format(formatStr, "sampleLocations", tiers.sampleLocations) << endl;
	LogInfo(LogGraphics) << format(formatStr, "rayTracing", tiers.rayTracing) << endl;
	LogInfo(LogGraphics) << format(formatStr, "shadingRate", tiers.shadingRate) << endl;
	LogInfo(LogGraphics) << format(formatStr, "resourceBinding", tiers.resourceBinding) << endl;
	LogInfo(LogGraphics) << format(formatStr, "bindless", tiers.bindless) << endl;
	LogInfo(LogGraphics) << format(formatStr, "memory", tiers.memory) << endl;
	LogInfo(LogGraphics) << format(formatStr, "workGraphs", tiers.workGraphs) << endl;
	LogInfo(LogGraphics) << format(formatStr, "executeIndirect", tiers.executeIndirect) << endl;
	LogInfo(LogGraphics) << "  [Features]" << endl;
	LogInfo(LogGraphics) << format(formatStr, "getMemoryDesc2", features.getMemoryDesc2) << endl;
	LogInfo(LogGraphics) << format(formatStr, "enhancedBarriers", features.enhancedBarriers) << endl;
	LogInfo(LogGraphics) << format(formatStr, "swapChain", features.swapChain) << endl;
	LogInfo(LogGraphics) << format(formatStr, "rayTracing", features.rayTracing) << endl;
	LogInfo(LogGraphics) << format(formatStr, "meshShader", features.meshShader) << endl;
	LogInfo(LogGraphics) << format(formatStr, "lowLatency", features.lowLatency) << endl;
	LogInfo(LogGraphics) << format(formatStr, "micromap", features.micromap) << endl;
	LogInfo(LogGraphics) << format(formatStr, "independentFrontAndBackStencilReferenceAndMasks", features.independentFrontAndBackStencilReferenceAndMasks) << endl;
	LogInfo(LogGraphics) << format(formatStr, "textureFilterMinMax", features.textureFilterMinMax) << endl;
	LogInfo(LogGraphics) << format(formatStr, "logicOp", features.logicOp) << endl;
	LogInfo(LogGraphics) << format(formatStr, "depthBoundsTest", features.depthBoundsTest) << endl;
	LogInfo(LogGraphics) << format(formatStr, "drawIndirectCount", features.drawIndirectCount) << endl;
	LogInfo(LogGraphics) << format(formatStr, "lineSmoothing", features.lineSmoothing) << endl;
	LogInfo(LogGraphics) << format(formatStr, "copyQueueTimestamp", features.copyQueueTimestamp) << endl;
	LogInfo(LogGraphics) << format(formatStr, "meshShaderPipelineStats", features.meshShaderPipelineStats) << endl;
	LogInfo(LogGraphics) << format(formatStr, "dynamicDepthBias", features.dynamicDepthBias) << endl;
	LogInfo(LogGraphics) << format(formatStr, "additionalShadingRates", features.additionalShadingRates) << endl;
	LogInfo(LogGraphics) << format(formatStr, "viewportOriginBottomLeft", features.viewportOriginBottomLeft) << endl;
	LogInfo(LogGraphics) << format(formatStr, "regionResolve", features.regionResolve) << endl;
	LogInfo(LogGraphics) << format(formatStr, "flexibleMultiview", features.flexibleMultiview) << endl;
	LogInfo(LogGraphics) << format(formatStr, "layerBasedMultiview", features.layerBasedMultiview) << endl;
	LogInfo(LogGraphics) << format(formatStr, "viewportBasedMultiview", features.viewportBasedMultiview) << endl;
	LogInfo(LogGraphics) << format(formatStr, "presentFromCompute", features.presentFromCompute) << endl;
	LogInfo(LogGraphics) << format(formatStr, "waitableSwapChain", features.waitableSwapChain) << endl;
	LogInfo(LogGraphics) << format(formatStr, "pipelineStatistics", features.pipelineStatistics) << endl;
	LogInfo(LogGraphics) << format(formatStr, "rasterizerDesc2", features.rasterizerDesc2) << endl;
	LogInfo(LogGraphics) << "  [Shader Features]" << endl;
	LogInfo(LogGraphics) << format(formatStr, "viewportIndex", shaderFeatures.viewportIndex) << endl;
	LogInfo(LogGraphics) << format(formatStr, "layerIndex", shaderFeatures.layerIndex) << endl;
	LogInfo(LogGraphics) << format(formatStr, "clock", shaderFeatures.clock) << endl;
	LogInfo(LogGraphics) << format(formatStr, "rasterizedOrderedView", shaderFeatures.rasterizedOrderedView) << endl;
	LogInfo(LogGraphics) << format(formatStr, "barycentric", shaderFeatures.barycentric) << endl;
	LogInfo(LogGraphics) << format(formatStr, "rayTracingPositionFetch", shaderFeatures.rayTracingPositionFetch) << endl;
	LogInfo(LogGraphics) << format(formatStr, "nativeI16", shaderFeatures.nativeI16) << endl;
	LogInfo(LogGraphics) << format(formatStr, "nativeF16", shaderFeatures.nativeF16) << endl;
	LogInfo(LogGraphics) << format(formatStr, "nativeI64", shaderFeatures.nativeI64) << endl;
	LogInfo(LogGraphics) << format(formatStr, "nativeF64", shaderFeatures.nativeF64) << endl;
	LogInfo(LogGraphics) << format(formatStr, "atomicsI16", shaderFeatures.atomicsI16) << endl;
	LogInfo(LogGraphics) << format(formatStr, "atomicsF16", shaderFeatures.atomicsF16) << endl;
	LogInfo(LogGraphics) << format(formatStr, "atomicsF32", shaderFeatures.atomicsF32) << endl;
	LogInfo(LogGraphics) << format(formatStr, "atomicsI64", shaderFeatures.atomicsI64) << endl;
	LogInfo(LogGraphics) << format(formatStr, "atomicsF64", shaderFeatures.atomicsF64) << endl;
	LogInfo(LogGraphics) << format(formatStr, "storageReadWithoutFormat", shaderFeatures.storageReadWithoutFormat) << endl;
	LogInfo(LogGraphics) << format(formatStr, "storageWriteWithoutFormat", shaderFeatures.storageWriteWithoutFormat) << endl;
	LogInfo(LogGraphics) << format(formatStr, "waveQuery", shaderFeatures.waveQuery) << endl;
	LogInfo(LogGraphics) << format(formatStr, "waveVote", shaderFeatures.waveVote) << endl;
	LogInfo(LogGraphics) << format(formatStr, "waveShuffle", shaderFeatures.waveShuffle) << endl;
	LogInfo(LogGraphics) << format(formatStr, "waveArithmetic", shaderFeatures.waveArithmetic) << endl;
	LogInfo(LogGraphics) << format(formatStr, "waveReduction", shaderFeatures.waveReduction) << endl;
	LogInfo(LogGraphics) << format(formatStr, "waveQuad", shaderFeatures.waveQuad) << endl;
}

} // namespace Luna