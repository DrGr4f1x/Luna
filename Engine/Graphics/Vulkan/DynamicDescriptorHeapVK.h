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

#include "Graphics\Vulkan\VulkanCommon.h"


namespace Luna
{

class DescriptorSetPool;
class RootSignature;

} // namespace Luna


namespace Luna::VK
{

class IDynamicDescriptorHeap
{
public:
	virtual void CleanupUsedPools(uint64_t fenceValue) = 0;

	virtual void ParseGraphicsRootSignature(const RootSignature& rootSignature) = 0;
	virtual void ParseComputeRootSignature(const RootSignature& rootSignature) = 0;
};


class DefaultDynamicDescriptorHeap : public IDynamicDescriptorHeap
{
public:
	void CleanupUsedPools(uint64_t fenceValue) override;

private:
	void RetireCurrentPools();
	void RetireUsedPools(uint64_t fenceValue);

private:
	// Static members
	static std::mutex sm_mutex;
	static std::queue<std::pair<uint64_t, std::shared_ptr<DescriptorSetPool>>> sm_retiredDescriptorPools;

	// Non-static members
	std::array<std::shared_ptr<DescriptorSetPool>, MaxRootParameters> m_descriptorPools;
	std::vector<std::shared_ptr<DescriptorSetPool>> m_retiredPools;
};

} // namespace Luna::VK