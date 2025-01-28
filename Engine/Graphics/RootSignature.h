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
#include "Graphics\PlatformData.h"


namespace Luna
{

struct DescriptorRange
{
	DescriptorType descriptorType{ DescriptorType::None };
	uint32_t startRegister{ 0 };
	uint32_t numDescriptors{ 1 };


	static DescriptorRange ConstantBuffer(uint32_t startRegister, uint32_t numDescriptors = 1)
	{
		auto res = DescriptorRange{
			.descriptorType		= DescriptorType::ConstantBuffer,
			.startRegister		= startRegister,
			.numDescriptors		= numDescriptors
		};
		return res;
	}


	static DescriptorRange DynamicConstantBuffer(uint32_t startRegister, uint32_t numDescriptors = 1)
	{
		auto res = DescriptorRange{
			.descriptorType		= DescriptorType::DynamicConstantBuffer,
			.startRegister		= startRegister,
			.numDescriptors		= numDescriptors
		};
		return res;
	}


	static DescriptorRange TextureSRV(uint32_t startRegister, uint32_t numDescriptors = 1)
	{
		auto res = DescriptorRange{
			.descriptorType		= DescriptorType::TextureSRV,
			.startRegister		= startRegister,
			.numDescriptors		= numDescriptors
		};
		return res;
	}


	static DescriptorRange TextureUAV(uint32_t startRegister, uint32_t numDescriptors = 1)
	{
		auto res = DescriptorRange{
			.descriptorType		= DescriptorType::TextureUAV,
			.startRegister		= startRegister,
			.numDescriptors		= numDescriptors
		};
		return res;
	}


	static DescriptorRange TypedBufferSRV(uint32_t startRegister, uint32_t numDescriptors = 1)
	{
		auto res = DescriptorRange{
			.descriptorType		= DescriptorType::TypedBufferSRV,
			.startRegister		= startRegister,
			.numDescriptors		= numDescriptors
		};
		return res;
	}


	static DescriptorRange TypedBufferUAV(uint32_t startRegister, uint32_t numDescriptors = 1)
	{
		auto res = DescriptorRange{
			.descriptorType		= DescriptorType::TypedBufferUAV,
			.startRegister		= startRegister,
			.numDescriptors		= numDescriptors
		};
		return res;
	}


	static DescriptorRange StructuredBufferSRV(uint32_t startRegister, uint32_t numDescriptors = 1)
	{
		auto res = DescriptorRange{
			.descriptorType		= DescriptorType::StructuredBufferSRV,
			.startRegister		= startRegister,
			.numDescriptors		= numDescriptors
		};
		return res;
	}


	static DescriptorRange StructuredBufferUAV(uint32_t startRegister, uint32_t numDescriptors = 1)
	{
		auto res = DescriptorRange{
			.descriptorType		= DescriptorType::StructuredBufferUAV,
			.startRegister		= startRegister,
			.numDescriptors		= numDescriptors
		};
		return res;
	}

	static DescriptorRange RawBufferSRV(uint32_t startRegister, uint32_t numDescriptors = 1)
	{
		auto res = DescriptorRange{
			.descriptorType		= DescriptorType::RawBufferSRV,
			.startRegister		= startRegister,
			.numDescriptors		= numDescriptors
		};
		return res;
	}


	static DescriptorRange RawBufferUAV(uint32_t startRegister, uint32_t numDescriptors = 1)
	{
		auto res = DescriptorRange{
			.descriptorType		= DescriptorType::RawBufferUAV,
			.startRegister		= startRegister,
			.numDescriptors		= numDescriptors
		};
		return res;
	}


	static DescriptorRange Sampler(uint32_t startRegister, uint32_t numDescriptors = 1)
	{
		auto res = DescriptorRange{
			.descriptorType		= DescriptorType::Sampler,
			.startRegister		= startRegister,
			.numDescriptors		= numDescriptors
		};
		return res;
	}


	static DescriptorRange RayTracingAccelStruct(uint32_t startRegister, uint32_t numDescriptors = 1)
	{
		auto res = DescriptorRange{
			.descriptorType		= DescriptorType::RayTracingAccelStruct,
			.startRegister		= startRegister,
			.numDescriptors		= numDescriptors
		};
		return res;
	}


	static DescriptorRange SamplerFeedbackTextureUAV(uint32_t startRegister, uint32_t numDescriptors = 1)
	{
		auto res = DescriptorRange{
			.descriptorType		= DescriptorType::SamplerFeedbackTextureUAV,
			.startRegister		= startRegister,
			.numDescriptors		= numDescriptors
		};
		return res;
	}
};


struct RootParameter
{
	Luna::ShaderStage shaderVisibility{ ShaderStage::All };
	RootParameterType parameterType{ RootParameterType::Unknown };
	uint32_t startRegister{ 0 };
	uint32_t registerSpace{ 0 };
	uint32_t num32BitConstants{ 0 };
	std::vector<DescriptorRange> table;

	static RootParameter RootConstants(uint32_t startRegister, uint32_t num32BitConstants, ShaderStage shaderVisibility = ShaderStage::All, uint32_t registerSpace = 0)
	{
		auto ret = RootParameter
		{
			.shaderVisibility	= shaderVisibility,
			.parameterType		= RootParameterType::RootConstants,
			.startRegister		= startRegister = startRegister,
			.registerSpace		= registerSpace,
			.num32BitConstants	= num32BitConstants
		};
		return ret;
	}

	static RootParameter RootCBV(uint32_t startRegister, ShaderStage shaderVisibility = ShaderStage::All, uint32_t registerSpace = 0)
	{
		auto ret = RootParameter
		{
			.shaderVisibility	= shaderVisibility,
			.parameterType		= RootParameterType::RootCBV,
			.startRegister		= startRegister = startRegister,
			.registerSpace		= registerSpace
		};
		return ret;
	}

	static RootParameter RootSRV(uint32_t startRegister, ShaderStage shaderVisibility = ShaderStage::All, uint32_t registerSpace = 0)
	{
		auto ret = RootParameter
		{
			.shaderVisibility	= shaderVisibility,
			.parameterType		= RootParameterType::RootSRV,
			.startRegister		= startRegister = startRegister,
			.registerSpace		= registerSpace
		};
		return ret;
	}

	static RootParameter RootUAV(uint32_t startRegister, ShaderStage shaderVisibility = ShaderStage::All, uint32_t registerSpace = 0)
	{
		auto ret = RootParameter
		{
			.shaderVisibility	= shaderVisibility,
			.parameterType		= RootParameterType::RootUAV,
			.startRegister		= startRegister = startRegister,
			.registerSpace		= registerSpace
		};
		return ret;
	}

	static RootParameter Range(Luna::DescriptorType descriptorType, uint32_t startRegister, uint32_t numDescriptors, ShaderStage shaderVisibility = ShaderStage::All, uint32_t registerSpace = 0)
	{
		auto ret = RootParameter
		{
			.shaderVisibility	= shaderVisibility,
			.parameterType		= RootParameterType::Table,
			.startRegister		= startRegister = startRegister,
			.registerSpace		= registerSpace,
			.table = { { descriptorType, startRegister, numDescriptors } }
		};
		return ret;
	}

	static RootParameter Table(std::vector<DescriptorRange> ranges, ShaderStage shaderVisibility = ShaderStage::All)
	{
		auto ret = RootParameter
		{
			.shaderVisibility	= shaderVisibility,
			.parameterType		= RootParameterType::Table,
			.table				= { ranges }
		};
		return ret;
	}
};

using RootParameters = std::vector<RootParameter>;


struct VulkanBindingOffsets
{
	uint32_t shaderResource{ 0 };
	uint32_t sampler{ 128 };
	uint32_t constantBuffer{ 256 };
	uint32_t unorderedAccess{ 384 };

	constexpr VulkanBindingOffsets& SetShaderResourceOffset(uint32_t value) { shaderResource = value; return *this; }
	constexpr VulkanBindingOffsets& SetSamplerOffset(uint32_t value) { sampler = value; return *this; }
	constexpr VulkanBindingOffsets& SetConstantBufferOffset(uint32_t value) { constantBuffer = value; return *this; }
	constexpr VulkanBindingOffsets& SetUnorderedAccessViewOffset(uint32_t value) { unorderedAccess = value; return *this; }
};


struct RootSignatureDesc
{
	std::string name{};
	RootSignatureFlags flags{ RootSignatureFlags::None };
	std::vector<RootParameters> rootParameters;
	VulkanBindingOffsets bindingOffsets{};
};


class RootSignature
{
public:
	bool Initialize(RootSignatureDesc& desc);

	IPlatformData* GetPlatformData() const noexcept { return m_platformData.get(); }

private:
	wil::com_ptr<IPlatformData> m_platformData;
};

} // namespace Luna