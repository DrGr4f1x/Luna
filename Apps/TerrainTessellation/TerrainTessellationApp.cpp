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

#include "TerrainTessellationApp.h"

#include "Graphics\CommandContext.h"
#include "Graphics\CommonStates.h"

using namespace Luna;
using namespace Math;
using namespace std;


TerrainTessellationApp::TerrainTessellationApp(uint32_t width, uint32_t height)
	: Application{ width, height, s_appName }
	, m_controller{ m_camera, Vector3(kYUnitVector) }
{}


int TerrainTessellationApp::ProcessCommandLine(int argc, char* argv[])
{
	// Handle commandline arguments here
	return Application::ProcessCommandLine(argc, argv);
}


void TerrainTessellationApp::Configure()
{
	// Application config, before device creation
	Application::Configure();
}


void TerrainTessellationApp::Startup()
{
	m_showGrid = true;
}


void TerrainTessellationApp::Shutdown()
{
	// Application cleanup on shutdown
}


void TerrainTessellationApp::Update()
{
	m_controller.Update(m_inputSystem.get(), (float)m_timer.GetElapsedSeconds(), m_mouseMoveHandled);

	UpdateConstantBuffers();
}


void TerrainTessellationApp::UpdateUI()
{
	// TODO
}


void TerrainTessellationApp::Render()
{
	auto& context = GraphicsContext::Begin("Scene");

	context.TransitionResource(GetColorBuffer(), ResourceState::RenderTarget);
	context.TransitionResource(m_depthBuffer, ResourceState::DepthWrite);
	context.ClearColor(GetColorBuffer());
	context.ClearDepth(m_depthBuffer);

	context.BeginRendering(GetColorBuffer(), m_depthBuffer);

	context.SetViewportAndScissor(0u, 0u, GetWindowWidth(), GetWindowHeight());

	// Sky sphere
	context.SetRootSignature(m_skyRootSignature);
	context.SetGraphicsPipeline(m_skyPipeline);

	context.SetResources(m_skyResources);

	m_skyModel->Render(context);

	// Terrain
	context.SetRootSignature(m_terrainRootSignature);
	context.SetGraphicsPipeline(m_terrainPipeline);

	context.SetResources(m_terrainResources);

	context.SetIndexBuffer(m_terrainIndices);
	context.SetVertexBuffer(0, m_terrainVertices);

	context.DrawIndexed((uint32_t)m_terrainIndices->GetElementCount());

	RenderGrid(context);
	RenderUI(context);

	context.EndRendering();
	context.TransitionResource(GetColorBuffer(), ResourceState::Present);

	context.Finish();
}


void TerrainTessellationApp::CreateDeviceDependentResources()
{
	m_camera.SetPerspectiveMatrix(
		XMConvertToRadians(60.0f),
		GetWindowAspectRatio(),
		0.1f,
		512.0f);
	m_camera.SetPosition(Vector3(-18.0f, -22.5f, -57.5f));
	//m_camera.SetRotation(Quaternion(XMConvertToRadians(7.5f), XMConvertToRadians(-343.0f), 0.0f));
	m_camera.Update();

	m_controller.SetSpeedScale(0.01f);
	m_controller.RefreshFromCamera();
	m_controller.SetCameraMode(CameraMode::ArcBall);
	m_controller.SetOrbitTarget(Vector3(0.0f, -2.0f, 0.0f), Length(m_camera.GetPosition()), 4.0f);

	InitRootSignatures();
	InitConstantBuffers();

	LoadAssets();

	InitTerrain();

	InitResourceSets();
}


void TerrainTessellationApp::CreateWindowSizeDependentResources()
{
	InitDepthBuffer();
	if (!m_pipelinesCreated)
	{
		InitPipelines();
		m_pipelinesCreated = true;
	}
}


void TerrainTessellationApp::InitDepthBuffer()
{
	DepthBufferDesc depthBufferDesc{
		.name		= "Depth Buffer",
		.width		= GetWindowWidth(),
		.height		= GetWindowHeight(),
		.format		= GetDepthFormat()
	};

	m_depthBuffer = CreateDepthBuffer(depthBufferDesc);
}


void TerrainTessellationApp::InitRootSignatures()
{
	RootSignatureDesc skyRootSignatureDesc{
		.name				= "Sky Root Signature",
		.flags				= RootSignatureFlags::AllowInputAssemblerInputLayout,
		.rootParameters		= {
			RootParameter::RootCBV(0, ShaderStage::Vertex),
			RootParameter::Range(DescriptorType::TextureSRV, 0, 1, ShaderStage::Pixel),
			RootParameter::Range(DescriptorType::Sampler, 0, 1, ShaderStage::Pixel)
		}
	};
	m_skyRootSignature = CreateRootSignature(skyRootSignatureDesc);

	RootSignatureDesc terrainRootSignatureDesc{
		.name				= "Terrain Root Signature",
		.flags				= RootSignatureFlags::AllowInputAssemblerInputLayout,
		.rootParameters		= {
			RootParameter::Table({ DescriptorRange::ConstantBuffer(0), DescriptorRange::TextureSRV(1) }, ShaderStage::Hull),
			RootParameter::Range(DescriptorType::Sampler, 0, 1, ShaderStage::Hull),
			RootParameter::Table({ DescriptorRange::ConstantBuffer(0), DescriptorRange::TextureSRV(1) }, ShaderStage::Domain),
			RootParameter::Range(DescriptorType::TextureSRV, 0, 2, ShaderStage::Pixel),
			RootParameter::Range(DescriptorType::Sampler, 0, 1, ShaderStage::Domain),
			RootParameter::Range(DescriptorType::Sampler, 0, 2, ShaderStage::Pixel)
		}
	};
	m_terrainRootSignature = CreateRootSignature(terrainRootSignatureDesc);
}


void TerrainTessellationApp::InitPipelines()
{
	auto vertexLayout = VertexLayout<VertexComponent::PositionNormalTexcoord>();

	VertexStreamDesc vertexStreamDesc{
		.inputSlot				= 0,
		.stride					= vertexLayout.GetSizeInBytes(),
		.inputClassification	= InputClassification::PerVertexData
	};

	// Sky graphics pipeline
	GraphicsPipelineDesc skyPipelineDesc{
		.name				= "Sky Graphics PSO",
		.blendState			= CommonStates::BlendDisable(),
		.depthStencilState	= CommonStates::DepthStateReadOnlyReversed(),
		.rasterizerState	= CommonStates::RasterizerDefaultCW(),
		.rtvFormats			= { GetColorFormat() },
		.dsvFormat			= GetDepthFormat(),
		.topology			= PrimitiveTopology::TriangleList,
		.vertexShader		= { .shaderFile = "SkySphereVS" },
		.pixelShader		= { .shaderFile = "SkySpherePS" },
		.vertexStreams		= { vertexStreamDesc },
		.vertexElements		= vertexLayout.GetElements(),
		.rootSignature		= m_skyRootSignature
	};

	m_skyPipeline = CreateGraphicsPipeline(skyPipelineDesc);

	// Terrain graphics pipeline
	GraphicsPipelineDesc terrainPipelineDesc{
		.name				= "Graphics PSO",
		.blendState			= CommonStates::BlendDisable(),
		.depthStencilState	= CommonStates::DepthStateReadWriteReversed(),
		.rasterizerState	= CommonStates::RasterizerDefaultCW(),
		.rtvFormats			= { GetColorFormat() },
		.dsvFormat			= GetDepthFormat(),
		.topology			= PrimitiveTopology::PatchList_4_ControlPoint,
		.vertexShader		= { .shaderFile = "TerrainVS" },
		.pixelShader		= { .shaderFile = "TerrainPS" },
		.hullShader			= { .shaderFile = "TerrainHS"},
		.domainShader		= { .shaderFile = "TerrainDS"},
		.vertexStreams		= { vertexStreamDesc },
		.vertexElements		= vertexLayout.GetElements(),
		.rootSignature		= m_terrainRootSignature
	};
	m_terrainPipeline = CreateGraphicsPipeline(terrainPipelineDesc);
}


void TerrainTessellationApp::InitConstantBuffers()
{
	// Sky constant buffer
	GpuBufferDesc skyConstantBufferDesc{
		.name			= "Sky Constant Buffer",
		.resourceType	= ResourceType::ConstantBuffer,
		.memoryAccess	= MemoryAccess::GpuRead | MemoryAccess::CpuWrite,
		.elementCount	= 1,
		.elementSize	= sizeof(SkyConstants)
	};
	m_skyConstantBuffer = CreateGpuBuffer(skyConstantBufferDesc);

	// Domain shader constant buffer
	GpuBufferDesc terrainConstantBufferDesc{
		.name			= "Terrain Constant Buffer",
		.resourceType	= ResourceType::ConstantBuffer,
		.memoryAccess	= MemoryAccess::GpuRead | MemoryAccess::CpuWrite,
		.elementCount	= 1,
		.elementSize	= sizeof(TerrainConstants)
	};
	m_terrainConstantBuffer = CreateGpuBuffer(terrainConstantBufferDesc);
}


void TerrainTessellationApp::InitResourceSets()
{
	m_skyResources.Initialize(m_skyRootSignature);
	m_skyResources.SetCBV(0, 0, m_skyConstantBuffer);
	m_skyResources.SetSRV(1, 0, m_skyTexture);
	m_skyResources.SetSampler(2, 0, m_samplerLinearWrap);

	m_terrainResources.Initialize(m_terrainRootSignature);
	m_terrainResources.SetCBV(0, 0, m_terrainConstantBuffer);
	m_terrainResources.SetSRV(0, 1, m_terrainHeightMap);
	m_terrainResources.SetSampler(1, 0, m_samplerLinearMirror);
	m_terrainResources.SetCBV(2, 0, m_terrainConstantBuffer);
	m_terrainResources.SetSRV(2, 1, m_terrainHeightMap);
	m_terrainResources.SetSRV(3, 0, m_terrainHeightMap);
	m_terrainResources.SetSRV(3, 1, m_terrainTextureArray);
	m_terrainResources.SetSampler(4, 0, m_samplerLinearMirror);
	m_terrainResources.SetSampler(5, 0, m_samplerLinearMirror);
	m_terrainResources.SetSampler(5, 1, m_samplerLinearWrap);
}


static float GetTerrainHeight(const uint16_t* data, int x, int y, int dim, int scale)
{
	int rx = x * scale;
	int ry = y * scale;
	rx = max(0, min(rx, dim - 1));
	ry = max(0, min(ry, dim - 1));
	rx /= scale;
	ry /= scale;
	return *(data + (rx + ry * dim) * scale) / 65535.0f;
}


void TerrainTessellationApp::InitTerrain()
{
	const uint32_t PATCH_SIZE = 64;
	const float UV_SCALE = 1.0f;

	const uint32_t scale = (uint32_t)m_terrainHeightMap->GetWidth() / PATCH_SIZE;

	const uint32_t vertexCount = PATCH_SIZE * PATCH_SIZE;
	unique_ptr<Vertex[]> vertices(new Vertex[vertexCount]);

	const float wx = 2.0f;
	const float wy = 2.0f;

	for (uint32_t x = 0; x < PATCH_SIZE; ++x)
	{
		for (uint32_t y = 0; y < PATCH_SIZE; ++y)
		{
			uint32_t index = x + y * PATCH_SIZE;
			vertices[index].position[0] = x * wx + wx / 2.0f - (float)PATCH_SIZE * wx / 2.0f;
			vertices[index].position[1] = 0.0f;
			vertices[index].position[2] = y * wy + wy / 2.0f - (float)PATCH_SIZE * wy / 2.0f;
			vertices[index].uv[0] = ((float)x / (PATCH_SIZE - 1)) * UV_SCALE;
			vertices[index].uv[1] = ((float)y / (PATCH_SIZE - 1)) * UV_SCALE;
		}
	}

	// Calculate normals from height map using a Sobel filter
	const uint16_t* data = (const uint16_t*)m_terrainHeightMap->GetData();
	const uint32_t dim = (uint32_t)m_terrainHeightMap->GetWidth();

	for (int x = 0; x < (int)PATCH_SIZE; ++x)
	{
		for (int y = 0; y < (int)PATCH_SIZE; ++y)
		{
			uint32_t index = x + y * PATCH_SIZE;

			// Get height samples centered around current position
			float heights[3][3];
			for (int hx = -1; hx <= 1; hx++)
			{
				for (int hy = -1; hy <= 1; hy++)
				{
					heights[hx + 1][hy + 1] = GetTerrainHeight(data, x + hx, y + hy, dim, scale);
				}
			}

			// Calculate the normal
			Vector3 normal;
			// Gx Sobel filter
			normal.SetX(heights[0][0] - heights[2][0] + 2.0f * heights[0][1] - 2.0f * heights[2][1] + heights[0][2] - heights[2][2]);
			// Gy Sobel filter
			normal.SetZ(heights[0][0] + 2.0f * heights[1][0] + heights[2][0] - heights[0][2] - 2.0f * heights[1][2] - heights[2][2]);
			// Calculate missing up component of the normal using the filtered x and y axis
			// The first value controls the bump strength
			normal.SetY(0.25f * sqrt(1.0f - normal.GetX() * normal.GetX() - normal.GetZ() * normal.GetZ()));

			normal = Normalize(normal * Vector3(2.0f, 1.0f, 2.0f));

			vertices[index].normal[0] = normal.GetX();
			vertices[index].normal[1] = normal.GetY();
			vertices[index].normal[2] = normal.GetZ();
		}
	}

	// Indices
	const uint32_t w = (PATCH_SIZE - 1);
	const uint32_t indexCount = w * w * 4;
	unique_ptr<uint32_t[]> indices(new uint32_t[indexCount]);
	for (uint32_t x = 0; x < w; ++x)
	{
		for (uint32_t y = 0; y < w; ++y)
		{
			uint32_t index = (x + y * w) * 4;
			indices[index] = (x + y * PATCH_SIZE);
			indices[index + 1] = indices[index] + PATCH_SIZE;
			indices[index + 2] = indices[index + 1] + 1;
			indices[index + 3] = indices[index] + 1;
		}
	}

	GpuBufferDesc vertexBufferDesc{
		.name			= "Vertex Buffer",
		.resourceType	= ResourceType::VertexBuffer,
		.memoryAccess	= MemoryAccess::GpuRead,
		.elementCount	= vertexCount,
		.elementSize	= sizeof(Vertex),
		.initialData	= vertices.get()
	};
	m_terrainVertices = CreateGpuBuffer(vertexBufferDesc);

	GpuBufferDesc indexBufferDesc{
		.name			= "Index Buffer",
		.resourceType	= ResourceType::IndexBuffer,
		.memoryAccess	= MemoryAccess::GpuRead,
		.elementCount	= indexCount,
		.elementSize	= sizeof(uint32_t),
		.initialData	= indices.get()
	};
	m_terrainIndices = CreateGpuBuffer(indexBufferDesc);

	m_terrainHeightMap->ClearRetainedData();
}


void TerrainTessellationApp::LoadAssets()
{
	auto layout = VertexLayout<VertexComponent::PositionNormalTexcoord>();
	m_skyModel = LoadModel("sphere.gltf", layout);

	m_skyTexture = LoadTexture("skysphere_rgba.ktx");
	m_terrainTextureArray = LoadTexture("terrain_texturearray_rgba.ktx");
	m_terrainHeightMap = LoadTexture("terrain_heightmap_r16.ktx", Format::R16_UNorm, false, true);

	m_samplerLinearMirror = CreateSampler(CommonStates::SamplerLinearMirror());
	m_samplerLinearWrap = CreateSampler(CommonStates::SamplerLinearWrap());
}


void TerrainTessellationApp::UpdateConstantBuffers()
{
	Matrix4 flipZMatrix{ kIdentity };
	flipZMatrix.SetZ(Vector4(0.0f, 0.0f, -1.0f, 0.0f));

	Matrix4 viewMatrix = m_camera.GetViewMatrix();
	viewMatrix.SetW(Vector4(0.0f, 0.0f, 0.0f, 1.0f));
	m_skyConstants.modelViewProjectionMatrix = m_camera.GetProjMatrix() * viewMatrix;
	m_skyConstantBuffer->Update(sizeof(SkyConstants), &m_skyConstants);

	m_terrainConstants.modelViewMatrix = m_camera.GetViewMatrix();
	m_terrainConstants.projectionMatrix = m_camera.GetProjMatrix();
	m_terrainConstants.lightPos = Vector4(-48.0f, -40.0f, 46.0f, 0.0f);
	m_terrainConstants.displacementFactor = 32.0f;
	m_terrainConstants.tessellationFactor = 0.75f;
	m_terrainConstants.tessellatedEdgeSize = 20.0f;
	m_terrainConstants.viewportDim[0] = (float)GetWindowWidth();
	m_terrainConstants.viewportDim[1] = (float)GetWindowHeight();

	Frustum frustum = m_camera.GetWorldSpaceFrustum();
	m_terrainConstants.frustumPlanes[0] = frustum.GetFrustumPlane(Frustum::kBottomPlane);
	m_terrainConstants.frustumPlanes[1] = frustum.GetFrustumPlane(Frustum::kTopPlane);
	m_terrainConstants.frustumPlanes[2] = frustum.GetFrustumPlane(Frustum::kLeftPlane);
	m_terrainConstants.frustumPlanes[3] = frustum.GetFrustumPlane(Frustum::kRightPlane);
	m_terrainConstants.frustumPlanes[4] = frustum.GetFrustumPlane(Frustum::kNearPlane);
	m_terrainConstants.frustumPlanes[5] = frustum.GetFrustumPlane(Frustum::kFarPlane);

	m_terrainConstantBuffer->Update(sizeof(TerrainConstants), &m_terrainConstants);
}