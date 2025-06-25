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
#include "Graphics\RootSignature.h"


namespace Luna
{

class IDevice
{
public:
	virtual ~IDevice() = default;

	virtual ColorBufferPtr CreateColorBuffer(const ColorBufferDesc& colorBufferDesc) = 0;
	virtual DepthBufferPtr CreateDepthBuffer(const DepthBufferDesc& depthBufferDesc) = 0;
	virtual GpuBufferPtr CreateGpuBuffer(const GpuBufferDesc& gpuBufferDesc) = 0;

	virtual RootSignaturePtr CreateRootSignature(const RootSignatureDesc& rootSignatureDesc) = 0;

	virtual GraphicsPipelineStatePtr CreateGraphicsPipelineState(const GraphicsPipelineDesc& pipelineDesc) = 0;
};

using DevicePtr = std::shared_ptr<IDevice>;

} // namespace Luna