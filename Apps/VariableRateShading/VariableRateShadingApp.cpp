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

#include "VariableRateShadingApp.h"

#include "BinaryReader.h"
#include "FileSystem.h"
#include "Graphics\CommandContext.h"
#include "Graphics\CommonStates.h"
#include "Graphics\Device.h"

#include "SquidRoom.h"

using namespace Luna;
using namespace Math;
using namespace std;


VariableRateShadingApp::VariableRateShadingApp(uint32_t width, uint32_t height)
	: Application{ width, height, s_appName }
{
}


int VariableRateShadingApp::ProcessCommandLine(int argc, char* argv[])
{
	// Handle commandline arguments here
	return Application::ProcessCommandLine(argc, argv);
}


void VariableRateShadingApp::Configure()
{
	// Application config, before device creation
	Application::Configure();
}


void VariableRateShadingApp::Startup()
{
	// Application initialization, after device creation
}


void VariableRateShadingApp::Shutdown()
{
	// Application cleanup on shutdown
}


void VariableRateShadingApp::Update()
{
	// Application update tick
	// Set m_bIsRunning to false if your application wants to exit
}


void VariableRateShadingApp::Render()
{
	// Application main render loop
	Application::Render();
}


void VariableRateShadingApp::CreateDeviceDependentResources()
{
	// Create any resources that depend on the device, but not the window size

	LoadAssets();
	InitRootSignature();
}


void VariableRateShadingApp::CreateWindowSizeDependentResources()
{
	// Create any resources that depend on window size.  May be called when the window size changes.
	if (!m_pipelineCreated)
	{
		InitPipeline();
		m_pipelineCreated = true;
	}
}


void VariableRateShadingApp::InitRootSignature()
{
	RootSignatureDesc desc{
		.name				= "Scene Root Signature",
		.rootParameters		= {
			RootCBV(0, ShaderStage::Vertex),
			Table({ TextureSRV(3) }, ShaderStage::Pixel)
		},
		.staticSamplers		= { 
			StaticSampler(CommonStates::SamplerLinearClamp(), ShaderStage::Pixel), 
			StaticSampler(CommonStates::SamplerLinearWrap(), ShaderStage::Pixel) 
		}
	};
	m_sceneRootSignature = CreateRootSignature(desc);
}


void VariableRateShadingApp::InitPipeline()
{
	VertexStreamDesc vertexStreamDesc{
		.inputSlot				= 0,
		.stride					= sizeof(Vertex),
		.inputClassification	= InputClassification::PerVertexData
	};

	GraphicsPipelineDesc desc{
		.name				= "Graphics Pipeline",
		.blendState			= CommonStates::BlendDisable(),
		.depthStencilState	= CommonStates::DepthStateReadWriteReversed(),
		.rasterizerState	= CommonStates::RasterizerDefault(),
		.rtvFormats			= { GetColorFormat() },
		.dsvFormat			= GetDepthFormat(),
		.topology			= PrimitiveTopology::TriangleList,
		.vertexShader		= { .shaderFile = "ShadowsAndSceneVS" },
		.pixelShader		= { .shaderFile = "ShadowsAndScenePS" },
		.vertexStreams		= { vertexStreamDesc },
		.vertexElements		= SampleAssets::s_standardVertexElements,
		.rootSignature		= m_sceneRootSignature
	};

	m_sceneGraphicsPipeline = CreateGraphicsPipeline(desc);
}


void VariableRateShadingApp::LoadAssets()
{
	using namespace SampleAssets;

	string filepath = m_fileSystem->GetFullPath(s_dataFileName);

	size_t dataSize = 0;
	unique_ptr<std::byte[]> data;
	BinaryReader::ReadEntireFile(filepath, data, &dataSize);

	// Create vertex and index buffers
	size_t numVertices = s_vertexDataSize / s_standardVertexStride;
	m_vertexBuffer = CreateVertexBuffer("Vertex Buffer", numVertices, s_standardVertexStride, data.get() + s_vertexDataOffset);

	// Compute scene bounding box
	Vertex* currentVertex = (Vertex*)(data.get() + s_vertexDataOffset);
	constexpr float maxF = numeric_limits<float>::max();
	Vector3 minExtents{ maxF, maxF, maxF };
	Vector3 maxExtents{ -maxF, -maxF, -maxF };
	for (uint32_t i = 0; i < numVertices; ++i)
	{
		Vector3 pos{ currentVertex->pos[0], currentVertex->pos[1], currentVertex->pos[2] };
		minExtents = Math::Min(minExtents, pos);
		maxExtents = Math::Max(maxExtents, pos);
		++currentVertex;
	}
	m_sceneBoundingBox = Math::BoundingBoxFromMinMax(minExtents, maxExtents);

	// Create index buffer
	GpuBufferDesc indexBufferDesc{
		.name			= "Index Buffer",
		.resourceType	= ResourceType::IndexBuffer,
		.memoryAccess	= MemoryAccess::GpuRead,
		.elementCount	= s_indexDataSize / sizeof(uint32_t),
		.elementSize	= sizeof(uint32_t),
		.initialData	= data.get() + s_indexDataOffset
	};
	m_indexBuffer = CreateGpuBuffer(indexBufferDesc);

	// Create textures
	uint32_t i = 0;
	auto device = GetDevice();
	for (const auto& textureData : s_textures)
	{
		string texName = format("SquidRoom_Texture_{}.dds", i++);

		TextureDesc desc{
			.name		= texName,
			.width		= textureData.width,
			.height		= textureData.height,
			.depth		= 1,
			.numMips	= 1,
			.format		= textureData.format,
			.dataSize	= textureData.data->size,
			.data		= data.get() + textureData.data->offset
		};

		TexturePtr texture = device->CreateTexture2D(desc);

		m_textures.push_back(texture);
	}
}