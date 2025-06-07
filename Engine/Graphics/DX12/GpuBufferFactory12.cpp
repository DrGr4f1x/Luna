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

#include "GpuBufferFactory12.h"

#include "Graphics\ResourceManager.h"


namespace Luna::DX12
{

GpuBufferFactory::GpuBufferFactory(IResourceManager* owner, ID3D12Device* device, D3D12MA::Allocator* allocator)
	: GpuBufferFactoryBase()
	, m_owner{ owner }
	, m_device{ device }
	, m_allocator{ allocator }
{
	for (uint32_t i = 0; i < MaxResources; ++i)
	{
		m_freeList.push(i);
	}

	ClearDescs();
	ClearData();
	ClearResources();
	ClearAllocations();
}


ResourceHandle GpuBufferFactory::CreateGpuBuffer(const GpuBufferDesc& gpuBufferDescIn)
{
	ResourceState initialState = ResourceState::GenericRead;

	GpuBufferDesc gpuBufferDesc = gpuBufferDescIn;
	if (gpuBufferDescIn.resourceType == ResourceType::ConstantBuffer)
	{
		gpuBufferDesc.elementSize = Math::AlignUp(gpuBufferDescIn.elementSize, 256);
	}

	wil::com_ptr<D3D12MA::Allocation> allocation = AllocateBuffer(gpuBufferDesc);
	ID3D12Resource* pResource = allocation->GetResource();

	SetDebugName(pResource, gpuBufferDesc.name);

	ResourceData resourceData{
		.resource		= pResource,
		.usageState		= ResourceState::Common
	};

	GpuBufferData gpuBufferData{
		.allocation = allocation.get()
	};

	const size_t bufferSize = gpuBufferDesc.elementCount * gpuBufferDesc.elementSize;

	if (gpuBufferDesc.resourceType == ResourceType::ByteAddressBuffer || gpuBufferDesc.resourceType == ResourceType::IndirectArgsBuffer)
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{
			.Format						= DXGI_FORMAT_R32_TYPELESS,
			.ViewDimension				= D3D12_SRV_DIMENSION_BUFFER,
			.Shader4ComponentMapping	= D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
			.Buffer = {
				.NumElements	= (uint32_t)bufferSize / 4,
				.Flags			= D3D12_BUFFER_SRV_FLAG_RAW
			}
		};

		auto srvHandle = AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		m_device->CreateShaderResourceView(pResource, &srvDesc, srvHandle);

		D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc{
			.Format			= DXGI_FORMAT_R32_TYPELESS,
			.ViewDimension	= D3D12_UAV_DIMENSION_BUFFER,
			.Buffer = {
				.NumElements	= (uint32_t)bufferSize / 4,
				.Flags			= D3D12_BUFFER_UAV_FLAG_RAW
			}
		};

		auto uavHandle = AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		m_device->CreateUnorderedAccessView(pResource, nullptr, &uavDesc, uavHandle);

		gpuBufferData.srvHandle = srvHandle;
		gpuBufferData.uavHandle = uavHandle;
	}

	if (gpuBufferDesc.resourceType == ResourceType::StructuredBuffer)
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{
			.Format						= DXGI_FORMAT_UNKNOWN,
			.ViewDimension				= D3D12_SRV_DIMENSION_BUFFER,
			.Shader4ComponentMapping	= D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
			.Buffer = {
				.NumElements			= (uint32_t)gpuBufferDesc.elementCount,
				.StructureByteStride	= (uint32_t)gpuBufferDesc.elementSize,
				.Flags					= D3D12_BUFFER_SRV_FLAG_NONE
			}
		};

		auto srvHandle = AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		m_device->CreateShaderResourceView(pResource, &srvDesc, srvHandle);

		D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc{
			.Format			= DXGI_FORMAT_UNKNOWN,
			.ViewDimension	= D3D12_UAV_DIMENSION_BUFFER,
			.Buffer = {
				.NumElements			= (uint32_t)gpuBufferDesc.elementCount,
				.StructureByteStride	= (uint32_t)gpuBufferDesc.elementSize,
				.CounterOffsetInBytes	= 0,
				.Flags					= D3D12_BUFFER_UAV_FLAG_NONE
			}
		};

		auto uavHandle = AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		m_device->CreateUnorderedAccessView(pResource, nullptr, &uavDesc, uavHandle);

		gpuBufferData.srvHandle = srvHandle;
		gpuBufferData.uavHandle = uavHandle;
	}

	if (gpuBufferDesc.resourceType == ResourceType::TypedBuffer)
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{
			.Format						= FormatToDxgi(gpuBufferDesc.format).resourceFormat,
			.ViewDimension				= D3D12_SRV_DIMENSION_BUFFER,
			.Shader4ComponentMapping	= D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
			.Buffer = {
				.NumElements	= (uint32_t)gpuBufferDesc.elementCount,
				.Flags			= D3D12_BUFFER_SRV_FLAG_NONE
			}
		};

		auto srvHandle = AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		m_device->CreateShaderResourceView(pResource, &srvDesc, srvHandle);

		D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc{
			.Format			= FormatToDxgi(gpuBufferDesc.format).resourceFormat,
			.ViewDimension	= D3D12_UAV_DIMENSION_BUFFER,
			.Buffer = {
				.NumElements	= (uint32_t)gpuBufferDesc.elementCount,
				.Flags			= D3D12_BUFFER_UAV_FLAG_NONE
			}
		};

		auto uavHandle = AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		m_device->CreateUnorderedAccessView(pResource, nullptr, &uavDesc, uavHandle);

		gpuBufferData.srvHandle = srvHandle;
		gpuBufferData.uavHandle = uavHandle;
	}

	if (gpuBufferDesc.resourceType == ResourceType::ConstantBuffer)
	{
		D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc{
			.BufferLocation		= pResource->GetGPUVirtualAddress(),
			.SizeInBytes		= (uint32_t)(gpuBufferDesc.elementCount * gpuBufferDesc.elementSize)
		};

		auto cbvHandle = AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		m_device->CreateConstantBufferView(&cbvDesc, cbvHandle);

		gpuBufferData.cbvHandle = cbvHandle;
	}

	// Create handle and store cached data
	{
		std::lock_guard lock(m_mutex);

		assert(!m_freeList.empty());

		// Get an index allocation
		uint32_t index = m_freeList.front();
		m_freeList.pop();

		m_descs[index] = gpuBufferDesc;

		m_data[index] = gpuBufferData;
		m_resources[index] = resourceData;
		m_allocations[index] = allocation;

		return make_shared<ResourceHandleType>(index, IResourceManager::ManagedGpuBuffer, m_owner);
	}
}


void GpuBufferFactory::Destroy(uint32_t index)
{
	std::lock_guard lock(m_mutex);

	ResetDesc(index);
	ResetData(index);
	ResetResource(index);
	ResetAllocation(index);

	m_freeList.push(index);
}


void GpuBufferFactory::Update(uint32_t index, size_t sizeInBytes, size_t offset, const void* data) const
{
	assert((sizeInBytes + offset) <= GetSize(index));
	assert(HasFlag(m_descs[index].memoryAccess, MemoryAccess::CpuWrite));

	CD3DX12_RANGE readRange(0, 0);

	ID3D12Resource* resource = GetResource(index);

	// Map uniform buffer and update it
	uint8_t* pData = nullptr;
	ThrowIfFailed(resource->Map(0, &readRange, reinterpret_cast<void**>(&pData)));
	memcpy((void*)(pData + offset), data, sizeInBytes);
	// Unmap after data has been copied
	// Note: Since we requested a host coherent memory type for the uniform buffer, the write is instantly visible to the GPU
	resource->Unmap(0, nullptr);
}


wil::com_ptr<D3D12MA::Allocation> GpuBufferFactory::AllocateBuffer(const GpuBufferDesc& gpuBufferDesc) const
{
	const UINT64 bufferSize = gpuBufferDesc.elementSize * gpuBufferDesc.elementCount;

	D3D12_HEAP_TYPE heapType = GetHeapType(gpuBufferDesc.memoryAccess);
	D3D12_RESOURCE_FLAGS flags = (gpuBufferDesc.bAllowUnorderedAccess || IsUnorderedAccessType(gpuBufferDesc.resourceType))
		? D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS
		: D3D12_RESOURCE_FLAG_NONE;


	D3D12_RESOURCE_DESC resourceDesc{
		.Dimension			= D3D12_RESOURCE_DIMENSION_BUFFER,
		.Alignment			= 0,
		.Width				= bufferSize,
		.Height				= 1,
		.DepthOrArraySize	= 1,
		.MipLevels			= 1,
		.Format				= FormatToDxgi(gpuBufferDesc.format).resourceFormat,
		.SampleDesc			= { .Count = 1, .Quality = 0 },
		.Layout				= D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
		.Flags				= flags
	};

	auto allocationDesc = D3D12MA::ALLOCATION_DESC{
		.HeapType = heapType
	};

	wil::com_ptr<D3D12MA::Allocation> allocation;
	HRESULT hr = m_allocator->CreateResource(
		&allocationDesc,
		&resourceDesc,
		D3D12_RESOURCE_STATE_COMMON,
		NULL,
		&allocation,
		IID_NULL, NULL);

	return allocation;
}


void GpuBufferFactory::ResetData(uint32_t index)
{
	m_data[index] = GpuBufferData{};
}


void GpuBufferFactory::ResetResource(uint32_t index)
{
	m_resources[index].resource.reset();
	m_resources[index].usageState = ResourceState::Undefined;
}


void GpuBufferFactory::ResetAllocation(uint32_t index)
{
	m_allocations[index].reset();
}


void GpuBufferFactory::ClearData()
{
	for (uint32_t i = 0; i < MaxResources; ++i)
	{
		ResetData(i);
	}
}


void GpuBufferFactory::ClearResources()
{
	for (uint32_t i = 0; i < MaxResources; ++i)
	{
		ResetResource(i);
	}
}


void GpuBufferFactory::ClearAllocations()
{
	for (uint32_t i = 0; i < MaxResources; ++i)
	{
		ResetAllocation(i);
	}
}

} // namespace Luna::DX12