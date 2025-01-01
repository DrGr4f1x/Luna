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

#include "ColorBuffer12.h"


namespace Luna::DX12
{

ColorBufferData::ColorBufferData(const ColorBufferDescExt& descExt)
	: m_resource{ descExt.resource }
	, m_srvHandle{ descExt.srvHandle }
	, m_rtvHandle{ descExt.rtvHandle }
	, m_uavHandles{ descExt.uavHandles }
{
}

} // namespace Luna::DX12