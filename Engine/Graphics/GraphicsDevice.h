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

// Forward declarations (new)
class IColorBufferPool;
class IDepthBufferPool;
class IDescriptorSetPool;
class IGpuBufferPool;
class IPipelineStatePool;
class IRootSignaturePool;


class __declspec(uuid("DBECDD70-7F0B-4C9B-ADFA-048104E474C8")) IGraphicsDevice : public IUnknown
{
public:
	virtual ~IGraphicsDevice() = default;

	virtual IColorBufferPool* GetColorBufferPool() = 0;
	virtual IDepthBufferPool* GetDepthBufferPool() = 0;
	virtual IDescriptorSetPool* GetDescriptorSetPool() = 0;
	virtual IGpuBufferPool* GetGpuBufferPool() = 0;
	virtual IPipelineStatePool* GetPipelineStatePool() = 0;
	virtual IRootSignaturePool* GetRootSignaturePool() = 0;
};

} // namespace Luna
