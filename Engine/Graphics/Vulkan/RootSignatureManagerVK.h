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

#include "Graphics\RootSignature.h"
#include "Graphics\Vulkan\VulkanCommon.h"


namespace Luna::VK
{

struct RootSignatureData
{
	wil::com_ptr<CVkPipelineLayout> pipelineLayout;
	std::vector<wil::com_ptr<CVkDescriptorSetLayout>> descriptorSetLayouts;
};


class RootSignatureManager : public IRootSignatureManager
{
	static const uint32_t MaxItems = (1 << 12);

public:
	explicit RootSignatureManager(CVkDevice* device);
	~RootSignatureManager();

	// Create/Destroy pipeline state
	RootSignatureHandle CreateRootSignature(const RootSignatureDesc& rootSignatureDesc) override;
	void DestroyHandle(RootSignatureHandleType* handle) override;

	// Platform agnostic functions
	const RootSignatureDesc& GetDesc(const RootSignatureHandleType* handle) const override;
	uint32_t GetNumRootParameters(const RootSignatureHandleType* handle) const override;
	wil::com_ptr<DescriptorSetHandleType> CreateDescriptorSet(RootSignatureHandleType* handle, uint32_t index) const;

	// Getters
	VkPipelineLayout GetPipelineLayout(RootSignatureHandleType* handle) const;

private:
	const RootSignatureData& GetData(RootSignatureHandleType* handle) const;

	RootSignatureData FindOrCreateRootSignatureData(const RootSignatureDesc& rootSignatureDesc);

private:
	wil::com_ptr<CVkDevice> m_device;

	// Allocation mutex
	std::mutex m_allocationMutex;

	// Free list
	std::queue<uint32_t> m_freeList;

	// Hot data: RootSignatureData
	std::array<RootSignatureData, MaxItems> m_rootSignatureData;

	// Cold data: RootSignatureDesc
	std::array<RootSignatureDesc, MaxItems> m_descs;

	// Root signatures
	std::mutex m_pipelineLayoutMutex;
	std::map<size_t, wil::com_ptr<CVkPipelineLayout>> m_pipelineLayoutHashMap;
};


RootSignatureManager* const GetVulkanRootSignatureManager();

} // namespace Luna::VK