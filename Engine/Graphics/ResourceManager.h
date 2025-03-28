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

#include "Graphics\Enums.h"
#include "Graphics\Formats.h"


namespace Luna
{

// Forward declarations
class ColorBuffer;
class DepthBuffer;
class DescriptorSetHandleType;
class GpuBuffer;
class IResourceManager;
struct ColorBufferDesc;
struct DepthBufferDesc;
struct GpuBufferDesc;
struct GraphicsPipelineDesc;
struct RootSignatureDesc;


class __declspec(uuid("76EB2254-E7A6-4F2D-9037-A0FE41926CE2")) ResourceHandleType : public RefCounted<ResourceHandleType>
{
public:
	ResourceHandleType(uint32_t index, uint32_t type, IResourceManager* const manager)
		: m_index{ index }
		, m_type{ type }
		, m_manager{ manager }
	{}
	~ResourceHandleType();

	uint32_t GetIndex() const { return m_index; }
	uint32_t GetType() const { return m_type; }

private:
	const uint32_t m_index{ 0 };
	const uint32_t m_type{ 0 };
	IResourceManager* const m_manager{ nullptr };
};

using ResourceHandle = wil::com_ptr<ResourceHandleType>;


class IResourceManager
{
public:
	// Creation/destruction methods
	virtual ResourceHandle CreateColorBuffer(const ColorBufferDesc& colorBufferDesc) = 0;
	virtual ResourceHandle CreateDepthBuffer(const DepthBufferDesc& depthBufferDesc) = 0;
	virtual ResourceHandle CreateGpuBuffer(const GpuBufferDesc& gpuBufferDesc) = 0;
	virtual ResourceHandle CreateGraphicsPipeline(const GraphicsPipelineDesc& pipelineDesc) = 0;
	virtual ResourceHandle CreateRootSignature(const RootSignatureDesc& rootSignatureDesc) = 0;
	virtual void DestroyHandle(const ResourceHandleType* handle) = 0;

	// General resource methods
	virtual std::optional<ResourceType> GetResourceType(const ResourceHandleType* handle) const = 0;
	virtual std::optional<ResourceState> GetUsageState(const ResourceHandleType* handle) const = 0;
	virtual void SetUsageState(const ResourceHandleType* handle, ResourceState newState) = 0;
	virtual std::optional<Format> GetFormat(const ResourceHandleType* handle) const = 0;

	// Pixel buffer methods
	virtual std::optional<uint64_t> GetWidth(const ResourceHandleType* handle) const = 0;
	virtual std::optional<uint32_t> GetHeight(const ResourceHandleType* handle) const = 0;
	virtual std::optional<uint32_t> GetDepthOrArraySize(const ResourceHandleType* handle) const = 0;
	virtual std::optional<uint32_t> GetNumMips(const ResourceHandleType* handle) const = 0;
	virtual std::optional<uint32_t> GetNumSamples(const ResourceHandleType* handle) const = 0;
	virtual std::optional<uint32_t> GetPlaneCount(const ResourceHandleType* handle) const = 0;

	// Color buffer methods
	virtual std::optional<Color> GetClearColor(const ResourceHandleType* handle) const = 0;

	// Depth buffer methods
	virtual std::optional<float> GetClearDepth(const ResourceHandleType* handle) const = 0;
	virtual std::optional<uint8_t> GetClearStencil(const ResourceHandleType* handle) const = 0;

	// Gpu buffer methods
	virtual std::optional<size_t> GetSize(const ResourceHandleType* handle) const = 0;
	virtual std::optional<size_t> GetElementCount(const ResourceHandleType* handle) const = 0;
	virtual std::optional<size_t> GetElementSize(const ResourceHandleType* handle) const = 0;
	virtual void Update(const ResourceHandleType* handle, size_t sizeInBytes, size_t offset, const void* data) const = 0;

	// Graphics pipeline methods
	virtual const GraphicsPipelineDesc& GetGraphicsPipelineDesc(const ResourceHandleType* handle) const = 0;

	// Root signature methods
	virtual const RootSignatureDesc& GetRootSignatureDesc(const ResourceHandleType* handle) const = 0;
	virtual uint32_t GetNumRootParameters(const ResourceHandleType* handle) const = 0;
	virtual ResourceHandle CreateDescriptorSet(const ResourceHandleType* handle, uint32_t rootParamIndex) = 0;

	// Descriptor set methods
	virtual void SetSRV(const ResourceHandleType* handle, int slot, const ColorBuffer& colorBuffer) = 0;
	virtual void SetSRV(const ResourceHandleType* handle, int slot, const DepthBuffer& depthBuffer, bool depthSrv = true) = 0;
	virtual void SetSRV(const ResourceHandleType* handle, int slot, const GpuBuffer& gpuBuffer) = 0;
	virtual void SetUAV(const ResourceHandleType* handle, int slot, const ColorBuffer& colorBuffer, uint32_t uavIndex = 0) = 0;
	virtual void SetUAV(const ResourceHandleType* handle, int slot, const DepthBuffer& depthBuffer) = 0;
	virtual void SetUAV(const ResourceHandleType* handle, int slot, const GpuBuffer& gpuBuffer) = 0;
	virtual void SetCBV(const ResourceHandleType* handle, int slot, const GpuBuffer& gpuBuffer) = 0;
	virtual void SetDynamicOffset(const ResourceHandleType* handle, uint32_t offset) = 0;
	virtual void UpdateGpuDescriptors(const ResourceHandleType* handle) = 0;
};

} // namespace Luna