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

#include "DeviceCapsVK.h"

#include "Graphics\Enums.h"

using namespace std;


namespace Luna::VK
{

DeviceCaps::DeviceCaps()
{
	// Attach pNext pointers
	properties2.pNext = &properties11;
	properties11.pNext = &properties12;
	properties12.pNext = &properties13;
	properties13.pNext = nullptr;

	memoryProperties2.pNext = &memoryBudgetProperties;
	memoryBudgetProperties.pNext = nullptr;

	features2.pNext = &features11;
	features11.pNext = &features12;
	features12.pNext = &features13;
	features13.pNext = nullptr;
}


void DeviceCaps::ReadCaps(VkPhysicalDevice physicalDevice)
{
	if (!m_capsRead)
	{
		vkGetPhysicalDeviceProperties2(physicalDevice, &properties2);
		vkGetPhysicalDeviceMemoryProperties2(physicalDevice, &memoryProperties2);
		vkGetPhysicalDeviceFeatures2(physicalDevice, &features2);

		version = DecodeVulkanVersion(properties.apiVersion);

		m_capsRead = true;
	}
}


void DeviceCaps::LogCaps()
{
	LogInfo(LogVulkan) << "  Device Caps" << endl;

	constexpr const char* formatStr = "      {:48} {}";
	constexpr const char* formatStr2 = "        {:46} {}";
	constexpr const char* formatStr3 = "        {:46} {:#x}";
	constexpr const char* formatStr4 = "        {:54} {}";
	constexpr const char* formatStr5 = "        {:54} {:#x}";
	constexpr const char* formatStr6 = "        {:72} {}";
	constexpr const char* formatStr7 = "        {:72} {:#x}";

	// Vulkan 1.0 properties
	LogInfo(LogVulkan) << "    Vulkan 1.0 properties" << endl;
	LogInfo(LogVulkan) << format(formatStr, "apiVersion:", version) << endl;
	LogInfo(LogVulkan) << format("      {:48} {:#x}", "driverVersion:", properties.driverVersion) << endl;
	LogInfo(LogVulkan) << format("      {:48} {} ({:#x})", "vendorID:", VendorIdToString(properties.vendorID), properties.vendorID) << endl;
	LogInfo(LogVulkan) << format("      {:48} {:#x}", "deviceID:", properties.deviceID) << endl;
	LogInfo(LogVulkan) << format(formatStr, "deviceType:", properties.deviceType) << endl;

	// Vulkan 1.0 limits
	VkPhysicalDeviceLimits& limits = properties.limits;
	LogInfo(LogVulkan) << "    Vulkan 1.0 limits" << endl;
	LogInfo(LogVulkan) << format(formatStr2, "maxImageDimension1D:", limits.maxImageDimension1D) << endl;
	LogInfo(LogVulkan) << format(formatStr2, "maxImageDimension2D:", limits.maxImageDimension2D) << endl;
	LogInfo(LogVulkan) << format(formatStr2, "maxImageDimension3D:", limits.maxImageDimension3D) << endl;
	LogInfo(LogVulkan) << format(formatStr2, "maxImageDimensionCube:", limits.maxImageDimensionCube) << endl;
	LogInfo(LogVulkan) << format(formatStr2, "maxImageArrayLayers:", limits.maxImageArrayLayers) << endl;
	LogInfo(LogVulkan) << format(formatStr2, "maxTexelBufferElements:", limits.maxTexelBufferElements) << endl;
	LogInfo(LogVulkan) << format(formatStr2, "maxUniformBufferRange:", limits.maxUniformBufferRange) << endl;
	LogInfo(LogVulkan) << format(formatStr2, "maxStorageBufferRange:", limits.maxStorageBufferRange) << endl;
	LogInfo(LogVulkan) << format(formatStr2, "maxPushConstantsSize:", limits.maxPushConstantsSize) << endl;
	LogInfo(LogVulkan) << format(formatStr2, "maxMemoryAllocationCount:", limits.maxMemoryAllocationCount) << endl;
	LogInfo(LogVulkan) << format(formatStr2, "maxSamplerAllocationCount:", limits.maxSamplerAllocationCount) << endl;
	LogInfo(LogVulkan) << format(formatStr3, "bufferImageGranularity:", limits.bufferImageGranularity) << endl;
	LogInfo(LogVulkan) << format(formatStr3, "sparseAddressSpaceSize:", limits.sparseAddressSpaceSize) << endl;
	LogInfo(LogVulkan) << format(formatStr2, "maxBoundDescriptorSets:", limits.maxBoundDescriptorSets) << endl;
	LogInfo(LogVulkan) << format(formatStr2, "maxPerStageDescriptorSamplers:", limits.maxPerStageDescriptorSamplers) << endl;
	LogInfo(LogVulkan) << format(formatStr2, "maxPerStageDescriptorUniformBuffers:", limits.maxPerStageDescriptorUniformBuffers) << endl;
	LogInfo(LogVulkan) << format(formatStr2, "maxPerStageDescriptorStorageBuffers:", limits.maxPerStageDescriptorStorageBuffers) << endl;
	LogInfo(LogVulkan) << format(formatStr2, "maxPerStageDescriptorSampledImages:", limits.maxPerStageDescriptorSampledImages) << endl;
	LogInfo(LogVulkan) << format(formatStr2, "maxPerStageDescriptorStorageImages:", limits.maxPerStageDescriptorStorageImages) << endl;
	LogInfo(LogVulkan) << format(formatStr2, "maxPerStageDescriptorInputAttachments:", limits.maxPerStageDescriptorInputAttachments) << endl;
	LogInfo(LogVulkan) << format(formatStr2, "maxPerStageResources:", limits.maxPerStageResources) << endl;
	LogInfo(LogVulkan) << format(formatStr2, "maxDescriptorSetSamplers:", limits.maxDescriptorSetSamplers) << endl;
	LogInfo(LogVulkan) << format(formatStr2, "maxDescriptorSetUniformBuffers:", limits.maxDescriptorSetUniformBuffers) << endl;
	LogInfo(LogVulkan) << format(formatStr2, "maxDescriptorSetUniformBuffersDynamic:", limits.maxDescriptorSetUniformBuffersDynamic) << endl;
	LogInfo(LogVulkan) << format(formatStr2, "maxDescriptorSetStorageBuffers:", limits.maxDescriptorSetStorageBuffers) << endl;
	LogInfo(LogVulkan) << format(formatStr2, "maxDescriptorSetStorageBuffersDynamic:", limits.maxDescriptorSetStorageBuffersDynamic) << endl;
	LogInfo(LogVulkan) << format(formatStr2, "maxDescriptorSetSampledImages:", limits.maxDescriptorSetSampledImages) << endl;
	LogInfo(LogVulkan) << format(formatStr2, "maxDescriptorSetStorageImages:", limits.maxDescriptorSetStorageImages) << endl;
	LogInfo(LogVulkan) << format(formatStr2, "maxDescriptorSetInputAttachments:", limits.maxDescriptorSetInputAttachments) << endl;
	LogInfo(LogVulkan) << format(formatStr2, "maxVertexInputAttributes:", limits.maxVertexInputAttributes) << endl;
	LogInfo(LogVulkan) << format(formatStr2, "maxVertexInputBindings:", limits.maxVertexInputBindings) << endl;
	LogInfo(LogVulkan) << format(formatStr2, "maxVertexInputAttributeOffset:", limits.maxVertexInputAttributeOffset) << endl;
	LogInfo(LogVulkan) << format(formatStr2, "maxVertexInputBindingStride:", limits.maxVertexInputBindingStride) << endl;
	LogInfo(LogVulkan) << format(formatStr2, "maxVertexOutputComponents:", limits.maxVertexOutputComponents) << endl;
	LogInfo(LogVulkan) << format(formatStr2, "maxTessellationGenerationLevel:", limits.maxTessellationGenerationLevel) << endl;
	LogInfo(LogVulkan) << format(formatStr2, "maxTessellationPatchSize:", limits.maxTessellationPatchSize) << endl;
	LogInfo(LogVulkan) << format(formatStr2, "maxTessellationControlPerVertexInputComponents:", limits.maxTessellationControlPerVertexInputComponents) << endl;
	LogInfo(LogVulkan) << format(formatStr2, "maxTessellationControlPerVertexOutputComponents:", limits.maxTessellationControlPerVertexOutputComponents) << endl;
	LogInfo(LogVulkan) << format(formatStr2, "maxTessellationControlPerPatchOutputComponents:", limits.maxTessellationControlPerPatchOutputComponents) << endl;
	LogInfo(LogVulkan) << format(formatStr2, "maxTessellationControlTotalOutputComponents:", limits.maxTessellationControlTotalOutputComponents) << endl;
	LogInfo(LogVulkan) << format(formatStr2, "maxTessellationEvaluationInputComponents:", limits.maxTessellationEvaluationInputComponents) << endl;
	LogInfo(LogVulkan) << format(formatStr2, "maxTessellationEvaluationOutputComponents:", limits.maxTessellationEvaluationOutputComponents) << endl;
	LogInfo(LogVulkan) << format(formatStr2, "maxGeometryShaderInvocations:", limits.maxGeometryShaderInvocations) << endl;
	LogInfo(LogVulkan) << format(formatStr2, "maxGeometryInputComponents:", limits.maxGeometryInputComponents) << endl;
	LogInfo(LogVulkan) << format(formatStr2, "maxGeometryOutputComponents:", limits.maxGeometryOutputComponents) << endl;
	LogInfo(LogVulkan) << format(formatStr2, "maxGeometryOutputVertices:", limits.maxGeometryOutputVertices) << endl;
	LogInfo(LogVulkan) << format(formatStr2, "maxGeometryTotalOutputComponents:", limits.maxGeometryTotalOutputComponents) << endl;
	LogInfo(LogVulkan) << format(formatStr2, "maxFragmentInputComponents:", limits.maxFragmentInputComponents) << endl;
	LogInfo(LogVulkan) << format(formatStr2, "maxFragmentOutputAttachments:", limits.maxFragmentOutputAttachments) << endl;
	LogInfo(LogVulkan) << format(formatStr2, "maxFragmentDualSrcAttachments:", limits.maxFragmentDualSrcAttachments) << endl;
	LogInfo(LogVulkan) << format(formatStr2, "maxFragmentCombinedOutputResources:", limits.maxFragmentCombinedOutputResources) << endl;
	LogInfo(LogVulkan) << format("        {:46} [{}, {}, {}]", "maxComputeWorkGroupCount:",
		limits.maxComputeWorkGroupCount[0],
		limits.maxComputeWorkGroupCount[1],
		limits.maxComputeWorkGroupCount[2]) << endl;
	LogInfo(LogVulkan) << format(formatStr2, "maxComputeWorkGroupInvocations:", limits.maxComputeWorkGroupInvocations) << endl;
	LogInfo(LogVulkan) << format("        {:46} [{}, {}, {}]", "maxComputeWorkGroupSize:",
		limits.maxComputeWorkGroupSize[0],
		limits.maxComputeWorkGroupSize[1],
		limits.maxComputeWorkGroupSize[2]) << endl;
	LogInfo(LogVulkan) << format(formatStr2, "subPixelPrecisionBits:", limits.subPixelPrecisionBits) << endl;
	LogInfo(LogVulkan) << format(formatStr2, "subTexelPrecisionBits:", limits.subTexelPrecisionBits) << endl;
	LogInfo(LogVulkan) << format(formatStr2, "mipmapPrecisionBits:", limits.mipmapPrecisionBits) << endl;
	LogInfo(LogVulkan) << format(formatStr2, "maxDrawIndexedIndexValue:", limits.maxDrawIndexedIndexValue) << endl;
	LogInfo(LogVulkan) << format(formatStr2, "maxDrawIndirectCount:", limits.maxDrawIndirectCount) << endl;
	LogInfo(LogVulkan) << format(formatStr2, "maxSamplerLodBias:", limits.maxSamplerLodBias) << endl;
	LogInfo(LogVulkan) << format(formatStr2, "maxSamplerAnisotropy:", limits.maxSamplerAnisotropy) << endl;
	LogInfo(LogVulkan) << format(formatStr2, "maxViewports:", limits.maxViewports) << endl;
	LogInfo(LogVulkan) << format("        {:46} [{}, {}]", "maxViewportDimensions:",
		limits.maxViewportDimensions[0],
		limits.maxViewportDimensions[1]) << endl;
	LogInfo(LogVulkan) << format("        {:46} [{}, {}]", "viewportBoundsRange:",
		limits.viewportBoundsRange[0],
		limits.viewportBoundsRange[1]) << endl;
	LogInfo(LogVulkan) << format(formatStr2, "viewportSubPixelBits:", limits.viewportSubPixelBits) << endl;
	LogInfo(LogVulkan) << format(formatStr2, "minMemoryMapAlignment:", limits.minMemoryMapAlignment) << endl;
	LogInfo(LogVulkan) << format(formatStr3, "minTexelBufferOffsetAlignment:", limits.minTexelBufferOffsetAlignment) << endl;
	LogInfo(LogVulkan) << format(formatStr3, "minUniformBufferOffsetAlignment:", limits.minUniformBufferOffsetAlignment) << endl;
	LogInfo(LogVulkan) << format(formatStr3, "minStorageBufferOffsetAlignment:", limits.minStorageBufferOffsetAlignment) << endl;
	LogInfo(LogVulkan) << format(formatStr2, "subPixelInterpolationOffsetBits:", limits.subPixelInterpolationOffsetBits) << endl;
	LogInfo(LogVulkan) << format(formatStr2, "maxFramebufferWidth:", limits.maxFramebufferWidth) << endl;
	LogInfo(LogVulkan) << format(formatStr2, "maxFramebufferHeight:", limits.maxFramebufferHeight) << endl;
	LogInfo(LogVulkan) << format(formatStr2, "maxFramebufferLayers:", limits.maxFramebufferLayers) << endl;
	LogInfo(LogVulkan) << format(formatStr2, "framebufferColorSampleCounts:", VkSampleCountFlagsToString(limits.framebufferColorSampleCounts)) << endl;
	LogInfo(LogVulkan) << format(formatStr2, "framebufferDepthSampleCounts:", VkSampleCountFlagsToString(limits.framebufferDepthSampleCounts)) << endl;
	LogInfo(LogVulkan) << format(formatStr2, "framebufferStencilSampleCounts:", VkSampleCountFlagsToString(limits.framebufferStencilSampleCounts)) << endl;
	LogInfo(LogVulkan) << format(formatStr2, "framebufferNoAttachmentsSampleCounts:", VkSampleCountFlagsToString(limits.framebufferNoAttachmentsSampleCounts)) << endl;
	LogInfo(LogVulkan) << format(formatStr2, "maxColorAttachments:", limits.maxColorAttachments) << endl;
	LogInfo(LogVulkan) << format(formatStr2, "sampledImageColorSampleCounts:", VkSampleCountFlagsToString(limits.sampledImageColorSampleCounts)) << endl;
	LogInfo(LogVulkan) << format(formatStr2, "sampledImageIntegerSampleCounts:", VkSampleCountFlagsToString(limits.sampledImageIntegerSampleCounts)) << endl;
	LogInfo(LogVulkan) << format(formatStr2, "sampledImageDepthSampleCounts:", VkSampleCountFlagsToString(limits.sampledImageDepthSampleCounts)) << endl;
	LogInfo(LogVulkan) << format(formatStr2, "sampledImageStencilSampleCounts:", VkSampleCountFlagsToString(limits.sampledImageStencilSampleCounts)) << endl;
	LogInfo(LogVulkan) << format(formatStr2, "storageImageSampleCounts:", VkSampleCountFlagsToString(limits.storageImageSampleCounts)) << endl;
	LogInfo(LogVulkan) << format(formatStr2, "maxSampleMaskWords:", limits.maxSampleMaskWords) << endl;
	LogInfo(LogVulkan) << format(formatStr2, "timestampComputeAndGraphics:", (bool)limits.timestampComputeAndGraphics) << endl;
	LogInfo(LogVulkan) << format(formatStr2, "timestampPeriod:", limits.timestampPeriod) << endl;
	LogInfo(LogVulkan) << format(formatStr2, "maxClipDistances:", limits.maxClipDistances) << endl;
	LogInfo(LogVulkan) << format(formatStr2, "maxCullDistances:", limits.maxCullDistances) << endl;
	LogInfo(LogVulkan) << format(formatStr2, "maxCombinedClipAndCullDistances:", limits.maxCombinedClipAndCullDistances) << endl;
	LogInfo(LogVulkan) << format(formatStr2, "discreteQueuePriorities:", limits.discreteQueuePriorities) << endl;
	LogInfo(LogVulkan) << format("        {:46} [{}, {}]", "pointSizeRange:",
		limits.pointSizeRange[0],
		limits.pointSizeRange[1]) << endl;
	LogInfo(LogVulkan) << format("        {:46} [{}, {}]", "lineWidthRange:",
		limits.lineWidthRange[0],
		limits.lineWidthRange[1]) << endl;
	LogInfo(LogVulkan) << format(formatStr2, "pointSizeGranularity:", limits.pointSizeGranularity) << endl;
	LogInfo(LogVulkan) << format(formatStr2, "lineWidthGranularity:", limits.lineWidthGranularity) << endl;
	LogInfo(LogVulkan) << format(formatStr2, "strictLines:", (bool)limits.strictLines) << endl;
	LogInfo(LogVulkan) << format(formatStr2, "standardSampleLocations:", (bool)limits.standardSampleLocations) << endl;
	LogInfo(LogVulkan) << format(formatStr3, "optimalBufferCopyOffsetAlignment:", limits.optimalBufferCopyOffsetAlignment) << endl;
	LogInfo(LogVulkan) << format(formatStr3, "optimalBufferCopyRowPitchAlignment:", limits.optimalBufferCopyRowPitchAlignment) << endl;
	LogInfo(LogVulkan) << format(formatStr3, "nonCoherentAtomSize:", limits.nonCoherentAtomSize) << endl;
	LogInfo(LogVulkan) << endl;

	// Vulkan 1.1 properties
	LogInfo(LogVulkan) << "    Vulkan 1.1 properties" << endl;
	LogInfo(LogVulkan) << format(formatStr2, "deviceUUID:", AsUUID(properties11.deviceUUID)) << endl;
	LogInfo(LogVulkan) << format(formatStr2, "driverUUID:", AsUUID(properties11.driverUUID)) << endl;
	LogInfo(LogVulkan) << format(formatStr2, "deviceLUID:", AsLUID(properties11.deviceLUID)) << endl;
	LogInfo(LogVulkan) << format(formatStr2, "deviceNodeMask:", properties11.deviceNodeMask) << endl;
	LogInfo(LogVulkan) << format(formatStr2, "deviceLUIDValid:", (bool)properties11.deviceLUIDValid) << endl;
	LogInfo(LogVulkan) << format(formatStr2, "subgroupSize: ", properties11.subgroupSize) << endl;
	LogInfo(LogVulkan) << format(formatStr2, "subgroupSupportedStages:", VkShaderStageFlagsToString(properties11.subgroupSupportedStages)) << endl;
	LogInfo(LogVulkan) << format(formatStr2, "subgroupSupportedOperations:", VkSubgroupFeatureFlagsToString(properties11.subgroupSupportedOperations)) << endl;
	LogInfo(LogVulkan) << format(formatStr2, "subgroupQuadOperationsInAllStages:", (bool)properties11.subgroupQuadOperationsInAllStages) << endl;
	LogInfo(LogVulkan) << format(formatStr2, "pointClippingBehavior", properties11.pointClippingBehavior) << endl;
	LogInfo(LogVulkan) << format(formatStr2, "maxMultiviewViewCount:", properties11.maxMultiviewViewCount) << endl;
	LogInfo(LogVulkan) << format(formatStr2, "maxMultiviewInstanceIndex:", properties11.maxMultiviewInstanceIndex) << endl;
	LogInfo(LogVulkan) << format(formatStr2, "proptectedNoFault:", (bool)properties11.protectedNoFault) << endl;
	LogInfo(LogVulkan) << format(formatStr2, "maxPerSetDescriptors:", properties11.maxPerSetDescriptors) << endl;
	LogInfo(LogVulkan) << format(formatStr3, "maxMemoryAllocationSize:", properties11.maxMemoryAllocationSize) << endl;
	LogInfo(LogVulkan) << endl;

	// Vulkan 1.2 properties
	LogInfo(LogVulkan) << "    Vulkan 1.2 properties" << endl;
	LogInfo(LogVulkan) << format(formatStr4, "driverID:", properties12.driverID) << endl;
	LogInfo(LogVulkan) << format(formatStr4, "driverName:", properties12.driverName) << endl;
	LogInfo(LogVulkan) << format(formatStr4, "driverInfo:", properties12.driverInfo) << endl;
	LogInfo(LogVulkan) << format(formatStr4, "conformanceVersion:", properties12.conformanceVersion) << endl;
	LogInfo(LogVulkan) << format(formatStr4, "denormBehaviorIndependence:", properties12.denormBehaviorIndependence) << endl;
	LogInfo(LogVulkan) << format(formatStr4, "roundingModeIndependence:", properties12.roundingModeIndependence) << endl;
	LogInfo(LogVulkan) << format(formatStr4, "shaderSignedZeroInfNanPreserveFloat16:", (bool)properties12.shaderSignedZeroInfNanPreserveFloat16) << endl;
	LogInfo(LogVulkan) << format(formatStr4, "shaderSignedZeroInfNanPreserveFloat32:", (bool)properties12.shaderSignedZeroInfNanPreserveFloat32) << endl;
	LogInfo(LogVulkan) << format(formatStr4, "shaderSignedZeroInfNanPreserveFloat64:", (bool)properties12.shaderSignedZeroInfNanPreserveFloat64) << endl;
	LogInfo(LogVulkan) << format(formatStr4, "shaderDenormPreserveFloat16:", (bool)properties12.shaderDenormPreserveFloat16) << endl;
	LogInfo(LogVulkan) << format(formatStr4, "shaderDenormPreserveFloat32:", (bool)properties12.shaderDenormPreserveFloat32) << endl;
	LogInfo(LogVulkan) << format(formatStr4, "shaderDenormPreserveFloat64:", (bool)properties12.shaderDenormPreserveFloat64) << endl;
	LogInfo(LogVulkan) << format(formatStr4, "shaderDenormFlushToZeroFloat16:", (bool)properties12.shaderDenormFlushToZeroFloat16) << endl;
	LogInfo(LogVulkan) << format(formatStr4, "shaderDenormFlushToZeroFloat32:", (bool)properties12.shaderDenormFlushToZeroFloat32) << endl;
	LogInfo(LogVulkan) << format(formatStr4, "shaderDenormFlushToZeroFloat64:", (bool)properties12.shaderDenormFlushToZeroFloat64) << endl;
	LogInfo(LogVulkan) << format(formatStr4, "shaderRoundingModeRTEFloat16:", (bool)properties12.shaderRoundingModeRTEFloat16) << endl;
	LogInfo(LogVulkan) << format(formatStr4, "shaderRoundingModeRTEFloat32:", (bool)properties12.shaderRoundingModeRTEFloat32) << endl;
	LogInfo(LogVulkan) << format(formatStr4, "shaderRoundingModeRTEFloat64:", (bool)properties12.shaderRoundingModeRTEFloat64) << endl;
	LogInfo(LogVulkan) << format(formatStr4, "shaderRoundingModeRTZFloat16:", (bool)properties12.shaderRoundingModeRTZFloat16) << endl;
	LogInfo(LogVulkan) << format(formatStr4, "shaderRoundingModeRTZFloat32:", (bool)properties12.shaderRoundingModeRTZFloat32) << endl;
	LogInfo(LogVulkan) << format(formatStr4, "shaderRoundingModeRTZFloat64:", (bool)properties12.shaderRoundingModeRTZFloat64) << endl;
	LogInfo(LogVulkan) << format(formatStr4, "maxUpdateAfterBindDescriptorsInAllPools:", properties12.maxUpdateAfterBindDescriptorsInAllPools) << endl;
	LogInfo(LogVulkan) << format(formatStr4, "shaderUniformBufferArrayNonUniformIndexingNative:", (bool)properties12.shaderUniformBufferArrayNonUniformIndexingNative) << endl;
	LogInfo(LogVulkan) << format(formatStr4, "shaderSampledImageArrayNonUniformIndexingNative:", (bool)properties12.shaderSampledImageArrayNonUniformIndexingNative) << endl;
	LogInfo(LogVulkan) << format(formatStr4, "shaderStorageBufferArrayNonUniformIndexingNative:", (bool)properties12.shaderStorageBufferArrayNonUniformIndexingNative) << endl;
	LogInfo(LogVulkan) << format(formatStr4, "shaderStorageImageArrayNonUniformIndexingNative:", (bool)properties12.shaderStorageImageArrayNonUniformIndexingNative) << endl;
	LogInfo(LogVulkan) << format(formatStr4, "shaderInputAttachmentArrayNonUniformIndexingNative:", (bool)properties12.shaderInputAttachmentArrayNonUniformIndexingNative) << endl;
	LogInfo(LogVulkan) << format(formatStr4, "robustBufferAccessUpdateAfterBind:", (bool)properties12.robustBufferAccessUpdateAfterBind) << endl;
	LogInfo(LogVulkan) << format(formatStr4, "quadDivergentImplicitLod:", (bool)properties12.quadDivergentImplicitLod) << endl;
	LogInfo(LogVulkan) << format(formatStr4, "maxPerStageDescriptorUpdateAfterBindSamplers:", properties12.maxPerStageDescriptorUpdateAfterBindSamplers) << endl;
	LogInfo(LogVulkan) << format(formatStr4, "maxPerStageDescriptorUpdateAfterBindUniformBuffers:", properties12.maxPerStageDescriptorUpdateAfterBindUniformBuffers) << endl;
	LogInfo(LogVulkan) << format(formatStr4, "maxPerStageDescriptorUpdateAfterBindStorageBuffers:", properties12.maxPerStageDescriptorUpdateAfterBindStorageBuffers) << endl;
	LogInfo(LogVulkan) << format(formatStr4, "maxPerStageDescriptorUpdateAfterBindSampledImages:", properties12.maxPerStageDescriptorUpdateAfterBindSampledImages) << endl;
	LogInfo(LogVulkan) << format(formatStr4, "maxPerStageDescriptorUpdateAfterBindStorageImages:", properties12.maxPerStageDescriptorUpdateAfterBindStorageImages) << endl;
	LogInfo(LogVulkan) << format(formatStr4, "maxPerStageDescriptorUpdateAfterBindInputAttachments:", properties12.maxPerStageDescriptorUpdateAfterBindInputAttachments) << endl;
	LogInfo(LogVulkan) << format(formatStr4, "maxPerStageUpdateAfterBindResources:", properties12.maxPerStageUpdateAfterBindResources) << endl;
	LogInfo(LogVulkan) << format(formatStr4, "maxDescriptorSetUpdateAfterBindSamplers:", properties12.maxDescriptorSetUpdateAfterBindSamplers) << endl;
	LogInfo(LogVulkan) << format(formatStr4, "maxDescriptorSetUpdateAfterBindUniformBuffers:", properties12.maxDescriptorSetUpdateAfterBindUniformBuffers) << endl;
	LogInfo(LogVulkan) << format(formatStr4, "maxDescriptorSetUpdateAfterBindUniformBuffersDynamic:", properties12.maxDescriptorSetUpdateAfterBindUniformBuffersDynamic) << endl;
	LogInfo(LogVulkan) << format(formatStr4, "maxDescriptorSetUpdateAfterBindStorageBuffers:", properties12.maxDescriptorSetUpdateAfterBindStorageBuffers) << endl;
	LogInfo(LogVulkan) << format(formatStr4, "maxDescriptorSetUpdateAfterBindStorageBuffersDynamic:", properties12.maxDescriptorSetUpdateAfterBindStorageBuffersDynamic) << endl;
	LogInfo(LogVulkan) << format(formatStr4, "maxDescriptorSetUpdateAfterBindSampledImages:", properties12.maxDescriptorSetUpdateAfterBindSampledImages) << endl;
	LogInfo(LogVulkan) << format(formatStr4, "maxDescriptorSetUpdateAfterBindStorageImages:", properties12.maxDescriptorSetUpdateAfterBindStorageImages) << endl;
	LogInfo(LogVulkan) << format(formatStr4, "maxDescriptorSetUpdateAfterBindInputAttachments:", properties12.maxDescriptorSetUpdateAfterBindInputAttachments) << endl;
	LogInfo(LogVulkan) << format(formatStr4, "supportedDepthResolveModes:", VkResolveModeFlagsToString(properties12.supportedDepthResolveModes)) << endl;
	LogInfo(LogVulkan) << format(formatStr4, "supportedStencilResolveModes:", VkResolveModeFlagsToString(properties12.supportedStencilResolveModes)) << endl;
	LogInfo(LogVulkan) << format(formatStr4, "independentResolveNone:", (bool)properties12.independentResolveNone) << endl;
	LogInfo(LogVulkan) << format(formatStr4, "independentResolve:", (bool)properties12.independentResolve) << endl;
	LogInfo(LogVulkan) << format(formatStr4, "filterMinmaxSingleComponentFormats:", (bool)properties12.filterMinmaxSingleComponentFormats) << endl;
	LogInfo(LogVulkan) << format(formatStr4, "filterMinmaxImageComponentMapping:", (bool)properties12.filterMinmaxImageComponentMapping) << endl;
	LogInfo(LogVulkan) << format(formatStr5, "maxTimelineSemaphoreValueDifference:", properties12.maxTimelineSemaphoreValueDifference) << endl;
	LogInfo(LogVulkan) << format(formatStr4, "framebufferIntegerColorSampleCounts:", VkSampleCountFlagsToString(properties12.framebufferIntegerColorSampleCounts)) << endl;
	LogInfo(LogVulkan) << endl;

	// Vulkan 1.3 properties
	LogInfo(LogVulkan) << "    Vulkan 1.3 properties" << endl;
	LogInfo(LogVulkan) << format(formatStr6, "minSubgroupSize:", properties13.minSubgroupSize) << endl;
	LogInfo(LogVulkan) << format(formatStr6, "maxSubgroupSize:", properties13.maxSubgroupSize) << endl;
	LogInfo(LogVulkan) << format(formatStr6, "maxComputeWorkgroupSubgroups:", properties13.maxComputeWorkgroupSubgroups) << endl;
	LogInfo(LogVulkan) << format(formatStr6, "requiredSubgroupSizeStages:", VkShaderStageFlagsToString(properties13.requiredSubgroupSizeStages)) << endl;
	LogInfo(LogVulkan) << format(formatStr6, "maxInlineUniformBlockSize:", properties13.maxInlineUniformBlockSize) << endl;
	LogInfo(LogVulkan) << format(formatStr6, "maxPerStageDescriptorInlineUniformBlocks:", properties13.maxPerStageDescriptorInlineUniformBlocks) << endl;
	LogInfo(LogVulkan) << format(formatStr6, "maxPerStageDescriptorUpdateAfterBindInlineUniformBlocks:", properties13.maxPerStageDescriptorUpdateAfterBindInlineUniformBlocks) << endl;
	LogInfo(LogVulkan) << format(formatStr6, "maxDescriptorSetInlineUniformBlocks:", properties13.maxDescriptorSetInlineUniformBlocks) << endl;
	LogInfo(LogVulkan) << format(formatStr6, "maxDescriptorSetUpdateAfterBindInlineUniformBlocks:", properties13.maxDescriptorSetUpdateAfterBindInlineUniformBlocks) << endl;
	LogInfo(LogVulkan) << format(formatStr6, "maxInlineUniformTotalSize:", properties13.maxInlineUniformTotalSize) << endl;
	LogInfo(LogVulkan) << format(formatStr6, "integerDotProduct8BitUnsignedAccelerated:", (bool)properties13.integerDotProduct8BitUnsignedAccelerated) << endl;
	LogInfo(LogVulkan) << format(formatStr6, "integerDotProduct8BitSignedAccelerated:", (bool)properties13.integerDotProduct8BitSignedAccelerated) << endl;
	LogInfo(LogVulkan) << format(formatStr6, "integerDotProduct8BitMixedSignednessAccelerated:", (bool)properties13.integerDotProduct8BitMixedSignednessAccelerated) << endl;
	LogInfo(LogVulkan) << format(formatStr6, "integerDotProduct4x8BitPackedUnsignedAccelerated:", (bool)properties13.integerDotProduct4x8BitPackedUnsignedAccelerated) << endl;
	LogInfo(LogVulkan) << format(formatStr6, "integerDotProduct4x8BitPackedSignedAccelerated:", (bool)properties13.integerDotProduct4x8BitPackedSignedAccelerated) << endl;
	LogInfo(LogVulkan) << format(formatStr6, "integerDotProduct4x8BitPackedMixedSignednessAccelerated:", (bool)properties13.integerDotProduct4x8BitPackedMixedSignednessAccelerated) << endl;
	LogInfo(LogVulkan) << format(formatStr6, "integerDotProduct16BitUnsignedAccelerated:", (bool)properties13.integerDotProduct16BitUnsignedAccelerated) << endl;
	LogInfo(LogVulkan) << format(formatStr6, "integerDotProduct16BitSignedAccelerated:", (bool)properties13.integerDotProduct16BitSignedAccelerated) << endl;
	LogInfo(LogVulkan) << format(formatStr6, "integerDotProduct16BitMixedSignednessAccelerated:", (bool)properties13.integerDotProduct16BitMixedSignednessAccelerated) << endl;
	LogInfo(LogVulkan) << format(formatStr6, "integerDotProduct32BitUnsignedAccelerated:", (bool)properties13.integerDotProduct32BitUnsignedAccelerated) << endl;
	LogInfo(LogVulkan) << format(formatStr6, "integerDotProduct32BitSignedAccelerated:", (bool)properties13.integerDotProduct32BitSignedAccelerated) << endl;
	LogInfo(LogVulkan) << format(formatStr6, "integerDotProduct32BitMixedSignednessAccelerated:", (bool)properties13.integerDotProduct32BitMixedSignednessAccelerated) << endl;
	LogInfo(LogVulkan) << format(formatStr6, "integerDotProduct64BitUnsignedAccelerated:", (bool)properties13.integerDotProduct64BitUnsignedAccelerated) << endl;
	LogInfo(LogVulkan) << format(formatStr6, "integerDotProduct64BitSignedAccelerated:", (bool)properties13.integerDotProduct64BitSignedAccelerated) << endl;
	LogInfo(LogVulkan) << format(formatStr6, "integerDotProduct64BitMixedSignednessAccelerated:", (bool)properties13.integerDotProduct64BitMixedSignednessAccelerated) << endl;
	LogInfo(LogVulkan) << format(formatStr6, "integerDotProductAccumulatingSaturating8BitUnsignedAccelerated:", (bool)properties13.integerDotProductAccumulatingSaturating8BitUnsignedAccelerated) << endl;
	LogInfo(LogVulkan) << format(formatStr6, "integerDotProductAccumulatingSaturating8BitSignedAccelerated:", (bool)properties13.integerDotProductAccumulatingSaturating8BitSignedAccelerated) << endl;
	LogInfo(LogVulkan) << format(formatStr6, "integerDotProductAccumulatingSaturating8BitMixedSignednessAccelerated:", (bool)properties13.integerDotProductAccumulatingSaturating8BitMixedSignednessAccelerated) << endl;
	LogInfo(LogVulkan) << format(formatStr6, "integerDotProductAccumulatingSaturating4x8BitPackedUnsignedAccelerated:", (bool)properties13.integerDotProductAccumulatingSaturating4x8BitPackedUnsignedAccelerated) << endl;
	LogInfo(LogVulkan) << format(formatStr6, "integerDotProductAccumulatingSaturating4x8BitPackedSignedAccelerated:", (bool)properties13.integerDotProductAccumulatingSaturating4x8BitPackedSignedAccelerated) << endl;
	LogInfo(LogVulkan) << format(formatStr6, "integerDotProductAccumulatingSaturating4x8BitPackedMixedSignednessAccelerated:", (bool)properties13.integerDotProductAccumulatingSaturating4x8BitPackedMixedSignednessAccelerated) << endl;
	LogInfo(LogVulkan) << format(formatStr6, "integerDotProductAccumulatingSaturating16BitUnsignedAccelerated:", (bool)properties13.integerDotProductAccumulatingSaturating16BitUnsignedAccelerated) << endl;
	LogInfo(LogVulkan) << format(formatStr6, "integerDotProductAccumulatingSaturating16BitSignedAccelerated:", (bool)properties13.integerDotProductAccumulatingSaturating16BitSignedAccelerated) << endl;
	LogInfo(LogVulkan) << format(formatStr6, "integerDotProductAccumulatingSaturating16BitMixedSignednessAccelerated:", (bool)properties13.integerDotProductAccumulatingSaturating16BitMixedSignednessAccelerated) << endl;
	LogInfo(LogVulkan) << format(formatStr6, "integerDotProductAccumulatingSaturating32BitUnsignedAccelerated:", (bool)properties13.integerDotProductAccumulatingSaturating32BitUnsignedAccelerated) << endl;
	LogInfo(LogVulkan) << format(formatStr6, "integerDotProductAccumulatingSaturating32BitSignedAccelerated:", (bool)properties13.integerDotProductAccumulatingSaturating32BitSignedAccelerated) << endl;
	LogInfo(LogVulkan) << format(formatStr6, "integerDotProductAccumulatingSaturating32BitMixedSignednessAccelerated:", (bool)properties13.integerDotProductAccumulatingSaturating32BitMixedSignednessAccelerated) << endl;
	LogInfo(LogVulkan) << format(formatStr6, "integerDotProductAccumulatingSaturating64BitUnsignedAccelerated:", (bool)properties13.integerDotProductAccumulatingSaturating64BitUnsignedAccelerated) << endl;
	LogInfo(LogVulkan) << format(formatStr6, "integerDotProductAccumulatingSaturating64BitSignedAccelerated:", (bool)properties13.integerDotProductAccumulatingSaturating64BitSignedAccelerated) << endl;
	LogInfo(LogVulkan) << format(formatStr6, "integerDotProductAccumulatingSaturating64BitMixedSignednessAccelerated:", (bool)properties13.integerDotProductAccumulatingSaturating64BitMixedSignednessAccelerated) << endl;
	LogInfo(LogVulkan) << format(formatStr7, "storageTexelBufferOffsetAlignmentBytes:", properties13.storageTexelBufferOffsetAlignmentBytes) << endl;
	LogInfo(LogVulkan) << format(formatStr6, "storageTexelBufferOffsetSingleTexelAlignment:", (bool)properties13.storageTexelBufferOffsetSingleTexelAlignment) << endl;
	LogInfo(LogVulkan) << format(formatStr7, "uniformTexelBufferOffsetAlignmentBytes:", properties13.uniformTexelBufferOffsetAlignmentBytes) << endl;
	LogInfo(LogVulkan) << format(formatStr6, "uniformTexelBufferOffsetSingleTexelAlignment:", (bool)properties13.uniformTexelBufferOffsetSingleTexelAlignment) << endl;
	LogInfo(LogVulkan) << format(formatStr7, "maxBufferSize:", properties13.maxBufferSize) << endl;
	LogInfo(LogVulkan) << endl;

	// Vulkan 1.0 features
	LogInfo(LogVulkan) << "    Vulkan 1.0 features" << endl;
	LogInfo(LogVulkan) << format(formatStr2, "robustBufferAccess:", (bool)features.robustBufferAccess) << endl;
	LogInfo(LogVulkan) << format(formatStr2, "fullDrawIndexUint32:", (bool)features.fullDrawIndexUint32) << endl;
	LogInfo(LogVulkan) << format(formatStr2, "imageCubeArray:", (bool)features.imageCubeArray) << endl;
	LogInfo(LogVulkan) << format(formatStr2, "independentBlend:", (bool)features.independentBlend) << endl;
	LogInfo(LogVulkan) << format(formatStr2, "geometryShader:", (bool)features.geometryShader) << endl;
	LogInfo(LogVulkan) << format(formatStr2, "tessellationShader:", (bool)features.tessellationShader) << endl;
	LogInfo(LogVulkan) << format(formatStr2, "sampleRateShading:", (bool)features.sampleRateShading) << endl;
	LogInfo(LogVulkan) << format(formatStr2, "dualSrcBlend:", (bool)features.dualSrcBlend) << endl;
	LogInfo(LogVulkan) << format(formatStr2, "logicOp:", (bool)features.logicOp) << endl;
	LogInfo(LogVulkan) << format(formatStr2, "multiDrawIndirect:", (bool)features.multiDrawIndirect) << endl;
	LogInfo(LogVulkan) << format(formatStr2, "drawIndirectFirstInstance:", (bool)features.drawIndirectFirstInstance) << endl;
	LogInfo(LogVulkan) << format(formatStr2, "depthClamp:", (bool)features.depthClamp) << endl;
	LogInfo(LogVulkan) << format(formatStr2, "depthBiasClamp:", (bool)features.depthBiasClamp) << endl;
	LogInfo(LogVulkan) << format(formatStr2, "fillModeNonSolid:", (bool)features.fillModeNonSolid) << endl;
	LogInfo(LogVulkan) << format(formatStr2, "depthBounds:", (bool)features.depthBounds) << endl;
	LogInfo(LogVulkan) << format(formatStr2, "wideLines:", (bool)features.wideLines) << endl;
	LogInfo(LogVulkan) << format(formatStr2, "largePoints:", (bool)features.largePoints) << endl;
	LogInfo(LogVulkan) << format(formatStr2, "alphaToOne:", (bool)features.alphaToOne) << endl;
	LogInfo(LogVulkan) << format(formatStr2, "multiViewport:", (bool)features.multiViewport) << endl;
	LogInfo(LogVulkan) << format(formatStr2, "samplerAnisotropy:", (bool)features.samplerAnisotropy) << endl;
	LogInfo(LogVulkan) << format(formatStr2, "textureCompressionETC2:", (bool)features.textureCompressionETC2) << endl;
	LogInfo(LogVulkan) << format(formatStr2, "textureCompressionASTC_LDR:", (bool)features.textureCompressionASTC_LDR) << endl;
	LogInfo(LogVulkan) << format(formatStr2, "textureCompressionBC:", (bool)features.textureCompressionBC) << endl;
	LogInfo(LogVulkan) << format(formatStr2, "occlusionQueryPrecise:", (bool)features.occlusionQueryPrecise) << endl;
	LogInfo(LogVulkan) << format(formatStr2, "pipelineStatisticsQuery:", (bool)features.pipelineStatisticsQuery) << endl;
	LogInfo(LogVulkan) << format(formatStr2, "vertexPipelineStoresAndAtomics:", (bool)features.vertexPipelineStoresAndAtomics) << endl;
	LogInfo(LogVulkan) << format(formatStr2, "fragmentStoresAndAtomics:", (bool)features.fragmentStoresAndAtomics) << endl;
	LogInfo(LogVulkan) << format(formatStr2, "shaderTessellationAndGeometryPointSize:", (bool)features.shaderTessellationAndGeometryPointSize) << endl;
	LogInfo(LogVulkan) << format(formatStr2, "shaderImageGatherExtended:", (bool)features.shaderImageGatherExtended) << endl;
	LogInfo(LogVulkan) << format(formatStr2, "shaderStorageImageExtendedFormats:", (bool)features.shaderStorageImageExtendedFormats) << endl;
	LogInfo(LogVulkan) << format(formatStr2, "shaderStorageImageMultisample:", (bool)features.shaderStorageImageMultisample) << endl;
	LogInfo(LogVulkan) << format(formatStr2, "shaderStorageImageReadWithoutFormat:", (bool)features.shaderStorageImageReadWithoutFormat) << endl;
	LogInfo(LogVulkan) << format(formatStr2, "shaderStorageImageWriteWithoutFormat:", (bool)features.shaderStorageImageWriteWithoutFormat) << endl;
	LogInfo(LogVulkan) << format(formatStr2, "shaderUniformBufferArrayDynamicIndexing:", (bool)features.shaderUniformBufferArrayDynamicIndexing) << endl;
	LogInfo(LogVulkan) << format(formatStr2, "shaderSampledImageArrayDynamicIndexing:", (bool)features.shaderSampledImageArrayDynamicIndexing) << endl;
	LogInfo(LogVulkan) << format(formatStr2, "shaderStorageBufferArrayDynamicIndexing:", (bool)features.shaderStorageBufferArrayDynamicIndexing) << endl;
	LogInfo(LogVulkan) << format(formatStr2, "shaderStorageImageArrayDynamicIndexing:", (bool)features.shaderStorageImageArrayDynamicIndexing) << endl;
	LogInfo(LogVulkan) << format(formatStr2, "shaderClipDistance:", (bool)features.shaderClipDistance) << endl;
	LogInfo(LogVulkan) << format(formatStr2, "shaderCullDistance:", (bool)features.shaderCullDistance) << endl;
	LogInfo(LogVulkan) << format(formatStr2, "shaderFloat64:", (bool)features.shaderFloat64) << endl;
	LogInfo(LogVulkan) << format(formatStr2, "shaderInt64:", (bool)features.shaderInt64) << endl;
	LogInfo(LogVulkan) << format(formatStr2, "shaderInt16:", (bool)features.shaderInt16) << endl;
	LogInfo(LogVulkan) << format(formatStr2, "shaderResourceResidency:", (bool)features.shaderResourceResidency) << endl;
	LogInfo(LogVulkan) << format(formatStr2, "shaderResourceMinLod:", (bool)features.shaderResourceMinLod) << endl;
	LogInfo(LogVulkan) << format(formatStr2, "sparseBinding:", (bool)features.sparseBinding) << endl;
	LogInfo(LogVulkan) << format(formatStr2, "sparseResidencyBuffer:", (bool)features.sparseResidencyBuffer) << endl;
	LogInfo(LogVulkan) << format(formatStr2, "sparseResidencyImage2D:", (bool)features.sparseResidencyImage2D) << endl;
	LogInfo(LogVulkan) << format(formatStr2, "sparseResidencyImage3D:", (bool)features.sparseResidencyImage3D) << endl;
	LogInfo(LogVulkan) << format(formatStr2, "sparseResidency2Samples:", (bool)features.sparseResidency2Samples) << endl;
	LogInfo(LogVulkan) << format(formatStr2, "sparseResidency4Samples:", (bool)features.sparseResidency4Samples) << endl;
	LogInfo(LogVulkan) << format(formatStr2, "sparseResidency8Samples:", (bool)features.sparseResidency8Samples) << endl;
	LogInfo(LogVulkan) << format(formatStr2, "sparseResidency16Samples:", (bool)features.sparseResidency16Samples) << endl;
	LogInfo(LogVulkan) << format(formatStr2, "sparseResidencyAliased:", (bool)features.sparseResidencyAliased) << endl;
	LogInfo(LogVulkan) << format(formatStr2, "variableMultisampleRate:", (bool)features.variableMultisampleRate) << endl;
	LogInfo(LogVulkan) << format(formatStr2, "inheritedQueries:", (bool)features.inheritedQueries) << endl;
	LogInfo(LogVulkan) << endl;

	// Vulkan 1.1 features
	LogInfo(LogVulkan) << "    Vulkan 1.1 features" << endl;
	LogInfo(LogVulkan) << format(formatStr2, "storageBuffer16BitAccess:", (bool)features11.storageBuffer16BitAccess) << endl;
	LogInfo(LogVulkan) << format(formatStr2, "uniformAndStorageBuffer16BitAccess:", (bool)features11.uniformAndStorageBuffer16BitAccess) << endl;
	LogInfo(LogVulkan) << format(formatStr2, "storagePushConstant16:", (bool)features11.storagePushConstant16) << endl;
	LogInfo(LogVulkan) << format(formatStr2, "storageInputOutput16:", (bool)features11.storageInputOutput16) << endl;
	LogInfo(LogVulkan) << format(formatStr2, "multiview:", (bool)features11.multiview) << endl;
	LogInfo(LogVulkan) << format(formatStr2, "multiviewGeometryShader:", (bool)features11.multiviewGeometryShader) << endl;
	LogInfo(LogVulkan) << format(formatStr2, "multiviewTessellationShader:", (bool)features11.multiviewTessellationShader) << endl;
	LogInfo(LogVulkan) << format(formatStr2, "variablePointersStorageBuffer:", (bool)features11.variablePointersStorageBuffer) << endl;
	LogInfo(LogVulkan) << format(formatStr2, "variablePointers:", (bool)features11.variablePointers) << endl;
	LogInfo(LogVulkan) << format(formatStr2, "protectedMemory:", (bool)features11.protectedMemory) << endl;
	LogInfo(LogVulkan) << format(formatStr2, "samplerYcbcrConversion:", (bool)features11.samplerYcbcrConversion) << endl;
	LogInfo(LogVulkan) << format(formatStr2, "shaderDrawParameters:", (bool)features11.shaderDrawParameters) << endl;
	LogInfo(LogVulkan) << endl;

	// Vulkan 1.2 features
	LogInfo(LogVulkan) << "    Vulkan 1.2 features" << endl;
	LogInfo(LogVulkan) << format(formatStr4, "samplerMirrorClampToEdge:", (bool)features12.samplerMirrorClampToEdge) << endl;
	LogInfo(LogVulkan) << format(formatStr4, "drawIndirectCount:", (bool)features12.drawIndirectCount) << endl;
	LogInfo(LogVulkan) << format(formatStr4, "storageBuffer8BitAccess:", (bool)features12.storageBuffer8BitAccess) << endl;
	LogInfo(LogVulkan) << format(formatStr4, "uniformAndStorageBuffer8BitAccess:", (bool)features12.uniformAndStorageBuffer8BitAccess) << endl;
	LogInfo(LogVulkan) << format(formatStr4, "storagePushConstant8:", (bool)features12.storagePushConstant8) << endl;
	LogInfo(LogVulkan) << format(formatStr4, "shaderBufferInt64Atomics:", (bool)features12.shaderBufferInt64Atomics) << endl;
	LogInfo(LogVulkan) << format(formatStr4, "shaderSharedInt64Atomics:", (bool)features12.shaderSharedInt64Atomics) << endl;
	LogInfo(LogVulkan) << format(formatStr4, "shaderFloat16:", (bool)features12.shaderFloat16) << endl;
	LogInfo(LogVulkan) << format(formatStr4, "shaderInt8:", (bool)features12.shaderInt8) << endl;
	LogInfo(LogVulkan) << format(formatStr4, "descriptorIndexing:", (bool)features12.descriptorIndexing) << endl;
	LogInfo(LogVulkan) << format(formatStr4, "shaderInputAttachmentArrayDynamicIndexing:", (bool)features12.shaderInputAttachmentArrayDynamicIndexing) << endl;
	LogInfo(LogVulkan) << format(formatStr4, "shaderUniformTexelBufferArrayDynamicIndexing:", (bool)features12.shaderUniformTexelBufferArrayDynamicIndexing) << endl;
	LogInfo(LogVulkan) << format(formatStr4, "shaderStorageTexelBufferArrayDynamicIndexing:", (bool)features12.shaderStorageTexelBufferArrayDynamicIndexing) << endl;
	LogInfo(LogVulkan) << format(formatStr4, "shaderUniformBufferArrayNonUniformIndexing:", (bool)features12.shaderUniformBufferArrayNonUniformIndexing) << endl;
	LogInfo(LogVulkan) << format(formatStr4, "shaderSampledImageArrayNonUniformIndexing:", (bool)features12.shaderSampledImageArrayNonUniformIndexing) << endl;
	LogInfo(LogVulkan) << format(formatStr4, "shaderStorageBufferArrayNonUniformIndexing:", (bool)features12.shaderStorageBufferArrayNonUniformIndexing) << endl;
	LogInfo(LogVulkan) << format(formatStr4, "shaderStorageImageArrayNonUniformIndexing:", (bool)features12.shaderStorageImageArrayNonUniformIndexing) << endl;
	LogInfo(LogVulkan) << format(formatStr4, "shaderInputAttachmentArrayNonUniformIndexing:", (bool)features12.shaderInputAttachmentArrayNonUniformIndexing) << endl;
	LogInfo(LogVulkan) << format(formatStr4, "shaderUniformTexelBufferArrayNonUniformIndexing:", (bool)features12.shaderUniformTexelBufferArrayNonUniformIndexing) << endl;
	LogInfo(LogVulkan) << format(formatStr4, "shaderStorageTexelBufferArrayNonUniformIndexing:", (bool)features12.shaderStorageTexelBufferArrayNonUniformIndexing) << endl;
	LogInfo(LogVulkan) << format(formatStr4, "descriptorBindingUniformBufferUpdateAfterBind:", (bool)features12.descriptorBindingUniformBufferUpdateAfterBind) << endl;
	LogInfo(LogVulkan) << format(formatStr4, "descriptorBindingSampledImageUpdateAfterBind:", (bool)features12.descriptorBindingSampledImageUpdateAfterBind) << endl;
	LogInfo(LogVulkan) << format(formatStr4, "descriptorBindingStorageImageUpdateAfterBind:", (bool)features12.descriptorBindingStorageImageUpdateAfterBind) << endl;
	LogInfo(LogVulkan) << format(formatStr4, "descriptorBindingStorageBufferUpdateAfterBind:", (bool)features12.descriptorBindingStorageBufferUpdateAfterBind) << endl;
	LogInfo(LogVulkan) << format(formatStr4, "descriptorBindingUniformTexelBufferUpdateAfterBind:", (bool)features12.descriptorBindingUniformTexelBufferUpdateAfterBind) << endl;
	LogInfo(LogVulkan) << format(formatStr4, "descriptorBindingStorageTexelBufferUpdateAfterBind:", (bool)features12.descriptorBindingStorageTexelBufferUpdateAfterBind) << endl;
	LogInfo(LogVulkan) << format(formatStr4, "descriptorBindingUpdateUnusedWhilePending:", (bool)features12.descriptorBindingUpdateUnusedWhilePending) << endl;
	LogInfo(LogVulkan) << format(formatStr4, "descriptorBindingPartiallyBound:", (bool)features12.descriptorBindingPartiallyBound) << endl;
	LogInfo(LogVulkan) << format(formatStr4, "descriptorBindingVariableDescriptorCount:", (bool)features12.descriptorBindingVariableDescriptorCount) << endl;
	LogInfo(LogVulkan) << format(formatStr4, "runtimeDescriptorArray:", (bool)features12.runtimeDescriptorArray) << endl;
	LogInfo(LogVulkan) << format(formatStr4, "samplerFilterMinmax:", (bool)features12.samplerFilterMinmax) << endl;
	LogInfo(LogVulkan) << format(formatStr4, "scalarBlockLayout:", (bool)features12.scalarBlockLayout) << endl;
	LogInfo(LogVulkan) << format(formatStr4, "imagelessFramebuffer:", (bool)features12.imagelessFramebuffer) << endl;
	LogInfo(LogVulkan) << format(formatStr4, "uniformBufferStandardLayout:", (bool)features12.uniformBufferStandardLayout) << endl;
	LogInfo(LogVulkan) << format(formatStr4, "shaderSubgroupExtendedTypes:", (bool)features12.shaderSubgroupExtendedTypes) << endl;
	LogInfo(LogVulkan) << format(formatStr4, "separateDepthStencilLayouts:", (bool)features12.separateDepthStencilLayouts) << endl;
	LogInfo(LogVulkan) << format(formatStr4, "hostQueryReset:", (bool)features12.hostQueryReset) << endl;
	LogInfo(LogVulkan) << format(formatStr4, "timelineSemaphore:", (bool)features12.timelineSemaphore) << endl;
	LogInfo(LogVulkan) << format(formatStr4, "bufferDeviceAddress:", (bool)features12.bufferDeviceAddress) << endl;
	LogInfo(LogVulkan) << format(formatStr4, "bufferDeviceAddressCaptureReplay:", (bool)features12.bufferDeviceAddressCaptureReplay) << endl;
	LogInfo(LogVulkan) << format(formatStr4, "bufferDeviceAddressMultiDevice:", (bool)features12.bufferDeviceAddressMultiDevice) << endl;
	LogInfo(LogVulkan) << format(formatStr4, "vulkanMemoryModel:", (bool)features12.vulkanMemoryModel) << endl;
	LogInfo(LogVulkan) << format(formatStr4, "vulkanMemoryModelDeviceScope:", (bool)features12.vulkanMemoryModelDeviceScope) << endl;
	LogInfo(LogVulkan) << format(formatStr4, "vulkanMemoryModelAvailabilityVisibilityChains:", (bool)features12.vulkanMemoryModelAvailabilityVisibilityChains) << endl;
	LogInfo(LogVulkan) << format(formatStr4, "shaderOutputViewportIndex:", (bool)features12.shaderOutputViewportIndex) << endl;
	LogInfo(LogVulkan) << format(formatStr4, "shaderOutputLayer:", (bool)features12.shaderOutputLayer) << endl;
	LogInfo(LogVulkan) << format(formatStr4, "subgroupBroadcastDynamicId:", (bool)features12.subgroupBroadcastDynamicId) << endl;
	LogInfo(LogVulkan) << endl;

	// Vulkan 1.3 features
	LogInfo(LogVulkan) << "    Vulkan 1.3 features" << endl;
	LogInfo(LogVulkan) << format(formatStr4, "robustImageAccess:", (bool)features13.robustImageAccess) << endl;
	LogInfo(LogVulkan) << format(formatStr4, "inlineUniformBlock:", (bool)features13.inlineUniformBlock) << endl;
	LogInfo(LogVulkan) << format(formatStr4, "descriptorBindingInlineUniformBlockUpdateAfterBind:", (bool)features13.descriptorBindingInlineUniformBlockUpdateAfterBind) << endl;
	LogInfo(LogVulkan) << format(formatStr4, "pipelineCreationCacheControl:", (bool)features13.pipelineCreationCacheControl) << endl;
	LogInfo(LogVulkan) << format(formatStr4, "privateData:", (bool)features13.privateData) << endl;
	LogInfo(LogVulkan) << format(formatStr4, "shaderDemoteToHelperInvocation:", (bool)features13.shaderDemoteToHelperInvocation) << endl;
	LogInfo(LogVulkan) << format(formatStr4, "shaderTerminateInvocation:", (bool)features13.shaderTerminateInvocation) << endl;
	LogInfo(LogVulkan) << format(formatStr4, "subgroupSizeControl:", (bool)features13.subgroupSizeControl) << endl;
	LogInfo(LogVulkan) << format(formatStr4, "computeFullSubgroups:", (bool)features13.computeFullSubgroups) << endl;
	LogInfo(LogVulkan) << format(formatStr4, "synchronization2:", (bool)features13.synchronization2) << endl;
	LogInfo(LogVulkan) << format(formatStr4, "textureCompressionASTC_HDR:", (bool)features13.textureCompressionASTC_HDR) << endl;
	LogInfo(LogVulkan) << format(formatStr4, "shaderZeroInitializeWorkgroupMemory:", (bool)features13.shaderZeroInitializeWorkgroupMemory) << endl;
	LogInfo(LogVulkan) << format(formatStr4, "dynamicRendering:", (bool)features13.dynamicRendering) << endl;
	LogInfo(LogVulkan) << format(formatStr4, "shaderIntegerDotProduct:", (bool)features13.shaderIntegerDotProduct) << endl;
	LogInfo(LogVulkan) << format(formatStr4, "maintenance4:", (bool)features13.maintenance4) << endl;
	LogInfo(LogVulkan) << endl;
}

} // namespace Luna::VK