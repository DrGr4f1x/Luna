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

NativeObjectPtr ColorBuffer::GetNativeObject(NativeObjectType nativeObjectType) const noexcept
{
	if (nativeObjectType == NativeObjectType::DX12_Resource)
	{

	}

	return IGpuImage::GetNativeObject(nativeObjectType);
}

} // namespace Luna::DX12