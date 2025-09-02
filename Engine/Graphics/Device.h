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

#include "Graphics\GraphicsCommon.h"

#include "Graphics\ColorBuffer.h"
#include "Graphics\DepthBuffer.h"
#include "Graphics\DescriptorSet.h"
#include "Graphics\GpuBuffer.h"
#include "Graphics\PipelineState.h"
#include "Graphics\QueryHeap.h"
#include "Graphics\RootSignature.h"
#include "Graphics\Sampler.h"
#include "Graphics\Texture.h"


namespace Luna
{

// Forward declarations
struct DeviceCaps;


class IDevice
{
	friend class TextureManager;

public:
	virtual ~IDevice() = default;

	virtual const DeviceCaps& GetDeviceCaps() const = 0;

	virtual ColorBufferPtr CreateColorBuffer(const ColorBufferDesc& colorBufferDesc) = 0;
	virtual DepthBufferPtr CreateDepthBuffer(const DepthBufferDesc& depthBufferDesc) = 0;
	virtual GpuBufferPtr CreateGpuBuffer(const GpuBufferDesc& gpuBufferDesc) = 0;

	virtual RootSignaturePtr CreateRootSignature(const RootSignatureDesc& rootSignatureDesc) = 0;

	virtual GraphicsPipelinePtr CreateGraphicsPipeline(const GraphicsPipelineDesc& pipelineDesc) = 0;
	virtual ComputePipelinePtr CreateComputePipeline(const ComputePipelineDesc& pipelineDesc) = 0;
	virtual MeshletPipelinePtr CreateMeshletPipeline(const MeshletPipelineDesc& pipelineDesc) = 0;

	virtual QueryHeapPtr CreateQueryHeap(const QueryHeapDesc& queryHeapDesc) = 0;

	virtual SamplerPtr CreateSampler(const SamplerDesc& samplerDesc) = 0;

	virtual TexturePtr CreateTexture1D(const TextureDesc& textureDesc) = 0;
	virtual TexturePtr CreateTexture2D(const TextureDesc& textureDesc) = 0;
	virtual TexturePtr CreateTexture3D(const TextureDesc& textureDesc) = 0;

	virtual ITexture* CreateUninitializedTexture(const std::string& name, const std::string& mapKey) = 0;
	virtual bool InitializeTexture(ITexture* texture, const TextureInitializer& texInit) = 0;
};

using DevicePtr = std::shared_ptr<IDevice>;

} // namespace Luna