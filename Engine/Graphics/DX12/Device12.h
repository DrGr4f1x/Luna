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

#include "Graphics\GraphicsDevice.h"
#include "Graphics\Shader.h"
#include "Graphics\DX12\DirectXCommon.h"

using namespace Microsoft::WRL;


namespace Luna::DX12
{

// Forward declarations
class DescriptorAllocator;
class Queue;
struct DeviceCaps;


struct DeviceRLDOHelper
{
	ID3D12Device* device{ nullptr };
	const bool doReport{ false };

	DeviceRLDOHelper(ID3D12Device* device, bool bDoReport) noexcept
		: device{ device }
		, doReport{ bDoReport }
	{
	}

	~DeviceRLDOHelper();
};


struct GraphicsDeviceDesc
{
	IDXGIFactory4* dxgiFactory{ nullptr };
	ID3D12Device* dx12Device{ nullptr };
	D3D12MA::Allocator* d3d12maAllocator{ nullptr };

	uint32_t backBufferWidth{ 0 };
	uint32_t backBufferHeight{ 0 };
	uint32_t numSwapChainBuffers{ 3 };
	Format swapChainFormat{ Format::Unknown };
	uint32_t swapChainSampleCount{ 1 };
	uint32_t swapChainSampleQuality{ 0 };
	bool allowModeSwitch{ false };
	bool isTearingSupported{ false };
	bool allowHDROutput{ false };

	bool enableVSync{ false };
	uint32_t maxFramesInFlight{ 2 };

	HWND hwnd{ nullptr };

#if ENABLE_D3D12_VALIDATION
	bool enableValidation{ true };
#else
	bool enableValidation{ false };
#endif

#if ENABLE_D3D12_DEBUG_MARKERS
	bool enableDebugMarkers{ true };
#else
	bool enableDebugMarkers{ false };
#endif

	constexpr GraphicsDeviceDesc& SetDxgiFactory(IDXGIFactory4* value) noexcept { dxgiFactory = value; return *this; }
	constexpr GraphicsDeviceDesc& SetDevice(ID3D12Device* value) noexcept { dx12Device = value; return *this; }
	constexpr GraphicsDeviceDesc& SetBackBufferWidth(uint32_t value) noexcept { backBufferWidth = value; return *this; }
	constexpr GraphicsDeviceDesc& SetBackBufferHeight(uint32_t value) noexcept { backBufferHeight = value; return *this; }
	constexpr GraphicsDeviceDesc& SetNumSwapChainBuffers(uint32_t value) noexcept { numSwapChainBuffers = value; return *this; }
	constexpr GraphicsDeviceDesc& SetSwapChainFormat(Format value) noexcept { swapChainFormat = value; return *this; }
	constexpr GraphicsDeviceDesc& SetSwapChainSampleCount(uint32_t value) noexcept { swapChainSampleCount = value; return*this; }
	constexpr GraphicsDeviceDesc& SetSwapChainSampleQuality(uint32_t value) noexcept { swapChainSampleQuality = value; return *this; }
	constexpr GraphicsDeviceDesc& SetAllowModeSwitch(bool value) noexcept { allowModeSwitch = value; return *this; }
	constexpr GraphicsDeviceDesc& SetIsTearingSupported(bool value) noexcept { isTearingSupported = value; return *this; }
	constexpr GraphicsDeviceDesc& SetAllowHDROutput(bool value) noexcept { allowHDROutput = value; return *this; }
	constexpr GraphicsDeviceDesc& SetEnableVSync(bool value) noexcept { enableVSync = value; return *this; }
	constexpr GraphicsDeviceDesc& SetMaxFramesInFlight(uint32_t value) noexcept { maxFramesInFlight = value; return *this; }
	constexpr GraphicsDeviceDesc& SetHwnd(HWND value) noexcept { hwnd = value; return *this; }
	constexpr GraphicsDeviceDesc& SetEnableValidation(bool value) noexcept { enableValidation = value; return *this; }
	constexpr GraphicsDeviceDesc& SetEnableDebugMarkers(bool value) noexcept { enableDebugMarkers = value; return *this; }
};


class __declspec(uuid("017DADC6-170C-4F84-AB6A-CA0938AB6A3F")) GraphicsDevice 
	: public RuntimeClass<RuntimeClassFlags<ClassicCom>, Luna::IGraphicsDevice>
	, public NonCopyable
{
	friend class CommandContext12;
	friend class DeviceManager;
	friend class GpuBufferData;

public:
	GraphicsDevice(const GraphicsDeviceDesc& desc) noexcept;
	virtual ~GraphicsDevice();

	// GraphicsDevice implementation
	ColorBufferHandle CreateColorBuffer(const ColorBufferDesc& colorBufferDesc) override;
	DepthBufferHandle CreateDepthBuffer(const DepthBufferDesc& depthBufferDesc) override;
	GpuBufferHandle CreateGpuBuffer(const GpuBufferDesc& gpuBufferDesc) override;
	RootSignatureHandle CreateRootSignature(const RootSignatureDesc& rootSignatureDesc) override;

	void CreateResources();

private:
	ID3D12Device* GetD3D12Device() { return m_dxDevice.get(); }
	D3D12MA::Allocator* GetAllocator() { return m_d3d12maAllocator.get(); }

	void InstallDebugCallback();
	void ReadCaps();

	D3D12_CPU_DESCRIPTOR_HANDLE AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t count = 1);
	wil::com_ptr<D3D12MA::Allocation> CreateGpuBuffer(const GpuBufferDesc& desc, ResourceState& initialState);
	wil::com_ptr<D3D12MA::Allocation> CreateStagingBuffer(const void* initialData, size_t numBytes) const;

	// Texture formats
	uint8_t GetFormatPlaneCount(DXGI_FORMAT format);

private:
	GraphicsDeviceDesc m_desc{};

	// DirectX 12 objects
	wil::com_ptr<ID3D12Device> m_dxDevice;
	DeviceRLDOHelper m_deviceRLDOHelper;
	wil::com_ptr<IDXGIFactory4> m_dxgiFactory;
	wil::com_ptr<ID3D12InfoQueue1> m_dxInfoQueue;
	wil::com_ptr<D3D12MA::Allocator> m_d3d12maAllocator;
	DWORD m_callbackCookie{ 0 };

	// Descriptor allocators
	std::array<std::unique_ptr<DescriptorAllocator>, D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES> m_descriptorAllocators;

	// DirectX caps
	std::unique_ptr<DeviceCaps> m_caps;

	// Format properties
	std::unordered_map<DXGI_FORMAT, uint8_t> m_dxgiFormatPlaneCounts;

	// Root signatures
	std::mutex m_rootSignatureMutex;
	std::map<size_t, wil::com_ptr<ID3D12RootSignature>> m_rootSignatureHashMap;

	// Pipeline states
	std::mutex m_pipelineStateMutex;
	std::map<size_t, wil::com_ptr<ID3D12PipelineState>> m_pipelineStateMap;

	// Shaders
	std::mutex m_shaderMutex;
	std::map<size_t, wil::com_ptr<IShaderData>> m_shaderMap;
};

} // namespace Luna::DX12