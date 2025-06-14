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

using namespace std;


namespace Luna
{

// Forward declarations
class IDescriptorSet;
class IResourceSet;
class DescriptorSetHandleType;


struct DescriptorRange
{
	DescriptorType descriptorType{ DescriptorType::None };
	uint32_t startRegister{ 0 };
	uint32_t numDescriptors{ 1 };

	constexpr DescriptorRange& SetDescriptorType(DescriptorType value) noexcept { descriptorType = value; return *this; }
	constexpr DescriptorRange& SetStartRegister(uint32_t value) noexcept { startRegister = value; return *this; }
	constexpr DescriptorRange& SetNumDescriptors(uint32_t value) noexcept { numDescriptors = value; return *this; }

	static DescriptorRange ConstantBuffer(uint32_t startRegister, uint32_t numDescriptors = 1)
	{
		auto res = DescriptorRange{
			.descriptorType		= DescriptorType::ConstantBuffer,
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

	constexpr RootParameter& SetShaderVisibility(Luna::ShaderStage value) noexcept { shaderVisibility = value; return *this; }
	constexpr RootParameter& SetParameterType(RootParameterType value) noexcept { parameterType = value; return *this; }
	constexpr RootParameter& SetStartRegister(uint32_t value) noexcept { startRegister = value; return *this; }
	constexpr RootParameter& SetRegisterSpace(uint32_t value) noexcept { registerSpace = value; return *this; }
	constexpr RootParameter& SetNum32BitConstants(uint32_t value) noexcept { num32BitConstants = value; return *this; }
	RootParameter& SetTable(const std::vector<DescriptorRange>& value) { table = value; return *this; }

	bool Validate() const
	{
		if (parameterType == RootParameterType::Table)
		{
			// Verify that the table does not contain a mix of sampler and non-sampler ranges
			if (!table.empty())
			{
				bool isSamplerTable = table[0].descriptorType == DescriptorType::Sampler;
				for (uint32_t i = 1; i < (uint32_t)table.size(); ++i)
				{
					if (isSamplerTable && table[i].descriptorType != DescriptorType::Sampler)
					{
						LogError(LogGraphics) << "RootSignature table " << i << " contains both sampler and non-sampler descriptors, which is not allowed." << endl;
						return false;
					}

					if (!isSamplerTable && table[i].descriptorType == DescriptorType::Sampler)
					{
						LogError(LogGraphics) << "RootSignature table " << i << " contains both sampler and non-sampler descriptors, which is not allowed." << endl;
						return false;
					}
				}
			}
		}
		return true;
	}

	uint32_t GetNumDescriptors() const
	{
		uint32_t numDescriptors = 0;

		if (parameterType == RootParameterType::Table)
		{
			for (const auto& tableRange : table)
			{
				numDescriptors += tableRange.numDescriptors;
			}
		}

		return numDescriptors;
	}

	bool IsSamplerTable() const
	{
		return (parameterType == RootParameterType::Table && table[0].descriptorType == DescriptorType::Sampler);
	}

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
	VulkanBindingOffsets bindingOffsets{};
	RootParameters rootParameters;

	RootSignatureDesc& SetName(const std::string& value) { name = value; return *this; }
	constexpr RootSignatureDesc& SetFlags(RootSignatureFlags value) noexcept { flags = value; return *this; }
	RootSignatureDesc& SetBindingOffsets(VulkanBindingOffsets value) noexcept { bindingOffsets = value; return *this; }
	RootSignatureDesc& SetRootParameters(const RootParameters& value) { rootParameters = value; return *this; }
	RootSignatureDesc& AppendRootParameters(const RootParameters& value)
	{
		rootParameters.insert(rootParameters.end(), value.begin(), value.end());
		return *this;
	}

	bool Validate() const
	{
		if (rootParameters.size() > MaxRootParameters)
		{
			LogError(LogGraphics) << std::format("RootSignature {} contains {} parameters, but the maximum is {}", name, rootParameters.size(), MaxRootParameters) << std::endl;
			return false;
		}

		for (const auto& rootParameter : rootParameters)
		{
			if (!rootParameter.Validate())
			{
				return false;
			}
		}
		return true;
	}
};


class RootSignature
{
public:
	
	uint32_t GetNumRootParameters() const;
	const RootParameter& GetRootParameter(uint32_t index) const;

	ResourceHandle CreateDescriptorSet(uint32_t index) const;

	const RootParameter& operator[](uint32_t index) const { return GetRootParameter(index); }

	void Initialize(RootSignatureDesc& rootSignatureDesc);

	ResourceHandle GetHandle() const { return m_handle; }

private:
	const RootSignatureDesc& GetDesc() const;

private:
	ResourceHandle m_handle;
};


class RootSignatureFactoryBase
{
protected:
	static const uint32_t MaxResources = (1 << 8);
	static const uint32_t InvalidAllocation = ~0u;

public:
	RootSignatureFactoryBase()
	{
		ClearDescs();
	}

	const RootSignatureDesc& GetRootSignatureDesc(uint32_t index) const
	{
		return m_descs[index];
	}
	
protected:
	void ResetDesc(uint32_t index)
	{
		m_descs[index] = RootSignatureDesc{};
	}

	void ClearDescs()
	{
		for (uint32_t i = 0; i < MaxResources; ++i)
		{
			ResetDesc(i);
		}
	}

protected:
	std::array<RootSignatureDesc, MaxResources> m_descs;
};

} // namespace Luna