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

#include "Graphics\QueryHeap.h"
#include "Graphics\Vulkan\VulkanCommon.h"


namespace Luna::VK
{

// Forward declaration
class Device;


class QueryHeap : public IQueryHeap
{
	friend class Device;

public:
	VkQueryPool GetQueryPool() const { return m_pool->Get(); }

protected:
	Device* m_device{ nullptr };
	wil::com_ptr<CVkQueryPool> m_pool;
};

} // namespace Luna::VK