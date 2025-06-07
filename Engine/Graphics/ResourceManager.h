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

#include "Graphics\ColorBuffer.h"
#include "Graphics\DepthBuffer.h"
#include "Graphics\DescriptorSet.h"
#include "Graphics\GpuBuffer.h"
#include "Graphics\GraphicsCommon.h"
#include "Graphics\PipelineState.h"
#include "Graphics\RootSignature.h"


namespace Luna
{

class IResourceManager
{
public:
	enum ManagedResourceType
	{
		ManagedColorBuffer = 0x0001,
		ManagedDepthBuffer = 0x0002,
		ManagedGpuBuffer = 0x0004,
		ManagedGraphicsPipeline = 0x0008,
		ManagedRootSignature = 0x0010,
		ManagedDescriptorSet = 0x0020,
	};

public:
	// Creation/destruction methods
	virtual ResourceHandle CreateColorBuffer(const ColorBufferDesc& colorBufferDesc) = 0;
	virtual ResourceHandle CreateDepthBuffer(const DepthBufferDesc& depthBufferDesc) = 0;
	virtual ResourceHandle CreateGpuBuffer(const GpuBufferDesc& gpuBufferDesc) = 0;
	virtual ResourceHandle CreateGraphicsPipeline(const GraphicsPipelineDesc& pipelineDesc) = 0;
	virtual ResourceHandle CreateRootSignature(const RootSignatureDesc& rootSignatureDesc) = 0;
	virtual void DestroyHandle(const ResourceHandleType* handle) = 0;

	// Load methods
	//virtual ResourceHandle LoadTexture(const std::string& filename, Format format, bool sRgb) = 0;

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


template <
	typename TDevice, 
	typename TAllocator, 
	typename TColorBufferFactory, 
	typename TDepthBufferFactory, 
	typename TGpuBufferFactory, 
	typename TPipelineStateFactory,
	typename TRootSignatureFactory,
	typename TDescriptorSetFactory>
class TResourceManager : public IResourceManager
{
public:
	TResourceManager(TDevice* device, TAllocator* allocator)
		: m_colorBufferFactory{ this, device, allocator }
		, m_depthBufferFactory{ this, device, allocator }
		, m_gpuBufferFactory{ this, device, allocator }
		, m_pipelineStateFactory{ this, device }
		, m_rootSignatureFactory{ this, device }
		, m_descriptorSetFactory{ this, device }
	{}

	// Creation/destruction methods
	ResourceHandle CreateColorBuffer(const ColorBufferDesc& colorBufferDesc) override
	{
		return m_colorBufferFactory.CreateColorBuffer(colorBufferDesc);
	}


	ResourceHandle CreateDepthBuffer(const DepthBufferDesc& depthBufferDesc) override
	{
		return m_depthBufferFactory.CreateDepthBuffer(depthBufferDesc);
	}


	ResourceHandle CreateGpuBuffer(const GpuBufferDesc& gpuBufferDesc) override
	{
		return m_gpuBufferFactory.CreateGpuBuffer(gpuBufferDesc);
	}


	ResourceHandle CreateGraphicsPipeline(const GraphicsPipelineDesc& pipelineDesc) override
	{
		return m_pipelineStateFactory.CreateGraphicsPipeline(pipelineDesc);
	}


	ResourceHandle CreateRootSignature(const RootSignatureDesc& rootSignatureDesc) override
	{
		return m_rootSignatureFactory.CreateRootSignature(rootSignatureDesc);
	}

	void DestroyHandle(const ResourceHandleType* handle) override
	{ 
		const auto [index, type] = UnpackHandle(handle);

		switch (type)
		{
		case ManagedColorBuffer:
			m_colorBufferFactory.Destroy(index);
			break;

		case ManagedDepthBuffer:
			m_depthBufferFactory.Destroy(index);
			break;

		case ManagedGpuBuffer:
			m_gpuBufferFactory.Destroy(index);
			break;

		case ManagedGraphicsPipeline:
			m_pipelineStateFactory.Destroy(index);
			break;

		case ManagedRootSignature:
			m_rootSignatureFactory.Destroy(index);
			break;

		case ManagedDescriptorSet:
			m_descriptorSetFactory.Destroy(index);
			break;

		default:
			assert(false);
			LogError(LogGraphics) << "ResourceManager::DestroyHandle(): unknown resource type " << type << endl;
			break;
		}
	}


	// General resource methods
	std::optional<ResourceType> GetResourceType(const ResourceHandleType* handle) const override
	{
		const auto [index, type] = UnpackHandle(handle);

		switch (type)
		{
		case ManagedColorBuffer:	return make_optional(m_colorBufferFactory.GetResourceType(index));
		case ManagedDepthBuffer:	return make_optional(m_depthBufferFactory.GetResourceType(index));
		case ManagedGpuBuffer:		return make_optional(m_gpuBufferFactory.GetResourceType(index));

		default:
			assert(false);
			LogError(LogGraphics) << "ResourceManager::GetResourceType(): unknown resource type " << type << endl;
			return std::nullopt;
		}
	}


	std::optional<ResourceState> GetUsageState(const ResourceHandleType* handle) const override
	{
		const auto [index, type] = UnpackHandle(handle);

		switch (type)
		{
		case ManagedColorBuffer:	return make_optional(m_colorBufferFactory.GetUsageState(index));
		case ManagedDepthBuffer:	return make_optional(m_depthBufferFactory.GetUsageState(index));
		case ManagedGpuBuffer:		return make_optional(m_gpuBufferFactory.GetUsageState(index));

		default:
			assert(false);
			LogError(LogGraphics) << "ResourceManager::GetUsageState(): unknown resource type " << type << endl;
			return std::nullopt;
		}
	}


	void SetUsageState(const ResourceHandleType* handle, ResourceState newState) override
	{
		const auto [index, type] = UnpackHandle(handle);

		switch (type)
		{
		case ManagedColorBuffer:	m_colorBufferFactory.SetUsageState(index, newState);	break;
		case ManagedDepthBuffer:	m_depthBufferFactory.SetUsageState(index, newState);	break;
		case ManagedGpuBuffer:		m_gpuBufferFactory.SetUsageState(index, newState);		break;

		default:
			assert(false);
			LogError(LogGraphics) << "ResourceManager::SetUsageState(): unknown resource type " << type << endl;
			break;
		}
	}


	std::optional<Format> GetFormat(const ResourceHandleType* handle) const override
	{
		const auto [index, type] = UnpackHandle(handle);

		switch (type)
		{
		case ManagedColorBuffer:	return make_optional(m_colorBufferFactory.GetFormat(index));
		case ManagedDepthBuffer:	return make_optional(m_depthBufferFactory.GetFormat(index));
		case ManagedGpuBuffer:		return make_optional(m_gpuBufferFactory.GetFormat(index));

		default:
			assert(false);
			LogError(LogGraphics) << "ResourceManager::GetFormat(): unknown resource type " << type << endl;
			return std::nullopt;
		}
	}


	// Pixel buffer methods
	std::optional<uint64_t> GetWidth(const ResourceHandleType* handle) const override
	{
		const auto [index, type] = UnpackHandle(handle);

		switch (type)
		{
		case ManagedColorBuffer: return make_optional(m_colorBufferFactory.GetWidth(index));
		case ManagedDepthBuffer: return make_optional(m_depthBufferFactory.GetWidth(index));
		default:
			assert(false);
			LogError(LogGraphics) << "ResourceManager::GetWidth(): unknown resource type " << type << endl;
			return std::nullopt;
		}
	}


	std::optional<uint32_t> GetHeight(const ResourceHandleType* handle) const override 
	{
		const auto [index, type] = UnpackHandle(handle);

		switch (type)
		{
		case ManagedColorBuffer: return make_optional(m_colorBufferFactory.GetHeight(index));
		case ManagedDepthBuffer: return make_optional(m_depthBufferFactory.GetHeight(index));
		default:
			assert(false);
			LogError(LogGraphics) << "ResourceManager::GetHeight(): unknown resource type " << type << endl;
			return std::nullopt;
		}
	}


	std::optional<uint32_t> GetDepthOrArraySize(const ResourceHandleType* handle) const override
	{
		const auto [index, type] = UnpackHandle(handle);

		switch (type)
		{
		case ManagedColorBuffer: return make_optional(m_colorBufferFactory.GetDepthOrArraySize(index));
		case ManagedDepthBuffer: return make_optional(m_depthBufferFactory.GetDepthOrArraySize(index));
		default:
			assert(false);
			LogError(LogGraphics) << "ResourceManager::GetDepthOrArraySize(): unknown resource type " << type << endl;
			return std::nullopt;
		}
	}


	std::optional<uint32_t> GetNumMips(const ResourceHandleType* handle) const override
	{
		const auto [index, type] = UnpackHandle(handle);

		switch (type)
		{
		case ManagedColorBuffer: return make_optional(m_colorBufferFactory.GetNumMips(index));
		case ManagedDepthBuffer: return make_optional(m_depthBufferFactory.GetNumMips(index));
		default:
			assert(false);
			LogError(LogGraphics) << "ResourceManager::GetNumMips(): unknown resource type " << type << endl;
			return std::nullopt;
		}
	}


	std::optional<uint32_t> GetNumSamples(const ResourceHandleType* handle) const override
	{
		const auto [index, type] = UnpackHandle(handle);

		switch (type)
		{
		case ManagedColorBuffer: return make_optional(m_colorBufferFactory.GetNumSamples(index));
		case ManagedDepthBuffer: return make_optional(m_depthBufferFactory.GetNumSamples(index));
		default:
			assert(false);
			LogError(LogGraphics) << "ResourceManager::GetNumSamples(): unknown resource type " << type << endl;
			return std::nullopt;
		}
	}


	std::optional<uint32_t> GetPlaneCount(const ResourceHandleType* handle) const override
	{
		const auto [index, type] = UnpackHandle(handle);

		switch (type)
		{
		case ManagedColorBuffer: return make_optional(m_colorBufferFactory.GetPlaneCount(index));
		case ManagedDepthBuffer: return make_optional(m_depthBufferFactory.GetPlaneCount(index));
		default:
			assert(false);
			LogError(LogGraphics) << "ResourceManager::GetPlaneCount(): unknown resource type " << type << endl;
			return std::nullopt;
		}
	}


	// Color buffer methods
	std::optional<Color> GetClearColor(const ResourceHandleType* handle) const override
	{
		const auto [index, type] = UnpackHandle(handle);

		switch (type)
		{
		case ManagedColorBuffer: return make_optional(m_colorBufferFactory.GetClearColor(index));
		default:
			assert(false);
			LogError(LogGraphics) << "ResourceManager::GetClearColor(): unknown resource type " << type << endl;
			return std::nullopt;
		}
	}


	// Depth buffer methods
	std::optional<float> GetClearDepth(const ResourceHandleType* handle) const override
	{
		const auto [index, type] = UnpackHandle(handle);

		switch (type)
		{
		case ManagedDepthBuffer: return make_optional(m_depthBufferFactory.GetClearDepth(index));
		default:
			assert(false);
			LogError(LogGraphics) << "ResourceManager::GetClearDepth(): unknown resource type " << type << endl;
			return std::nullopt;
		}
	}


	std::optional<uint8_t> GetClearStencil(const ResourceHandleType* handle) const override
	{
		const auto [index, type] = UnpackHandle(handle);

		switch (type)
		{
		case ManagedDepthBuffer: return make_optional(m_depthBufferFactory.GetClearStencil(index));
		default:
			assert(false);
			LogError(LogGraphics) << "ResourceManager::GetClearStencil(): unknown resource type " << type << endl;
			return std::nullopt;
		}
	}


	// Gpu buffer methods
	std::optional<size_t> GetSize(const ResourceHandleType* handle) const override
	{
		const auto [index, type] = UnpackHandle(handle);

		switch (type)
		{
		case ManagedGpuBuffer: return make_optional(m_gpuBufferFactory.GetSize(index));
		default:
			assert(false);
			LogError(LogGraphics) << "ResourceManager::GetSize(): unknown resource type " << type << endl;
			return std::nullopt;
		}
	}


	std::optional<size_t> GetElementCount(const ResourceHandleType* handle) const override
	{
		const auto [index, type] = UnpackHandle(handle);

		switch (type)
		{
		case ManagedGpuBuffer: return make_optional(m_gpuBufferFactory.GetElementCount(index));
		default:
			assert(false);
			LogError(LogGraphics) << "ResourceManager::GetElementCount(): unknown resource type " << type << endl;
			return std::nullopt;
		}
	}


	std::optional<size_t> GetElementSize(const ResourceHandleType* handle) const override
	{
		const auto [index, type] = UnpackHandle(handle);

		switch (type)
		{
		case ManagedGpuBuffer: return make_optional(m_gpuBufferFactory.GetElementSize(index));
		default:
			assert(false);
			LogError(LogGraphics) << "ResourceManager::GetElementSize(): unknown resource type " << type << endl;
			return std::nullopt;
		}
	}


	void Update(const ResourceHandleType* handle, size_t sizeInBytes, size_t offset, const void* data) const override
	{
		const auto [index, type] = UnpackHandle(handle);

		switch (type)
		{
		case ManagedGpuBuffer: m_gpuBufferFactory.Update(index, sizeInBytes, offset, data); break;
		default:
			assert(false);
			LogError(LogGraphics) << "ResourceManager::Update(): unknown resource type " << type << endl;
			break;
		}
	}


	// Graphics pipeline methods
	const GraphicsPipelineDesc& GetGraphicsPipelineDesc(const ResourceHandleType* handle) const override
	{
		const auto [index, type] = UnpackHandle(handle);

		assert(type == ManagedGraphicsPipeline);

		return m_pipelineStateFactory.GetDesc(index);
	}


	// Root signature methods
	const RootSignatureDesc& GetRootSignatureDesc(const ResourceHandleType* handle) const override
	{
		const auto [index, type] = UnpackHandle(handle);

		assert(type == ManagedRootSignature);

		return m_rootSignatureFactory.GetDesc(index);
	}


	uint32_t GetNumRootParameters(const ResourceHandleType* handle) const override
	{
		return (uint32_t)GetRootSignatureDesc(handle).rootParameters.size();
	}


	// Descriptor set methods
	void SetSRV(const ResourceHandleType* handle, int slot, const ColorBuffer& colorBuffer) override
	{
		const auto [index, type] = UnpackHandle(handle);

		assert(type == ManagedDescriptorSet);

		m_descriptorSetFactory.SetSRV(index, slot, colorBuffer);
	}


	void SetSRV(const ResourceHandleType* handle, int slot, const DepthBuffer& depthBuffer, bool depthSrv = true) override
	{
		const auto [index, type] = UnpackHandle(handle);

		assert(type == ManagedDescriptorSet);

		m_descriptorSetFactory.SetSRV(index, slot, depthBuffer, depthSrv);
	}


	void SetSRV(const ResourceHandleType* handle, int slot, const GpuBuffer& gpuBuffer) override
	{
		const auto [index, type] = UnpackHandle(handle);

		assert(type == ManagedDescriptorSet);

		m_descriptorSetFactory.SetSRV(index, slot, gpuBuffer);
	}


	void SetUAV(const ResourceHandleType* handle, int slot, const ColorBuffer& colorBuffer, uint32_t uavIndex = 0) override
	{
		const auto [index, type] = UnpackHandle(handle);

		assert(type == ManagedDescriptorSet);

		m_descriptorSetFactory.SetUAV(index, slot, colorBuffer, uavIndex);
	}


	void SetUAV(const ResourceHandleType* handle, int slot, const DepthBuffer& depthBuffer) override
	{
		const auto [index, type] = UnpackHandle(handle);

		assert(type == ManagedDescriptorSet);

		m_descriptorSetFactory.SetUAV(index, slot, depthBuffer);
	}


	void SetUAV(const ResourceHandleType* handle, int slot, const GpuBuffer& gpuBuffer) override
	{
		const auto [index, type] = UnpackHandle(handle);

		assert(type == ManagedDescriptorSet);

		m_descriptorSetFactory.SetUAV(index, slot, gpuBuffer);
	}


	void SetCBV(const ResourceHandleType* handle, int slot, const GpuBuffer& gpuBuffer) override
	{
		const auto [index, type] = UnpackHandle(handle);

		assert(type == ManagedDescriptorSet);

		m_descriptorSetFactory.SetCBV(index, slot, gpuBuffer);
	}


	void SetDynamicOffset(const ResourceHandleType* handle, uint32_t offset) override
	{
		const auto [index, type] = UnpackHandle(handle);

		assert(type == ManagedDescriptorSet);

		m_descriptorSetFactory.SetDynamicOffset(index, offset);
	}


	void UpdateGpuDescriptors(const ResourceHandleType* handle) override
	{
		const auto [index, type] = UnpackHandle(handle);

		assert(type == ManagedDescriptorSet);

		m_descriptorSetFactory.UpdateGpuDescriptors(index);
	}

protected:
	std::pair<uint32_t, uint32_t> UnpackHandle(const ResourceHandleType* handle) const
	{
		assert(handle != nullptr);

		auto index = handle->GetIndex();
		auto type = handle->GetType();

		return std::make_pair(index, type);
	}

protected:
	TColorBufferFactory m_colorBufferFactory;
	TDepthBufferFactory m_depthBufferFactory;
	TGpuBufferFactory m_gpuBufferFactory;
	TPipelineStateFactory m_pipelineStateFactory;
	TRootSignatureFactory m_rootSignatureFactory;
	TDescriptorSetFactory m_descriptorSetFactory;
};

} // namespace Luna