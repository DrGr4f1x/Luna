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

#include "EndCapApp.h"

#include "Graphics\CommandContext.h"
#include "Graphics\CommonStates.h"

using namespace Luna;
using namespace Math;
using namespace std;


EndCapApp::EndCapApp(uint32_t width, uint32_t height)
	: Application{ width, height, s_appName }
	, m_endCapGenerator{ this }
{
}


int EndCapApp::ProcessCommandLine(int argc, char* argv[])
{
	// Handle commandline arguments here
	return Application::ProcessCommandLine(argc, argv);
}


void EndCapApp::Configure()
{
	// Application config, before device creation
	Application::Configure();

	SetGridColor(DirectX::Colors::Black);
}


void EndCapApp::Startup()
{
	m_showGrid = true;
}


void EndCapApp::Shutdown()
{
	// Application cleanup on shutdown
}


void EndCapApp::Update()
{
	m_controller.Update(m_inputSystem.get(), (float)m_timer.GetElapsedSeconds(), m_mouseMoveHandled);

	m_planeY = Lerp(m_minY, m_maxY, m_planeDelta);

	UpdateConstantBuffers();
	m_endCapGenerator.Update(m_debugNormals, m_planeY, m_normalLength);
}


void EndCapApp::UpdateUI()
{
	auto prevModel = m_curModel;
	auto prevMultipleModels = m_multipleModels;

	if (m_uiOverlay->Header("Settings"))
	{
		m_uiOverlay->SliderFloat("Plane Height", &m_planeDelta, 0.0f, 1.0f);
		m_uiOverlay->SliderFloat("Plane Alpha", &m_alpha, 0.0f, 1.0f);
		m_uiOverlay->SliderFloat("Normal Length", &m_normalLength, 0.0f, 0.5f);
		m_uiOverlay->CheckBox("Multiple Models", &m_multipleModels);
		m_uiOverlay->CheckBox("Apply Cut", &m_applyCut);
		m_uiOverlay->CheckBox("Debug Normals", &m_debugNormals);
		m_uiOverlay->CheckBox("Show End Cap", &m_showEndCap);
		m_uiOverlay->ComboBox("Model", &m_curModel, m_modelNames);
	}

	if (prevModel != m_curModel || prevMultipleModels != m_multipleModels)
	{
		OnModelChanged();
	}
}


void EndCapApp::Render()
{
	auto& context = GraphicsContext::Begin("Scene");

	context.TransitionResource(GetColorBuffer(), ResourceState::RenderTarget);
	context.TransitionResource(GetDepthBuffer(), ResourceState::DepthWrite);
	const Color clearColor{ DirectX::Colors::BlanchedAlmond };
	context.ClearColor(GetColorBuffer(), clearColor);
	context.ClearDepthAndStencil(GetDepthBuffer());

	context.BeginRendering(GetColorBuffer(), GetDepthBuffer());

	context.SetViewportAndScissor(0u, 0u, GetWindowWidth(), GetWindowHeight());

	// Setup for mesh drawing
	context.SetRootSignature(m_meshRootSignature);
	context.SetGraphicsPipeline(m_meshPipeline);

	// Draw model
	auto model = m_models[m_curModel];

	{
		ScopedDrawEvent event(context, "Model");

		context.SetRootCBV(0, m_modelConstantBuffer);

		int applyCut = m_applyCut ? 1 : 0;

		if (m_multipleModels)
		{
			float xOffset[] = { -0.6f, 0.0f, 0.6f };
			for (uint32_t i = 0; i < _countof(xOffset); ++i)
			{
				context.SetConstants(1, xOffset[i], 0.0f, 0.0f, applyCut);
				model->Render(context);
			}
		}
		else
		{
			context.SetConstants(1, 0.0f, 0.0f, 0.0f, applyCut);
			model->Render(context);
		}
	}
	
	// Draw plane
	{
		ScopedDrawEvent event(context, "Plane");

		context.SetGraphicsPipeline(m_planePipeline);

		context.SetRootCBV(0, m_planeConstantBuffer);
		context.SetConstants(1, 0.0f, 0.0f, 0.0f);
		m_planeModel->Render(context);
	}

	context.EndRendering();

	// Generate end cap
	if (m_applyCut)
	{
		m_endCapGenerator.Render(context, model.get(), m_multipleModels);

		context.TransitionResource(m_endCapGenerator.GetEndCapMaskTexture(), ResourceState::PixelShaderResource);

		context.BeginRendering(GetColorBuffer(), GetDepthBuffer());
		context.SetViewportAndScissor(0u, 0u, GetWindowWidth(), GetWindowHeight());

		// Render end cap
		if (m_showEndCap)
		{
			ScopedDrawEvent event2(context, "End Cap");

			// Setup for mesh drawing
			context.SetRootSignature(m_endCapRootSig);
			context.SetGraphicsPipeline(m_endCapPipeline);

			context.SetRootCBV(0, m_endCapConstantBuffer);
			context.SetDescriptors(1, m_endCapDescriptors);
			m_planeModel->Render(context);
		}
	}
	else
	{
		context.BeginRendering(GetColorBuffer(), GetDepthBuffer());
		context.SetViewportAndScissor(0u, 0u, GetWindowWidth(), GetWindowHeight());
	}

	RenderGrid(context);
	RenderUI(context);

	context.EndRendering();
	context.TransitionResource(GetColorBuffer(), ResourceState::Present);

	context.Finish();
}


void EndCapApp::CreateDeviceDependentResources()
{
	m_camera.SetPerspectiveMatrix(
		DirectX::XMConvertToRadians(60.0f),
		GetWindowAspectRatio(),
		0.1f,
		512.0f);
	Vector3 cameraPosition{ 4.75f, 3.25f, 4.75f };
	m_camera.SetPosition(cameraPosition);

	InitRootSignatures();

	// Create and initialize constant buffers
	m_modelConstantBuffer = CreateConstantBuffer("Model Constant Buffer", 1, sizeof(Constants));
	m_planeConstantBuffer = CreateConstantBuffer("Plane Constant Buffer", 1, sizeof(Constants));
	m_endCapConstantBuffer = CreateConstantBuffer("End Cap Constant Buffer", 1, sizeof(EndCapConstants));
	UpdateConstantBuffers();

	LoadAssets();

	m_controller.RefreshFromCamera();
	m_controller.SetCameraMode(CameraMode::ArcBall);
	m_controller.SetOrbitTarget(Vector3{ kZero }, 4.0f, 0.25f);
	m_controller.SlowMovement(true);
	m_controller.SlowRotation(false);
	m_controller.SetSpeedScale(0.25f);

	OnModelChanged();

	m_endCapGenerator.CreateDeviceDependentResources();
}


void EndCapApp::CreateWindowSizeDependentResources()
{
	// Create any resources that depend on window size.  May be called when the window size changes.
	if (!m_pipelinesCreated)
	{
		InitPipelines();
		m_pipelinesCreated = true;
	}

	// Update the camera since the aspect ratio might have changed
	m_camera.SetPerspectiveMatrix(
		DirectX::XMConvertToRadians(60.0f),
		GetWindowAspectRatio(),
		0.1f,
		512.0f);

	m_endCapGenerator.CreateWindowSizeDependentResources();
	InitDescriptorSet();
}


void EndCapApp::InitRootSignatures()
{
	// Mesh
	auto meshDesc = RootSignatureDesc{
		.name				= "Mesh Root Signature",
		.rootParameters		= {	
			RootCBV(0, ShaderStage::Vertex),
			RootConstants(1, 4, ShaderStage::Vertex)
		}
	};

	m_meshRootSignature = CreateRootSignature(meshDesc);

	// End cap
	auto endCapDesc = RootSignatureDesc{
		.name				= "End Cap Root Signature",
		.rootParameters		= {
			RootCBV(0, ShaderStage::Vertex),
			Table({ TextureSRV }, ShaderStage::Pixel)
		}
	};

	m_endCapRootSig = CreateRootSignature(endCapDesc);
}


void EndCapApp::InitPipelines()
{
	VertexStreamDesc vertexStreamDesc{
		.inputSlot = 0,
		.stride = sizeof(Vertex),
		.inputClassification = InputClassification::PerVertexData
	};

	auto layout = VertexLayout<VertexComponent::PositionNormalColor>();

	// Mesh pipeline
	{
		GraphicsPipelineDesc meshPipelineDesc
		{
			.name				= "Mesh Graphics PSO",
			.blendState			= CommonStates::BlendDisable(),
			.depthStencilState	= CommonStates::DepthStateReadWriteReversed(),
			.rasterizerState	= CommonStates::RasterizerDefault(),
			.rtvFormats			= { GetColorFormat() },
			.dsvFormat			= GetDepthFormat(),
			.topology			= PrimitiveTopology::TriangleList,
			.vertexShader		= { .shaderFile = "MeshVS" },
			.pixelShader		= { .shaderFile = "MeshPS" },
			.vertexStreams		= { vertexStreamDesc },
			.vertexElements		= layout.GetElements(),
			.rootSignature		= m_meshRootSignature
		};

		m_meshPipeline = CreateGraphicsPipeline(meshPipelineDesc);

		meshPipelineDesc.blendState = CommonStates::BlendTraditional();
		meshPipelineDesc.depthStencilState = CommonStates::DepthStateReadOnlyReversed();
		m_planePipeline = CreateGraphicsPipeline(meshPipelineDesc);
	}

	// End cap pipeline
	{
		GraphicsPipelineDesc endCapDesc
		{
			.name				= "End Cap Graphics PSO",
			.blendState			= CommonStates::BlendDisable(),
			.depthStencilState	= CommonStates::DepthStateReadWriteReversed(),
			.rasterizerState	= CommonStates::RasterizerDefault(),
			.rtvFormats			= { GetColorFormat() },
			.dsvFormat			= GetDepthFormat(),
			.topology			= PrimitiveTopology::TriangleList,
			.vertexShader		= { .shaderFile = "EndCapVS" },
			.pixelShader		= { .shaderFile = "EndCapPS" },
			.vertexStreams		= { vertexStreamDesc },
			.vertexElements		= layout.GetElements(),
			.rootSignature		= m_endCapRootSig
		};

		m_endCapPipeline = CreateGraphicsPipeline(endCapDesc);
	}
}


void EndCapApp::InitDescriptorSet()
{
	m_endCapDescriptors = m_endCapRootSig->CreateDescriptorSet(1);
	m_endCapDescriptors->SetSRV(0, m_endCapGenerator.GetEndCapMaskTexture());
}


void EndCapApp::UpdateConstantBuffers()
{
	// Model
	m_modelConstants.viewProjectionMatrix = m_camera.GetViewProjectionMatrix();
	m_modelConstants.modelMatrix = Matrix4(kIdentity);
	m_modelConstants.modelViewMatrix = m_camera.GetViewMatrix();
	m_modelConstants.lightPos = Vector4(3.0f, 10.0f, 3.0f, 1.0f);
	m_modelConstants.modelColor = Vector4(0.6f, 0.6f, 0.9f, 0.0f);

	Vector3 planeNormal = Vector3(0.0f, 1.0f, 0.0f);
	Vector3 pointOnPlane = Vector3(0.0f, m_planeY, 0.0f);
	m_modelConstants.clipPlane = Vector4(planeNormal, -Dot(pointOnPlane, planeNormal));

	m_modelConstantBuffer->Update(sizeof(Constants), &m_modelConstants);

	// Plane
	m_planeConstants.viewProjectionMatrix = m_camera.GetViewProjectionMatrix();
	m_planeConstants.modelMatrix = Matrix4::MakeTranslation(0.0f, m_planeY, 0.0f) * m_planeScaleMatrix;
	m_planeConstants.lightPos = Vector4(3.0f, 10.0f, 3.0f, 1.0f);
	m_planeConstants.modelColor = Vector4(0.7f, 0.7f, 0.7f, 0.0f);
	m_planeConstants.alpha = m_alpha;

	m_planeConstantBuffer->Update(sizeof(Constants), &m_planeConstants);

	// End cap
	m_endCapConstants.viewProjectionMatrix = m_camera.GetViewProjectionMatrix();
	m_endCapConstants.modelMatrix = Matrix4::MakeTranslation(0.0f, m_planeY, 0.0f) * m_planeScaleMatrix;
	m_endCapConstants.lightPos = Vector4(3.0f, 10.0f, 3.0f, 1.0f);
	m_endCapConstants.modelColor = Vector4(0.6f, 0.6f, 0.9f, 0.0f);

	m_endCapConstantBuffer->Update(sizeof(EndCapConstants), &m_endCapConstants);
}


void EndCapApp::LoadAssets()
{
	auto layout = VertexLayout<VertexComponent::PositionNormalColor>();

	// Sphere
	auto model = LoadModel("sphere.gltf", layout, 1.0f);
	m_models.push_back(model);
	m_modelNames.push_back("Sphere");

	// Venus
	model = LoadModel("venus.gltf", layout, 1.8f);
	m_models.push_back(model);
	m_modelNames.push_back("Venus");

	// Coral 1
	model = LoadModel("astraea_fissicella_favistella/scene.gltf", layout, 30.0f);
	m_models.push_back(model);
	m_modelNames.push_back("Coral 1");

	// Coral 2
	model = LoadModel("gemmipora_brassica/scene.gltf", layout, 6.0f);
	m_models.push_back(model);
	m_modelNames.push_back("Coral 2");

	// Coral 3
	model = LoadModel("distichopora_violacea/scene.gltf", layout, 8.0f);
	m_models.push_back(model);
	m_modelNames.push_back("Coral 3");

	// Coral 4
	model = LoadModel("pseudodiploria_strigosa/scene.gltf", layout, 15.0f);
	m_models.push_back(model);
	m_modelNames.push_back("Coral 4");

	// Plane model
	m_planeModel = LoadModel("plane.gltf", layout, 0.2f);
}


void EndCapApp::OnModelChanged()
{
	std::vector<BoundingBox> boundingBoxes;

	BoundingBox boundingBox = m_models[m_curModel]->boundingBox;

	float xOffset[] = { -0.6f, 0.0f, 0.6f };
	if (m_multipleModels)
	{
		for (uint32_t i = 0; i < _countof(xOffset); ++i)
		{
			Vector3 center = boundingBox.GetCenter();
			Vector3 extents = boundingBox.GetExtents();

			center = center + Vector3(xOffset[i], 0.0f, 0.0f);

			boundingBoxes.push_back(BoundingBox(center, extents));
		}
		boundingBox = BoundingBoxUnion(boundingBoxes);
	}
	
	m_minY = boundingBox.GetMin().GetY();
	m_maxY = boundingBox.GetMax().GetY();

	m_camera.SetLookAt(boundingBox.GetCenter(), Vector3{ kYUnitVector });

	Vector3 extents = boundingBox.GetExtents();
	m_planeScaleMatrix = Matrix4::MakeScale(Vector3(1.1f * extents.GetX(), 1.0f, 1.1f * extents.GetZ()));
}