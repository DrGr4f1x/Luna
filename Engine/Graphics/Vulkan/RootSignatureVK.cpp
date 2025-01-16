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

#include "RootSignatureVK.h"


namespace Luna::VK
{

RootSignatureData::RootSignatureData(const RootSignatureDescExt& descExt)
	: m_pipelineLayout{ descExt.pipelineLayout }
	, m_descriptorSetLayouts{ descExt.descriptorSetLayouts }
{}

} // namespace Luna::VK