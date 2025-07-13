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

#include "DirectXCommon.h"

using namespace std;


#if USE_AGILITY_SDK
extern "C" { __declspec(dllexport) extern const UINT D3D12SDKVersion = 616; }
extern "C" { __declspec(dllexport) extern const char* D3D12SDKPath = ".\\D3D12\\"; }
#endif


namespace Luna::DX12
{

#if ENABLE_D3D12_DEBUG_MARKERS
void SetDebugName(IDXGIObject* object, const string& name)
{
	object->SetPrivateData(WKPDID_D3DDebugObjectName, (UINT)name.size(), name.data());
}


void SetDebugName(ID3D12Object* object, const string& name)
{
	object->SetName(MakeWStr(name).c_str());
}
#else
void SetDebugName(IDXGIObject* object, const string& name) {}
void SetDebugName(ID3D12Object* object, const string& name) {}
#endif


D3D12_RESOURCE_FLAGS CombineResourceFlags(uint32_t sampleCount)
{
	D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE;

	if (flags == D3D12_RESOURCE_FLAG_NONE && sampleCount == 1)
	{
		flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	}

	return D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | flags;
}

} // namespace Luna::DX12