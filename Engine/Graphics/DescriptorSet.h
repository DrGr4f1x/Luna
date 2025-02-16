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

namespace Luna
{

// Forward declarations
class ColorBuffer;
class DepthBuffer;
class GpuBuffer;
class IDescriptorSetPool;
class RootSignature;


class __declspec(uuid("BDE2324D-CF9C-4489-9612-4C5DE87F88B6")) DescriptorSetHandleType : public RefCounted<DescriptorSetHandleType>
{
public:
	DescriptorSetHandleType(uint32_t index, IDescriptorSetPool* pool)
		: m_index{ index }
		, m_pool{ pool }
	{}
	~DescriptorSetHandleType();

	uint32_t GetIndex() const { return m_index; }

private:
	uint32_t m_index{ 0 };
	IDescriptorSetPool* m_pool{ nullptr };
};

using DescriptorSetHandle = wil::com_ptr<DescriptorSetHandleType>;


class IDescriptorSetPool
{
public:
	// Destroy DescriptorSet.  Creation is implemented per-platform.
	virtual void DestroyHandle(DescriptorSetHandleType* handle) = 0;

	// Platform agnostic functions
	// TODO: change 'int slot' to uint32_t
	virtual void SetSRV(DescriptorSetHandleType* handle, int slot, const ColorBuffer& colorBuffer) = 0;
	virtual void SetSRV(DescriptorSetHandleType* handle, int slot, const DepthBuffer& depthBuffer, bool depthSrv = true) = 0;
	virtual void SetSRV(DescriptorSetHandleType* handle, int slot, const GpuBuffer& gpuBuffer) = 0;

	virtual void SetUAV(DescriptorSetHandleType* handle, int slot, const ColorBuffer& colorBuffer, uint32_t uavIndex = 0) = 0;
	virtual void SetUAV(DescriptorSetHandleType* handle, int slot, const DepthBuffer& depthBuffer) = 0;
	virtual void SetUAV(DescriptorSetHandleType* handle, int slot, const GpuBuffer& gpuBuffer) = 0;

	virtual void SetCBV(DescriptorSetHandleType* handle, int slot, const GpuBuffer& gpuBuffer) = 0;

	virtual void SetDynamicOffset(DescriptorSetHandleType* handle, uint32_t offset) = 0;

	virtual void UpdateGpuDescriptors(DescriptorSetHandleType* handle) = 0;
};


class DescriptorSet
{
public:
	void Initialize(const RootSignature& rootSignature, uint32_t rootParamIndex);

	// TODO: change 'int slot' to uint32_t
	void SetSRV(int slot, const ColorBuffer& colorBuffer);
	void SetSRV(int slot, const DepthBuffer& depthBuffer, bool depthSrv = true);
	void SetSRV(int slot, const GpuBuffer& gpuBuffer);

	void SetUAV(int slot, const ColorBuffer& colorBuffer, uint32_t uavIndex = 0);
	void SetUAV(int slot, const DepthBuffer& depthBuffer);
	void SetUAV(int slot, const GpuBuffer& gpuBuffer);

	void SetCBV(int slot, const GpuBuffer& gpuBuffer);

	void SetDynamicOffset(uint32_t offset);

	DescriptorSetHandle GetHandle() const { return m_handle; }

private:
	DescriptorSetHandle m_handle;
};

} // namespace Luna