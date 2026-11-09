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

#include "Graphics\Texture.h"


namespace Luna
{

// Forward declarations
class IColorBuffer;
class IDepthBuffer;
class IDescriptor;
class IGpuBuffer;
class ISampler;

using ColorBufferPtr = std::shared_ptr<IColorBuffer>;
using DepthBufferPtr = std::shared_ptr<IDepthBuffer>;
using GpuBufferPtr = std::shared_ptr<IGpuBuffer>;
using SamplerPtr = std::shared_ptr<ISampler>;

struct StencilSrv
{
	StencilSrv(DepthBufferPtr inDepthBuffer) : depthBuffer{ inDepthBuffer } {}
	DepthBufferPtr depthBuffer;
};

using SrvProvider = std::variant<ColorBufferPtr, DepthBufferPtr, StencilSrv, GpuBufferPtr, TexturePtr>;

using UavProvider = std::variant<ColorBufferPtr, DepthBufferPtr, GpuBufferPtr>;

// helper type for the visitor #4
template<class... Ts>
struct overloaded : Ts... { using Ts::operator()...; };
// explicit deduction guide (not needed as of C++20)
template<class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;


class IDescriptorSet
{
public:
	virtual ~IDescriptorSet() = default;

	virtual void SetBindlessSRVs(uint32_t slot, std::span<const IDescriptor*> descriptors) = 0;

	virtual void SetSRV(uint32_t slot, ColorBufferPtr colorBuffer) = 0;
	virtual void SetSRV(uint32_t slot, DepthBufferPtr depthBuffer, bool depthSrv = true) = 0;
	virtual void SetSRV(uint32_t slot, GpuBufferPtr gpuBuffer) = 0;
	virtual void SetSRV(uint32_t slot, TexturePtr texture) = 0;

	void SetSRVs(uint32_t startSlot, std::initializer_list<SrvProvider> srvs)
	{
		uint32_t slot = startSlot;
		for (auto srvProvider : srvs)
		{
			std::visit(overloaded{
				[slot, this](ColorBufferPtr colorBuffer) { SetSRV(slot, colorBuffer); },
				[slot, this](DepthBufferPtr depthBuffer) { SetSRV(slot, depthBuffer, true);  },
				[slot, this](StencilSrv stencilSrv) { SetSRV(slot, stencilSrv.depthBuffer, false); },
				[slot, this](GpuBufferPtr gpuBuffer) { SetSRV(slot, gpuBuffer); },
				[slot, this](TexturePtr texture) { SetSRV(slot, texture); }
				}, srvProvider);
			++slot;
		}
	}

	void SetSRVs(uint32_t startSlot, std::span<TexturePtr> textures)
	{
		uint32_t slot = startSlot;
		for (auto texture : textures)
		{
			SetSRV(slot, texture);
			++slot;
		}
	}

	virtual void SetUAV(uint32_t slot, ColorBufferPtr colorBuffer, uint32_t uavIndex = 0) = 0;
	virtual void SetUAV(uint32_t slot, DepthBufferPtr depthBuffer) = 0;
	virtual void SetUAV(uint32_t slot, GpuBufferPtr gpuBuffer) = 0;

	void SetUAVs(uint32_t startSlot, std::initializer_list<UavProvider> uavs)
	{
		uint32_t slot = startSlot;
		for (auto uavProvider : uavs)
		{
			std::visit(overloaded{
				[slot, this](ColorBufferPtr colorBuffer) { SetUAV(slot, colorBuffer); },
				[slot, this](DepthBufferPtr depthBuffer) { SetUAV(slot, depthBuffer); },
				[slot, this](GpuBufferPtr gpuBuffer) { SetUAV(slot, gpuBuffer); }
				}, uavProvider);
			++slot;
		}
	}

	virtual void SetCBV(uint32_t slot, GpuBufferPtr gpuBuffer) = 0;

	void SetCBVs(uint32_t startSlot, std::initializer_list<GpuBufferPtr> gpuBuffers)
	{
		uint32_t slot = startSlot;
		for (auto gpuBuffer : gpuBuffers)
		{
			SetCBV(slot, gpuBuffer);
			++slot;
		}
	}

	virtual void SetSampler(uint32_t slot, SamplerPtr sampler) = 0;

	void SetSamplers(uint32_t startSlot, std::initializer_list<SamplerPtr> samplers)
	{
		uint32_t slot = startSlot;
		for (auto sampler : samplers)
		{
			SetSampler(slot, sampler);
			++slot;
		}
	}
};

using DescriptorSetPtr = std::shared_ptr<IDescriptorSet>;

} // namespace Luna