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

#include "DynamicIndexingApp.h"

#include "BinaryReader.h"
#include "FileSystem.h"
#include "Graphics\CommandContext.h"
#include "Graphics\CommonStates.h"
#include "Graphics\Device.h"
#include "Graphics\DeviceCaps.h"

using namespace Luna;
using namespace Math;
using namespace std;


DynamicIndexingApp::DynamicIndexingApp(uint32_t width, uint32_t height)
	: Application{ width, height, s_appName }
{
}


int DynamicIndexingApp::ProcessCommandLine(int argc, char* argv[])
{
	// Handle commandline arguments here
	return Application::ProcessCommandLine(argc, argv);
}


void DynamicIndexingApp::Configure()
{
	// Application config, before device creation
	Application::Configure();
}


void DynamicIndexingApp::Startup()
{
	// Application initialization, after device creation
}


void DynamicIndexingApp::Shutdown()
{
	_aligned_free(modelViewProjectionMatrices);
}


void DynamicIndexingApp::Update()
{
	m_controller.Update(m_inputSystem.get(), (float)m_timer.GetElapsedSeconds(), m_mouseMoveHandled);

	UpdateConstantBuffer();
}


void DynamicIndexingApp::Render()
{
	auto& context = GraphicsContext::Begin("Scene");

	context.TransitionResource(GetColorBuffer(), ResourceState::RenderTarget);
	context.TransitionResource(GetDepthBuffer(), ResourceState::DepthWrite);
	context.ClearColor(GetColorBuffer());
	context.ClearDepthAndStencil(GetDepthBuffer());

	context.BeginRendering(GetColorBuffer(), GetDepthBuffer());

	context.SetViewportAndScissor(0u, 0u, GetWindowWidth(), GetWindowHeight());
	context.SetRootSignature(m_rootSignature);
	context.SetGraphicsPipeline(m_graphicsPipeline);

	context.SetIndexBuffer(m_indexBuffer);
	context.SetVertexBuffer(0, m_vertexBuffer);

	for (uint32_t i = 0; i < m_cityMaterialCount; ++i)
	{
		context.SetRootCBV(0, m_constantBuffer, i * (uint32_t)m_dynamicAlignment);
		context.SetDescriptors(1, m_psDescriptorSet);
		context.SetConstant(2, 0, i);

		context.DrawIndexed((uint32_t)m_indexBuffer->GetElementCount(), 0, 0);
	}

	RenderUI(context);

	context.EndRendering();
	context.TransitionResource(GetColorBuffer(), ResourceState::Present);

	context.Finish();
}


void DynamicIndexingApp::CreateDeviceDependentResources()
{
	m_camera.SetPerspectiveMatrix(
		DirectX::XMConvertToRadians(60.0f),
		GetWindowAspectRatio(),
		0.1f,
		512.0f);
	Vector3 cameraPosition{ (m_cityColumnCount / 2.0f) * m_citySpacingInterval - (m_citySpacingInterval / 2.0f), 30.0f, 80.0f };
	m_camera.SetPosition(cameraPosition);

	LoadAssets();
	InitRootSignature();
	InitConstantBuffer();
	InitDescriptorSets();

	m_controller.RefreshFromCamera();
	m_controller.SetCameraMode(CameraMode::ArcBall);
	m_controller.SetOrbitTarget(m_sceneBoundingBox.GetCenter(), Length(cameraPosition), 0.25f);
	m_controller.SlowMovement(true);
	m_controller.SlowRotation(false);
	m_controller.SetSpeedScale(0.25f);
}


void DynamicIndexingApp::CreateWindowSizeDependentResources()
{
	if (!m_pipelineCreated)
	{
		InitPipeline();
		m_pipelineCreated = true;
	}

	m_camera.SetPerspectiveMatrix(
		DirectX::XMConvertToRadians(60.0f),
		GetWindowAspectRatio(),
		0.1f,
		512.0f);
}


void DynamicIndexingApp::InitRootSignature()
{
	RootSignatureDesc desc{
		.name				= "Root Signature",
		.rootParameters		= {
			RootCBV(0, ShaderStage::Vertex),
			Table({ TextureSRV, BindlessTextureSRV(APPEND_REGISTER, m_cityMaterialCount) }, ShaderStage::Pixel),
			RootConstants(0, 1, ShaderStage::Pixel)
		},
		.staticSamplers		= { StaticSampler(CommonStates::SamplerLinearWrap(), ShaderStage::Pixel) }
	};
	m_rootSignature = CreateRootSignature(desc);
}


void DynamicIndexingApp::InitPipeline()
{
	VertexStreamDesc vertexStreamDesc{
		.inputSlot				= 0,
		.stride					= sizeof(Vertex),
		.inputClassification	= InputClassification::PerVertexData
	};

	vector<VertexElementDesc> vertexElements{
		{ "POSITION", 0, Format::RGB32_Float, 0, offsetof(Vertex, pos), InputClassification::PerVertexData, 0 },
		{ "NORMAL", 0, Format::RGB32_Float, 0, offsetof(Vertex, normal), InputClassification::PerVertexData, 0 },
		{ "TEXCOORD", 0, Format::RG32_Float, 0, offsetof(Vertex, uv), InputClassification::PerVertexData, 0 },
		{ "TANGENT", 0, Format::RGB32_Float, 0, offsetof(Vertex, tangent), InputClassification::PerVertexData, 0 }
	};

	GraphicsPipelineDesc desc{
		.name				= "Graphics Pipeline",
		.blendState			= CommonStates::BlendDisable(),
		.depthStencilState	= CommonStates::DepthStateReadWriteReversed(),
		.rasterizerState	= CommonStates::RasterizerDefault(),
		.rtvFormats			= { GetColorFormat() },
		.dsvFormat			= GetDepthFormat(),
		.topology			= PrimitiveTopology::TriangleList,
		.vertexShader		= { .shaderFile = "SimpleMeshVS" },
		.pixelShader		= { .shaderFile = "MeshDynamicIndexingPS" },
		.vertexStreams		= { vertexStreamDesc },
		.vertexElements		= vertexElements,
		.rootSignature		= m_rootSignature
	};

	m_graphicsPipeline = CreateGraphicsPipeline(desc);
}


void DynamicIndexingApp::InitConstantBuffer()
{
	const DeviceCaps& caps = GetDevice()->GetDeviceCaps();

	m_dynamicAlignment = AlignUp(sizeof(Matrix4), caps.memoryAlignment.constantBufferOffset);

	size_t allocSize = m_cityMaterialCount * m_dynamicAlignment;
	modelViewProjectionMatrices = (Matrix4*)_aligned_malloc(allocSize, m_dynamicAlignment);

	m_constantBuffer = CreateConstantBuffer("Constant Buffer", m_cityMaterialCount, m_dynamicAlignment);
}


void DynamicIndexingApp::InitDescriptorSets()
{
	m_psDescriptorSet = m_rootSignature->CreateDescriptorSet(1);
	m_psDescriptorSet->SetSRV(0, m_diffuseTexture);

	vector<const IDescriptor*> bindlessDescriptors;
	bindlessDescriptors.resize(m_cityMaterialCount);
	for (uint32_t i = 0; i < m_cityMaterialCount; ++i)
	{
		bindlessDescriptors[i] = m_cityMaterialTextures[i]->GetDescriptor();
	}
	m_psDescriptorSet->SetBindlessSRVs(1, bindlessDescriptors);
}


void DynamicIndexingApp::LoadAssets()
{
	string filepath = m_fileSystem->GetFullPath("occcity.bin");

	size_t dataSize = 0;
	unique_ptr<std::byte[]> data;
	BinaryReader::ReadEntireFile(filepath, data, &dataSize);

	// Create vertex and index buffers
	size_t numVertices = m_vertexDataSize / m_standardVertexStride;
	m_vertexBuffer = CreateVertexBuffer("Vertex Buffer", numVertices, m_standardVertexStride, data.get() + m_vertexDataOffset);

	// Compute model bounding box
	Vertex* currentVertex = (Vertex*)(data.get() + m_vertexDataOffset);
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
	m_modelBoundingBox = Math::BoundingBoxFromMinMax(minExtents, maxExtents);

	GpuBufferDesc indexBufferDesc{
		.name			= "Index Buffer",
		.resourceType	= ResourceType::IndexBuffer,
		.memoryAccess	= MemoryAccess::GpuRead,
		.elementCount	= m_indexDataSize / sizeof(uint32_t),
		.elementSize	= sizeof(uint32_t),
		.initialData	= data.get() + m_indexDataOffset
	};
	m_indexBuffer = CreateGpuBuffer(indexBufferDesc);

	// Create diffuse texture
	TextureDesc textureDesc{
		.name		= "city.dds",
		.width		= m_textureWidth,
		.height		= m_textureWidth,
		.format		= m_textureFormat,
		.dataSize	= m_textureDataSize,
		.data		= data.get()
	};
	m_diffuseTexture = CreateTexture2D(textureDesc);

	// Create material textures
	const float materialGradStep = 1.0f / (float)m_cityMaterialCount;
	m_cityMaterialTextures.resize(m_cityMaterialCount);
	for (uint32_t i = 0; i < m_cityMaterialCount; ++i)
	{
		vector<std::byte> textureData;
		textureData.resize(m_cityMaterialTextureWidth * m_cityMaterialTextureHeight * 4);
		float t = materialGradStep * (float)i;
		for (uint32_t x = 0; x < m_cityMaterialTextureWidth; ++x)
		{
			for (uint32_t y = 0; y < m_cityMaterialTextureHeight; ++y)
			{
				// Compute the appropriate index into the buffer based on the x/y coordinates.
				const uint32_t pixelIndex = (y * 4 * m_cityMaterialTextureWidth) + (x * 4);

				// Determine this row's position along the rainbow gradient.
				const float tPrime = t + ((float)y / (float)m_cityMaterialTextureHeight) * materialGradStep;

				// Compute the RGB value for this position along the rainbow
				// and pack the pixel value.
				XMVECTOR hsl = XMVectorSet(tPrime, 0.5f, 0.5f, 1.0f);
				XMVECTOR rgb = XMColorHSLToRGB(hsl);
				textureData[pixelIndex + 0] = (std::byte)(255 * XMVectorGetX(rgb));
				textureData[pixelIndex + 1] = (std::byte)(255 * XMVectorGetY(rgb));
				textureData[pixelIndex + 2] = (std::byte)(255 * XMVectorGetZ(rgb));
				textureData[pixelIndex + 3] = (std::byte)255;
			}
		}

		// Create the texture
		TextureDesc textureDesc{
			.name = format("City Texture {}", i),
			.width = m_cityMaterialTextureWidth,
			.height = m_cityMaterialTextureHeight,
			.format = Format::RGBA8_UNorm,
			.dataSize = textureData.size(),
			.data = textureData.data()
		};

		auto texture = CreateTexture2D(textureDesc);
		m_cityMaterialTextures[i] = texture;
	}

	// Compute model transforms
	vector<BoundingBox> m_modelBounds{ m_cityMaterialCount };
	m_modelMatrices.resize(m_cityMaterialCount);
	uint32_t index = 0;
	for (uint32_t i = 0; i < m_cityRowCount; ++i)
	{
		float cityOffsetZ = (float)i * m_citySpacingInterval;
		for (uint32_t j = 0; j < m_cityColumnCount; ++j)
		{
			float cityOffsetX = (float)j * (-m_citySpacingInterval);

			// The y position is based off of the city's row and column 
			// position to prevent z-fighting.
			m_modelMatrices[index] = Matrix4::MakeTranslation({ cityOffsetX, 0.02f * (i * m_cityColumnCount + j), cityOffsetZ });
			m_modelBounds[index] = m_modelMatrices[index] * m_modelBoundingBox;
			++index;
		}
	}
	m_sceneBoundingBox = BoundingBoxUnion(m_modelBounds);
}


void DynamicIndexingApp::UpdateConstantBuffer()
{
	for (uint32_t i = 0; i < m_cityMaterialCount; ++i)
	{
		auto modelMatrix = (Matrix4*)((uint64_t)modelViewProjectionMatrices + (i * m_dynamicAlignment));
		*modelMatrix = m_camera.GetViewProjectionMatrix() * m_modelMatrices[i];
	}

	m_constantBuffer->Update(m_dynamicAlignment * m_cityMaterialCount, modelViewProjectionMatrices);
}