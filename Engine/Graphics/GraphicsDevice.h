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
class IColorBuffer;
class IDepthBuffer;
class IGpuBuffer;
class IGraphicsPipeline;
class IRootSignature;
struct ColorBufferDesc;
struct DepthBufferDesc;
struct GpuBufferDesc;
struct GraphicsPipelineDesc;
struct RootSignatureDesc;


class __declspec(uuid("DBECDD70-7F0B-4C9B-ADFA-048104E474C8")) IGraphicsDevice : public IUnknown
{
public:
	virtual ~IGraphicsDevice() = default;

	virtual wil::com_ptr<IColorBuffer> CreateColorBuffer(const ColorBufferDesc& colorBufferDesc) = 0;
	virtual wil::com_ptr<IDepthBuffer> CreateDepthBuffer(const DepthBufferDesc& depthBufferDesc) = 0;
	virtual wil::com_ptr<IGpuBuffer> CreateGpuBuffer(const GpuBufferDesc& gpuBufferDesc) = 0;
	virtual wil::com_ptr<IRootSignature> CreateRootSignature(const RootSignatureDesc& rootSignatureDesc) = 0;
	virtual wil::com_ptr<IGraphicsPipeline> CreateGraphicsPipeline(const GraphicsPipelineDesc& graphicsPipelineDesc) = 0;
};

} // namespace Luna
