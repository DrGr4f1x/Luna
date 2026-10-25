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


namespace Luna
{

class IDescriptorHeap
{
public:

	DescriptorHeapType GetType() const { return m_type; }

protected:
	DescriptorHeapType m_type;
};

using DescriptorHeapPtr = std::shared_ptr<IDescriptorHeap>;

} // namespace Luna