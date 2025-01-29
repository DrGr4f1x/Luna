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

#include "Graphics\PipelineState.h"
#include "Graphics\DX12\DirectXCommon.h"

using namespace Microsoft::WRL;


namespace Luna::DX12
{

struct GraphicsPSODescExt
{
	ID3D12PipelineState* pipelineState{ nullptr };
};


class __declspec(uuid("BF47E4F0-2BB3-44E6-A076-8FF9F4C0A061")) GraphicsPSO12 final
	: public RuntimeClass<RuntimeClassFlags<ClassicCom>, IGraphicsPSO>
	, NonCopyable
{
public:
	GraphicsPSO12(const GraphicsPSODesc& graphicsPSODesc, const GraphicsPSODescExt& graphicsPSODescExt);

	NativeObjectPtr GetNativeObject() const noexcept override;

private:
	wil::com_ptr<ID3D12PipelineState> m_pipelineState;
};

} // namespace Luna::DX12