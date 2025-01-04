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

#include "GpuBuffer12.h"

namespace Luna::DX12
{

IndexBufferData::IndexBufferData(const IndexBufferDescExt& descExt)
	: m_resource{ descExt.resource }
	, m_ibvHandle{ descExt.ibvHandle }
{
}

} // namespace Luna::DX12