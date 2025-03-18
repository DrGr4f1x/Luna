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
class DescriptorSetHandleType;
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
	virtual void DestroyHandle(ResourceHandleType* handle) = 0;

	// General resource methods
	virtual std::optional<ResourceType> GetResourceType(ResourceHandleType* handle) const = 0;
	virtual std::optional<ResourceState> GetUsageState(ResourceHandleType* handle) const = 0;
	virtual void SetUsageState(ResourceHandleType* handle, ResourceState newState) = 0;
	virtual std::optional<Format> GetFormat(ResourceHandleType* handle) const = 0;

	// Pixel buffer methods
	virtual std::optional<uint64_t> GetWidth(ResourceHandleType* handle) const = 0;
	virtual std::optional<uint32_t> GetHeight(ResourceHandleType* handle) const = 0;
	virtual std::optional<uint32_t> GetDepthOrArraySize(ResourceHandleType* handle) const = 0;
	virtual std::optional<uint32_t> GetNumMips(ResourceHandleType* handle) const = 0;
	virtual std::optional<uint32_t> GetNumSamples(ResourceHandleType* handle) const = 0;
	virtual std::optional<uint32_t> GetPlaneCount(ResourceHandleType* handle) const = 0;

	// Color buffer methods
	virtual std::optional<Color> GetClearColor(ResourceHandleType* handle) const = 0;

	// Depth buffer methods
	virtual std::optional<float> GetClearDepth(ResourceHandleType* handle) const = 0;
	virtual std::optional<uint8_t> GetClearStencil(ResourceHandleType* handle) const = 0;

	// Gpu buffer methods
	virtual std::optional<size_t> GetSize(ResourceHandleType* handle) const = 0;
	virtual std::optional<size_t> GetElementCount(ResourceHandleType* handle) const = 0;
	virtual std::optional<size_t> GetElementSize(ResourceHandleType* handle) const = 0;
	virtual void Update(ResourceHandleType* handle, size_t sizeInBytes, size_t offset, const void* data) const = 0;

	// Graphics pipeline methods
	virtual const GraphicsPipelineDesc& GetGraphicsPipelineDesc(ResourceHandleType* handle) const = 0;

	// Root signature methods
	virtual const RootSignatureDesc& GetRootSignatureDesc(const ResourceHandleType* handle) const = 0;
	virtual uint32_t GetNumRootParameters(const ResourceHandleType* handle) const = 0;
	virtual wil::com_ptr<DescriptorSetHandleType> CreateDescriptorSet(ResourceHandleType* handle, uint32_t rootParamIndex) const = 0;
};

} // namespace Luna