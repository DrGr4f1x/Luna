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
extern "C" { __declspec(dllexport) extern const UINT D3D12SDKVersion = 615; }
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


D3D12_RESOURCE_FLAGS CombineResourceFlags(uint32_t fragmentCount)
{
	D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE;

	if (flags == D3D12_RESOURCE_FLAG_NONE && fragmentCount == 1)
	{
		flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	}

	return D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | flags;
}


wil::com_ptr<D3D12MA::Allocation> CreateStagingBuffer(D3D12MA::Allocator* allocator, const void* initialData, size_t numBytes)
{
	// Create an upload buffer
	auto resourceDesc = D3D12_RESOURCE_DESC{
		.Dimension			= D3D12_RESOURCE_DIMENSION_BUFFER,
		.Alignment			= 0,
		.Width				= numBytes,
		.Height				= 1,
		.DepthOrArraySize	= 1,
		.MipLevels			= 1,
		.Format				= DXGI_FORMAT_UNKNOWN,
		.SampleDesc			= { .Count = 1, .Quality = 0 },
		.Layout				= D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
		.Flags				= D3D12_RESOURCE_FLAG_NONE
	};

	auto allocationDesc = D3D12MA::ALLOCATION_DESC{ .HeapType = D3D12_HEAP_TYPE_UPLOAD };

	wil::com_ptr<D3D12MA::Allocation> allocation;
	HRESULT hr = allocator->CreateResource(
		&allocationDesc,
		&resourceDesc,
		D3D12_RESOURCE_STATE_COMMON,
		nullptr,
		&allocation,
		IID_NULL, nullptr);

	SetDebugName(allocation->GetResource(), "Staging Buffer");

	auto resource = allocation->GetResource();
	void* mappedPtr{ nullptr };
	assert_succeeded(resource->Map(0, nullptr, &mappedPtr));

	memcpy(mappedPtr, initialData, numBytes);

	resource->Unmap(0, nullptr);

	return allocation;
}

} // namespace Luna::DX12