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

#include "RootSignaturePoolVK.h"

#include "DescriptorAllocatorVK.h"
#include "DescriptorSetPoolVK.h"


namespace Luna::VK
{

RootSignaturePool* g_rootSignaturePool{ nullptr };


RootSignaturePool::RootSignaturePool(CVkDevice* device)
	: m_device{ device }
{
	assert(g_rootSignaturePool == nullptr);

	// Populate free list and data arrays
	for (uint32_t i = 0; i < MaxItems; ++i)
	{
		m_freeList.push(i);
		m_rootSignatureData[i] = RootSignatureData{};
		m_descs[i] = RootSignatureDesc{};
	}

	g_rootSignaturePool = this;
}


RootSignaturePool::~RootSignaturePool()
{
	g_rootSignaturePool = nullptr;
}


RootSignatureHandle RootSignaturePool::CreateRootSignature(const RootSignatureDesc& rootSignatureDesc)
{
	std::lock_guard guard(m_allocationMutex);

	assert(!m_freeList.empty());

	uint32_t index = m_freeList.front();
	m_freeList.pop();

	m_descs[index] = rootSignatureDesc;

	m_rootSignatureData[index] = FindOrCreateRootSignatureData(rootSignatureDesc);

	return Create<RootSignatureHandleType>(index, this);
}


void RootSignaturePool::DestroyHandle(RootSignatureHandleType* handle)
{
	assert(handle != nullptr);

	std::lock_guard guard(m_allocationMutex);

	// TODO: queue this up to execute in one big batch, e.g. at the end of the frame

	uint32_t index = handle->GetIndex();
	m_descs[index] = RootSignatureDesc{};
	m_rootSignatureData[index] = RootSignatureData{};

	m_freeList.push(index);
}


const RootSignatureDesc& RootSignaturePool::GetDesc(const RootSignatureHandleType* handle) const
{
	assert(handle != nullptr);

	uint32_t index = handle->GetIndex();
	return m_descs[index];
}


uint32_t RootSignaturePool::GetNumRootParameters(const RootSignatureHandleType* handle) const
{
	return (uint32_t)GetDesc(handle).rootParameters.size();
}


DescriptorSetHandle RootSignaturePool::CreateDescriptorSet(RootSignatureHandleType* handle, uint32_t index) const
{
	const auto& rootSignatureDesc = GetDesc(handle);
	const auto& data = GetData(handle);

	VkDescriptorSet descriptorSet = AllocateDescriptorSet(data.descriptorSetLayouts[index]->Get());
	const auto& rootParam = rootSignatureDesc.rootParameters[index];

	const bool isDynamicBuffer = rootParam.parameterType == RootParameterType::RootCBV ||
		rootParam.parameterType == RootParameterType::RootSRV ||
		rootParam.parameterType == RootParameterType::RootUAV;

	DescriptorSetDesc descriptorSetDesc{
		.descriptorSet		= descriptorSet,
		.bindingOffsets		= rootSignatureDesc.bindingOffsets,
		.numDescriptors		= rootParam.GetNumDescriptors(),
		.isDynamicBuffer	= isDynamicBuffer
	};

	return GetVulkanDescriptorSetPool()->CreateDescriptorSet(descriptorSetDesc);
}


VkPipelineLayout RootSignaturePool::GetPipelineLayout(RootSignatureHandleType* handle) const
{
	assert(handle != nullptr);

	uint32_t index = handle->GetIndex();
	return m_rootSignatureData[index].pipelineLayout->Get();
}


const RootSignatureData& RootSignaturePool::GetData(RootSignatureHandleType* handle) const
{
	assert(handle != nullptr);

	uint32_t index = handle->GetIndex();
	return m_rootSignatureData[index];
}


RootSignatureData RootSignaturePool::FindOrCreateRootSignatureData(const RootSignatureDesc& rootSignatureDesc)
{
	// Validate RootSignatureDesc
	if (!rootSignatureDesc.Validate())
	{
		LogError(LogVulkan) << "RootSignature is not valid!" << endl;
		return RootSignatureData{};
	}

	std::vector<VkDescriptorSetLayout> vkDescriptorSetLayouts;
	std::vector<wil::com_ptr<CVkDescriptorSetLayout>> descriptorSetLayouts;
	std::vector<VkPushConstantRange> vkPushConstantRanges;
	uint32_t pushConstantOffset{ 0 };

	size_t hashCode = Utility::g_hashStart;

	for (const auto& rootParameter : rootSignatureDesc.rootParameters)
	{
		VkShaderStageFlags shaderStageFlags = ShaderStageToVulkan(rootParameter.shaderVisibility);

		if (rootParameter.parameterType == RootParameterType::RootConstants)
		{
			VkPushConstantRange& vkPushConstantRange = vkPushConstantRanges.emplace_back();
			vkPushConstantRange.offset = pushConstantOffset;
			vkPushConstantRange.size = rootParameter.num32BitConstants * 4;
			vkPushConstantRange.stageFlags = shaderStageFlags;
			pushConstantOffset += rootParameter.num32BitConstants * 4;

			hashCode = Utility::HashState(&vkPushConstantRange, 1, hashCode);
		}
		else if (rootParameter.parameterType == RootParameterType::RootCBV)
		{
			VkDescriptorSetLayoutBinding vkBinding{
				.binding = rootParameter.startRegister + rootSignatureDesc.bindingOffsets.constantBuffer,
				.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
				.descriptorCount = 1,
				.stageFlags = shaderStageFlags,
				.pImmutableSamplers = nullptr
			};

			hashCode = Utility::HashState(&vkBinding, 1, hashCode);

			VkDescriptorSetLayoutCreateInfo createInfo{
				.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
				.bindingCount = 1,
				.pBindings = &vkBinding
			};

			VkDescriptorSetLayout vkDescriptorSetLayout{ VK_NULL_HANDLE };
			vkCreateDescriptorSetLayout(*m_device, &createInfo, nullptr, &vkDescriptorSetLayout);
			vkDescriptorSetLayouts.push_back(vkDescriptorSetLayout);

			wil::com_ptr<CVkDescriptorSetLayout> descriptorSetLayout = Create<CVkDescriptorSetLayout>(m_device.get(), vkDescriptorSetLayout);
			descriptorSetLayouts.push_back(descriptorSetLayout);
		}
		else if (rootParameter.parameterType == RootParameterType::RootSRV)
		{
			VkDescriptorSetLayoutBinding vkBinding{
				.binding = rootParameter.startRegister + rootSignatureDesc.bindingOffsets.shaderResource,
				.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC,
				.descriptorCount = 1,
				.stageFlags = shaderStageFlags,
				.pImmutableSamplers = nullptr
			};

			hashCode = Utility::HashState(&vkBinding, 1, hashCode);

			VkDescriptorSetLayoutCreateInfo createInfo{
				.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
				.bindingCount = 1,
				.pBindings = &vkBinding
			};

			VkDescriptorSetLayout vkDescriptorSetLayout{ VK_NULL_HANDLE };
			vkCreateDescriptorSetLayout(*m_device, &createInfo, nullptr, &vkDescriptorSetLayout);
			vkDescriptorSetLayouts.push_back(vkDescriptorSetLayout);

			wil::com_ptr<CVkDescriptorSetLayout> descriptorSetLayout = Create<CVkDescriptorSetLayout>(m_device.get(), vkDescriptorSetLayout);
			descriptorSetLayouts.push_back(descriptorSetLayout);
		}
		else if (rootParameter.parameterType == RootParameterType::RootUAV)
		{
			VkDescriptorSetLayoutBinding vkBinding{
				.binding = rootParameter.startRegister + rootSignatureDesc.bindingOffsets.unorderedAccess,
				.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC,
				.descriptorCount = 1,
				.stageFlags = shaderStageFlags,
				.pImmutableSamplers = nullptr
			};

			hashCode = Utility::HashState(&vkBinding, 1, hashCode);

			VkDescriptorSetLayoutCreateInfo createInfo{
				.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
				.bindingCount = 1,
				.pBindings = &vkBinding
			};

			VkDescriptorSetLayout vkDescriptorSetLayout{ VK_NULL_HANDLE };
			vkCreateDescriptorSetLayout(*m_device, &createInfo, nullptr, &vkDescriptorSetLayout);
			vkDescriptorSetLayouts.push_back(vkDescriptorSetLayout);

			wil::com_ptr<CVkDescriptorSetLayout> descriptorSetLayout = Create<CVkDescriptorSetLayout>(m_device.get(), vkDescriptorSetLayout);
			descriptorSetLayouts.push_back(descriptorSetLayout);
		}
		else if (rootParameter.parameterType == RootParameterType::Table)
		{
			std::vector<VkDescriptorSetLayoutBinding> vkLayoutBindings;

			for (const auto& range : rootParameter.table)
			{
				VkDescriptorSetLayoutBinding& vkBinding = vkLayoutBindings.emplace_back();
				vkBinding.descriptorCount = range.numDescriptors;
				vkBinding.descriptorType = DescriptorTypeToVulkan(range.descriptorType);
				vkBinding.stageFlags = shaderStageFlags;
				vkBinding.pImmutableSamplers = nullptr;

				uint32_t offset{ 0 };

				switch (range.descriptorType)
				{
				case DescriptorType::TextureSRV:
				case DescriptorType::StructuredBufferSRV:
				case DescriptorType::TypedBufferSRV:
				case DescriptorType::RawBufferSRV:
				case DescriptorType::RayTracingAccelStruct:
					offset = rootSignatureDesc.bindingOffsets.shaderResource;
					break;

				case DescriptorType::TextureUAV:
				case DescriptorType::StructuredBufferUAV:
				case DescriptorType::TypedBufferUAV:
				case DescriptorType::RawBufferUAV:
				case DescriptorType::SamplerFeedbackTextureUAV:
					offset = rootSignatureDesc.bindingOffsets.unorderedAccess;
					break;

				case DescriptorType::ConstantBuffer:
					offset = rootSignatureDesc.bindingOffsets.constantBuffer;
					break;

				case DescriptorType::Sampler:
					offset = rootSignatureDesc.bindingOffsets.sampler;
					break;
				}

				vkBinding.binding = range.startRegister + offset;

				hashCode = Utility::HashState(&vkBinding, 1, hashCode);
			}

			VkDescriptorSetLayoutCreateInfo createInfo{
				.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
				.bindingCount = (uint32_t)vkLayoutBindings.size(),
				.pBindings = vkLayoutBindings.data()
			};

			VkDescriptorSetLayout vkDescriptorSetLayout{ VK_NULL_HANDLE };
			vkCreateDescriptorSetLayout(*m_device, &createInfo, nullptr, &vkDescriptorSetLayout);
			vkDescriptorSetLayouts.push_back(vkDescriptorSetLayout);

			wil::com_ptr<CVkDescriptorSetLayout> descriptorSetLayout = Create<CVkDescriptorSetLayout>(m_device.get(), vkDescriptorSetLayout);
			descriptorSetLayouts.push_back(descriptorSetLayout);
		}
	}

	CVkPipelineLayout** ppPipelineLayout;
	bool firstCompile = false;
	{
		lock_guard<mutex> CS(m_pipelineLayoutMutex);

		auto iter = m_pipelineLayoutHashMap.find(hashCode);

		// Reserve space so the next inquiry will find that someone got here first.
		if (iter == m_pipelineLayoutHashMap.end())
		{
			ppPipelineLayout = m_pipelineLayoutHashMap[hashCode].addressof();
			firstCompile = true;
		}
		else
		{
			ppPipelineLayout = &iter->second;
		}
	}

	if (firstCompile)
	{
		VkPipelineLayoutCreateInfo createInfo{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
			.setLayoutCount = (uint32_t)vkDescriptorSetLayouts.size(),
			.pSetLayouts = vkDescriptorSetLayouts.data(),
			.pushConstantRangeCount = (uint32_t)vkPushConstantRanges.size(),
			.pPushConstantRanges = vkPushConstantRanges.data()
		};

		VkPipelineLayout vkPipelineLayout{ VK_NULL_HANDLE };
		vkCreatePipelineLayout(*m_device, &createInfo, nullptr, &vkPipelineLayout);

		wil::com_ptr<CVkPipelineLayout> pipelineLayout = Create<CVkPipelineLayout>(m_device.get(), vkPipelineLayout);

		*ppPipelineLayout = pipelineLayout.get();

		(*ppPipelineLayout)->AddRef();
	}

	RootSignatureData rootSignatureData{
		.pipelineLayout = *ppPipelineLayout,
		.descriptorSetLayouts = descriptorSetLayouts
	};

	return rootSignatureData;
}


RootSignaturePool* const GetVulkanRootSignaturePool()
{
	return g_rootSignaturePool;
}

} // namespace Luna::VK