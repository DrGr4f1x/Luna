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
	m_endCapGenerator.Update(m_planeY);
}


void EndCapApp::UpdateUI()
{
	if (m_uiOverlay->Header("Settings"))
	{
		m_uiOverlay->SliderFloat("Plane Height", &m_planeDelta, 0.0f, 1.0f);
		m_uiOverlay->CheckBox("Multiple Models", &m_multipleModels);
		m_uiOverlay->CheckBox("Apply Cut", &m_applyCut);
		m_uiOverlay->ComboBox("Model", &m_curModel, m_modelNames);
	}
}


void EndCapApp::Render()
{
	auto& context = GraphicsContext::Begin("Scene");

	context.TransitionResource(GetColorBuffer(), ResourceState::RenderTarget);
	context.TransitionResource(GetDepthBuffer(), ResourceState::DepthWrite);
	context.ClearColor(GetColorBuffer());
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

		context.SetRootCBV(0, m_planeConstantBuffer);
		context.SetConstants(1, 0.0f, 0.0f, 0.0f);
		m_planeModel->Render(context);
	}

	context.EndRendering();

	// Generate end cap
	m_endCapGenerator.Render(context, model.get(), m_multipleModels);

	context.BeginRendering(GetColorBuffer(), GetDepthBuffer());
	context.SetViewportAndScissor(0u, 0u, GetWindowWidth(), GetWindowHeight());

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
	UpdateConstantBuffers();

	LoadAssets();

	BoundingBox bounds = BoundingBoxUnion(m_modelBounds);
	m_minY = bounds.GetMin().GetY();
	m_maxY = bounds.GetMax().GetY();

	m_controller.RefreshFromCamera();
	m_controller.SetCameraMode(CameraMode::ArcBall);
	m_controller.SetOrbitTarget(bounds.GetCenter(), 4.0f, 0.25f);
	m_controller.SlowMovement(true);
	m_controller.SlowRotation(false);
	m_controller.SetSpeedScale(0.25f);

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
}


void EndCapApp::InitRootSignatures()
{
	auto meshDesc = RootSignatureDesc{
		.name				= "Mesh Root Signature",
		.rootParameters		= {	
			RootCBV(0, ShaderStage::Vertex),
			RootConstants(1, 4, ShaderStage::Vertex)
		}
	};

	m_meshRootSignature = CreateRootSignature(meshDesc);
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
		DepthStencilStateDesc depthStencilDesc = CommonStates::DepthStateReadWriteReversed();
		//depthStencilDesc.stencilEnable = true;
		//depthStencilDesc.backFace.stencilFunc = ComparisonFunc::Always;
		//depthStencilDesc.backFace.stencilDepthFailOp = StencilOp::Replace;
		//depthStencilDesc.backFace.stencilFailOp = StencilOp::Replace;
		//depthStencilDesc.backFace.stencilPassOp = StencilOp::Replace;
		//depthStencilDesc.frontFace = depthStencilDesc.backFace;

		GraphicsPipelineDesc meshPipelineDesc
		{
			.name				= "Mesh Graphics PSO",
			.blendState			= CommonStates::BlendDisable(),
			.depthStencilState	= depthStencilDesc,
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
	}
}


void EndCapApp::UpdateConstantBuffers()
{
	m_modelConstants.viewProjectionMatrix = m_camera.GetViewProjectionMatrix();
	m_modelConstants.modelMatrix = Matrix4(kIdentity);
	m_modelConstants.modelViewMatrix = m_camera.GetViewMatrix();
	m_modelConstants.lightPos = Vector4(3.0f, 10.0f, 3.0f, 1.0f);
	m_modelConstants.modelColor = Vector4(0.6f, 0.6f, 0.9f, 0.0f);

	Vector3 planeNormal = Vector3(0.0f, 1.0f, 0.0f);
	Vector3 pointOnPlane = Vector3(0.0f, m_planeY, 0.0f);
	m_modelConstants.clipPlane = Vector4(planeNormal, -Dot(pointOnPlane, planeNormal));

	m_modelConstantBuffer->Update(sizeof(Constants), &m_modelConstants);

	m_planeConstants.viewProjectionMatrix = m_camera.GetViewProjectionMatrix();

	m_planeConstants.modelMatrix = Matrix4::MakeTranslation(0.0f, m_planeY, 0.0f);
	m_planeConstants.lightPos = Vector4(3.0f, 10.0f, 3.0f, 1.0f);
	m_planeConstants.modelColor = Vector4(0.7f, 0.7f, 0.7f, 0.0f);

	m_planeConstantBuffer->Update(sizeof(Constants), &m_planeConstants);
}


void EndCapApp::LoadAssets()
{
	auto layout = VertexLayout<VertexComponent::PositionNormalColor>();

	// Sphere
	auto model = LoadModel("sphere.gltf", layout, 1.0f);
	m_models.push_back(model);
	m_modelNames.push_back("Sphere");
	m_modelBounds.push_back(model->boundingBox);

	// Venus
	model = LoadModel("venus.gltf", layout, 1.8f);
	m_models.push_back(model);
	m_modelNames.push_back("Venus");
	m_modelBounds.push_back(model->boundingBox);

	// Coral 1
	model = LoadModel("astraea_fissicella_favistella/scene.gltf", layout, 30.0f);
	m_models.push_back(model);
	m_modelNames.push_back("Coral 1");
	m_modelBounds.push_back(model->boundingBox);

	// Coral 2
	model = LoadModel("gemmipora_brassica/scene.gltf", layout, 6.0f);
	m_models.push_back(model);
	m_modelNames.push_back("Coral 2");
	m_modelBounds.push_back(model->boundingBox);

	// Coral 3
	model = LoadModel("distichopora_violacea/scene.gltf", layout, 8.0f);
	m_models.push_back(model);
	m_modelNames.push_back("Coral 3");
	m_modelBounds.push_back(model->boundingBox);

	m_planeModel = LoadModel("plane.gltf", layout, 0.3f);
}