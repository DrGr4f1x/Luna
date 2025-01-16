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

#include "RootSignature12.h"


namespace Luna::DX12
{

RootSignatureData::RootSignatureData(const RootSignatureDescExt& descExt)
	: m_rootSignature{ descExt.rootSignature }
	, m_descriptorTableBitmap{ descExt.descriptorTableBitmap }
	, m_samplerTableBitmap{ descExt.samplerTableBitmap }
	, m_descriptorTableSize{ descExt.descriptorTableSize }
{}

} // namespace Luna::DX12