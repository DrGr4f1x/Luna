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
struct ColorBufferDesc;
struct DepthBufferDesc;
struct GpuBufferDesc;
struct RootSignatureDesc;
class IPlatformData;
enum class ResourceState : uint32_t;


class __declspec(uuid("DBECDD70-7F0B-4C9B-ADFA-048104E474C8")) IGraphicsDevice : public IUnknown
{
public:
	virtual ~IGraphicsDevice() = default;

	virtual wil::com_ptr<IPlatformData> CreateColorBufferData(ColorBufferDesc& desc, ResourceState& initialState) = 0;
	virtual wil::com_ptr<IPlatformData> CreateDepthBufferData(DepthBufferDesc& desc, ResourceState& initialState) = 0;
	virtual wil::com_ptr<IPlatformData> CreateGpuBufferData(GpuBufferDesc& desc, ResourceState& initialState) = 0;
	virtual wil::com_ptr<IPlatformData> CreateRootSignatureData(RootSignatureDesc& desc) = 0;
};

} // namespace Luna
