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

RootSignatureVK::RootSignatureVK(const RootSignatureDesc& rootSignatureDesc, const RootSignatureDescExt& rootSignatureDescExt)
	: m_desc{ rootSignatureDesc }
	, m_pipelineLayout{ rootSignatureDescExt.pipelineLayout }
	, m_descriptorSetLayouts{ rootSignatureDescExt.descriptorSetLayouts }
{}


NativeObjectPtr RootSignatureVK::GetNativeObject(NativeObjectType type) const noexcept
{
	using enum NativeObjectType;

	switch (type)
	{
	case VK_PipelineLayout:
		return NativeObjectPtr(m_pipelineLayout->Get());

	default:
		assert(false);
		return nullptr;
	}
}


uint32_t RootSignatureVK::GetNumRootParameters() const noexcept
{
	return (uint32_t)m_desc.rootParameters.size();
}


const RootParameter& RootSignatureVK::GetRootParameter(uint32_t index) const noexcept
{
	return m_desc.rootParameters[index];
}

} // namespace Luna::VK