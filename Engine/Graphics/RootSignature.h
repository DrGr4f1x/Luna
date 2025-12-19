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
#include "Graphics\DescriptorSet.h"
#include "Graphics\Sampler.h"

namespace Luna
{

// Forward declarations
class IDescriptorSet;
class IResourceSet;


inline const uint32_t APPEND_REGISTER{ ~0u };

struct DescriptorRange
{
	DescriptorType descriptorType{ DescriptorType::None };
	uint32_t startRegister{ 0 };
	uint32_t numDescriptors{ 1 };
	uint32_t registerSpace{ 0 };
	DescriptorRangeFlags flags{ DescriptorRangeFlags::None };

	constexpr DescriptorRange& SetDescriptorType(DescriptorType value) noexcept { descriptorType = value; return *this; }
	constexpr DescriptorRange& SetStartRegister(uint32_t value) noexcept { startRegister = value; return *this; }
	constexpr DescriptorRange& SetNumDescriptors(uint32_t value) noexcept { numDescriptors = value; return *this; }
	constexpr DescriptorRange& SetRegisterSpace(uint32_t value) noexcept { registerSpace = value; return *this; }
	constexpr DescriptorRange& SetFlags(DescriptorRangeFlags value) noexcept { flags = value; return *this; }
};


class RangeBuilder
{
public:
	explicit RangeBuilder(DescriptorType type) : m_type{ type } 
	{}

	RangeBuilder(DescriptorType type, DescriptorRangeFlags flags)
		: m_type{ type }
		, m_flags{ flags }
	{}

	operator DescriptorRange()
	{
		return DescriptorRange{
			.descriptorType		= m_type,
			.startRegister		= APPEND_REGISTER,
			.numDescriptors		= 1,
			.registerSpace		= 0,
			.flags				= m_flags
		};
	}

	DescriptorRange operator()(uint32_t startRegister, uint32_t numDescriptors = 1, uint32_t registerSpace = 0, DescriptorRangeFlags flags = DescriptorRangeFlags::None)
	{
		return DescriptorRange{
			.descriptorType		= m_type,
			.startRegister		= startRegister,
			.numDescriptors		= numDescriptors,
			.registerSpace		= registerSpace,
			.flags				= m_flags
		};
	}

private:
	const DescriptorType m_type;
	const DescriptorRangeFlags m_flags{ DescriptorRangeFlags::None };
};


inline RangeBuilder ConstantBuffer{ DescriptorType::ConstantBuffer };
inline RangeBuilder TextureSRV{ DescriptorType::TextureSRV };
inline RangeBuilder TextureUAV{ DescriptorType::TextureUAV };
inline RangeBuilder TypedBufferSRV{ DescriptorType::TypedBufferSRV };
inline RangeBuilder TypedBufferUAV{ DescriptorType::TypedBufferUAV };
inline RangeBuilder StructuredBufferSRV{ DescriptorType::StructuredBufferSRV };
inline RangeBuilder StructuredBufferUAV{ DescriptorType::StructuredBufferUAV };
inline RangeBuilder RawBufferSRV{ DescriptorType::RawBufferSRV };
inline RangeBuilder RawBufferUAV{ DescriptorType::RawBufferUAV };
inline RangeBuilder Sampler{ DescriptorType::Sampler };
inline RangeBuilder RayTracingAccelStruct{ DescriptorType::RayTracingAccelStruct };
inline RangeBuilder SamplerFeedbackTextureUAV{ DescriptorType::SamplerFeedbackTextureUAV };

inline DescriptorRangeFlags BindlessFlags{ DescriptorRangeFlags::AllowUpdateAfterSet | DescriptorRangeFlags::PartiallyBound | DescriptorRangeFlags::VariableSizedArray };
inline RangeBuilder BindlessConstantBuffer{ DescriptorType::ConstantBuffer, BindlessFlags };
inline RangeBuilder BindlessTextureSRV{ DescriptorType::TextureSRV, BindlessFlags };
inline RangeBuilder BindlessTextureUAV{ DescriptorType::TextureUAV, BindlessFlags };
inline RangeBuilder BindlessTypedBufferSRV{ DescriptorType::TypedBufferSRV, BindlessFlags };
inline RangeBuilder BindlessTypedBufferUAV{ DescriptorType::TypedBufferUAV, BindlessFlags };
inline RangeBuilder BindlessStructuredBufferSRV{ DescriptorType::StructuredBufferSRV, BindlessFlags };
inline RangeBuilder BindlessStructuredBufferUAV{ DescriptorType::StructuredBufferUAV, BindlessFlags };
inline RangeBuilder BindlessRawBufferSRV{ DescriptorType::RawBufferSRV, BindlessFlags };
inline RangeBuilder BindlessRawBufferUAV{ DescriptorType::RawBufferUAV, BindlessFlags };
inline RangeBuilder BindlessSampler{ DescriptorType::Sampler, BindlessFlags };


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
						LogError(LogGraphics) << "RootSignature table " << i << " contains both sampler and non-sampler descriptors, which is not allowed." << std::endl;
						return false;
					}

					if (!isSamplerTable && table[i].descriptorType == DescriptorType::Sampler)
					{
						LogError(LogGraphics) << "RootSignature table " << i << " contains both sampler and non-sampler descriptors, which is not allowed." << std::endl;
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

	bool IsSamplerTable() const noexcept
	{
		return (parameterType == RootParameterType::Table && table[0].descriptorType == DescriptorType::Sampler);
	}

	DescriptorType GetDescriptorType(uint32_t slot)
	{
		// Search for the range containing slot, and return its descriptor type
		uint32_t currentSlot = 0;
		for (const auto& range : table)
		{
			if (slot >= currentSlot && slot < (currentSlot + range.numDescriptors))
			{
				return range.descriptorType;
			}

			currentSlot += range.numDescriptors;
		}
		return DescriptorType::None;
	}

	uint32_t GetRegisterForSlot(uint32_t slot)
	{
		// Search for the range containing slot, and the corresponding register
		uint32_t currentSlot = 0;
		uint32_t currentStartRegister = 0;
		for (const auto& range : table)
		{
			if (range.startRegister != APPEND_REGISTER)
			{
				currentStartRegister = range.startRegister;
			}

			if (slot >= currentSlot && slot < (currentSlot + range.numDescriptors))
			{
				return currentStartRegister + (slot - currentSlot);
			}

			currentSlot += range.numDescriptors;
			currentStartRegister += range.numDescriptors;
		}

		return ~0u;
	}

	uint32_t GetRangeIndex(uint32_t slot)
	{
		// Search for the range containing slot, and return its index
		uint32_t rangeIndex = 0;
		uint32_t currentSlot = 0;
		uint32_t currentStartRegister = 0;
		for (const auto& range : table)
		{
			if (range.startRegister != APPEND_REGISTER)
			{
				currentStartRegister = range.startRegister;
			}

			if (slot >= currentSlot && slot < (currentSlot + range.numDescriptors))
			{
				return rangeIndex;
			}

			currentSlot += range.numDescriptors;
			currentStartRegister += range.numDescriptors;
			++rangeIndex;
		}
		return ~0u;
	}

	uint32_t GetRangeStartRegister(uint32_t rangeIndex)
	{
		uint32_t currentStartRegister = 0;
		for (uint32_t i = 0; i < rangeIndex; ++i)
		{
			const auto& range = table[i];
			if (range.startRegister != APPEND_REGISTER)
			{
				currentStartRegister = range.startRegister;
			}
			currentStartRegister += range.numDescriptors;
		}
		return currentStartRegister;
	}
};

using RootParameters = std::vector<RootParameter>;


class RootConstantsBuilder
{
public:
	RootParameter operator()(uint32_t startRegister, uint32_t num32BitConstants, ShaderStage shaderVisibility = ShaderStage::All, uint32_t registerSpace = 0)
	{
		return RootParameter{
			.shaderVisibility	= shaderVisibility,
			.parameterType		= RootParameterType::RootConstants,
			.startRegister		= startRegister,
			.registerSpace		= registerSpace,
			.num32BitConstants	= num32BitConstants
		};
	}
};

inline RootConstantsBuilder RootConstants;


class RootSrvUavCbvBuilder
{
public:
	explicit RootSrvUavCbvBuilder(RootParameterType type)
		: m_type{ type }
	{}

	RootParameter operator()(uint32_t startRegister, ShaderStage shaderVisibility = ShaderStage::All, uint32_t registerSpace = 0)
	{ 
		return RootParameter{
			.shaderVisibility	= shaderVisibility,
			.parameterType		= m_type,
			.startRegister		= startRegister,
			.registerSpace		= registerSpace
		};
	}

private:
	const RootParameterType m_type;
};

inline RootSrvUavCbvBuilder RootCBV{ RootParameterType::RootCBV };
inline RootSrvUavCbvBuilder RootSRV{ RootParameterType::RootSRV };
inline RootSrvUavCbvBuilder RootUAV{ RootParameterType::RootUAV };


class RootTableBuilder
{
public:
	RootParameter operator()(std::vector<DescriptorRange> ranges, ShaderStage shaderVisibility = ShaderStage::All)
	{
		return RootParameter{
			.shaderVisibility	= shaderVisibility,
			.parameterType		= RootParameterType::Table,
			.table				= { ranges }
		};
	}
};

inline RootTableBuilder Table;


struct StaticSamplerDesc
{
	uint32_t shaderRegister{ 0 };
	ShaderStage shaderStage{ ShaderStage::All };
	SamplerDesc samplerDesc{};
};

using StaticSamplers = std::vector<StaticSamplerDesc>;


class StaticSamplerBuilder
{
public:
	StaticSamplerDesc operator()(const SamplerDesc& samplerDesc, ShaderStage shaderStage = ShaderStage::All)
	{
		return StaticSamplerDesc{
			.shaderRegister		= APPEND_REGISTER,
			.shaderStage		= shaderStage,
			.samplerDesc		= samplerDesc
		};
	}

	StaticSamplerDesc operator()(uint32_t shaderRegister, const SamplerDesc& samplerDesc, ShaderStage shaderStage = ShaderStage::All)
	{
		return StaticSamplerDesc{
			.shaderRegister		= shaderRegister,
			.shaderStage		= shaderStage,
			.samplerDesc		= samplerDesc
		};
	}
};

inline StaticSamplerBuilder StaticSampler;


struct RootSignatureDesc
{
	std::string name{};
	RootParameters rootParameters;
	StaticSamplers staticSamplers;

	RootSignatureDesc& SetName(const std::string& value) { name = value; return *this; }
	RootSignatureDesc& SetRootParameters(const RootParameters& value) { rootParameters = value; return *this; }
	RootSignatureDesc& SetStaticSamplers(const StaticSamplers& value) { staticSamplers = value; return *this; }
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


class IRootSignature
{
public:
	virtual ~IRootSignature() = default;

	virtual DescriptorSetPtr CreateDescriptorSet(uint32_t rootParamIndex) const = 0;

	const RootSignatureDesc& GetDesc() const noexcept { return m_desc; }
	uint32_t GetNumRootParameters() const noexcept;
	const RootParameter& GetRootParameter(uint32_t index) const noexcept;

	const RootParameter& operator[](uint32_t index) const noexcept { return GetRootParameter(index); }

protected:
	RootSignatureDesc m_desc{};
};

using RootSignaturePtr = std::shared_ptr<IRootSignature>;

} // namespace Luna