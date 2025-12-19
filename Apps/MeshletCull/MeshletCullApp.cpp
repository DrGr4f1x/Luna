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

#include "MeshletCullApp.h"

#include "InputSystem.h"
#include "Graphics\CommandContext.h"
#include "Graphics\CommonStates.h"
#include "Graphics\Device.h"
#include "Graphics\DeviceCaps.h"

using namespace Luna;
using namespace Math;
using namespace std;


namespace
{

const char* s_modelFilenames[] =
{
	"Dragon_LOD0.bin",
	"Camera.bin"
};


struct ObjectDefinition
{
	uint32_t modelIndex;
	XMFLOAT3 position;
	XMFLOAT3 rotation;
	float scale;

	bool cull;
	bool drawMeshlets;
};


const ObjectDefinition s_sceneDefinition[] =
{
	{ 0, {}, {}, 0.2f, true, true },  // View Model
	{ 1, {}, {}, 1.0f, false, false } // Debug Camera - this transform gets overwritten every frame by our controller.
};


enum NamedObjectIndex
{
	ViewModel = 0,
	DebugCamera
};


uint32_t ComputeSemanticByteOffset(const std::vector<Luna::VertexElementDesc>& layoutElems, const char* name)
{
	uint32_t offset = 0;
	for (uint32_t i = 0; i < (uint32_t)layoutElems.size(); ++i)
	{
		auto& elem = layoutElems[i];

		if (std::strcmp(elem.semanticName, name) == 0)
		{
			if (elem.alignedByteOffset != ~0u)
			{
				offset = elem.alignedByteOffset;
			}

			return offset;
		}

		offset += BlockSize(elem.format);
	}

	return uint32_t(-1);
}


bool RayIntersectSphere(FXMVECTOR o, FXMVECTOR d, FXMVECTOR s)
{
	XMVECTOR l = o - s;
	XMVECTOR r = XMVectorSplatW(s);
	XMVECTOR a = XMVector3Dot(d, d);
	XMVECTOR b = 2.0 * XMVector3Dot(l, d);
	XMVECTOR c = XMVector3Dot(l, l) - (r * r);

	XMVECTOR disc = b * b - 4 * a * c;
	return !XMVector4Less(disc, g_XMZero);
}


XMVECTOR RayIntersectTriangle(FXMVECTOR o, FXMVECTOR d, FXMVECTOR p0, GXMVECTOR p1, HXMVECTOR p2)
{
	XMVECTOR edge1, edge2, h, s, q;
	XMVECTOR a, f, u, v;
	edge1 = p1 - p0;
	edge2 = p2 - p0;

	h = XMVector3Cross(d, edge2);
	a = XMVector3Dot(edge1, h);
	if (XMVector4Less(XMVectorAbs(a), g_XMEpsilon))
		return g_XMQNaN;

	f = g_XMOne / a;
	s = o - p0;
	u = f * XMVector3Dot(s, h);
	if (XMVector4Less(u, g_XMZero) || XMVector4Greater(u, g_XMOne))
		return g_XMQNaN;

	q = XMVector3Cross(s, edge1);
	v = f * XMVector3Dot(d, q);
	if (XMVector4Less(v, g_XMZero) || XMVector4Greater(u + v, g_XMOne))
		return g_XMQNaN;

	// At this stage we can compute t to find out where the intersection point is on the line.
	XMVECTOR t = f * XMVector3Dot(edge2, q);
	if (XMVector4Greater(t, g_XMEpsilon) && XMVector4Less(t, XMVectorReciprocal(g_XMEpsilon))) // ray intersection
	{
		return t;
	}

	return g_XMQNaN; // This means that there is a line intersection but not a ray intersection.
}


inline uint32_t DivRoundUp(uint32_t num, uint32_t den) 
{ 
	return (num + den - 1) / den; 
}

} // anonymous namespace


MeshletCullApp::MeshletCullApp(uint32_t width, uint32_t height)
	: Application{ width, height, s_appName }
{}


int MeshletCullApp::ProcessCommandLine(int argc, char* argv[])
{
	// Handle commandline arguments here
	return Application::ProcessCommandLine(argc, argv);
}


void MeshletCullApp::Configure()
{
	// Application config, before device creation
	Application::Configure();
}


void MeshletCullApp::Startup()
{
	if (!GetDevice()->GetDeviceCaps().features.meshShader)
	{
		Utility::ExitFatal("Mesh shader support is required for this sample.", "Missing feature");
	}
}


void MeshletCullApp::Shutdown()
{
	for (auto& obj : m_objects)
	{
		obj.instanceBuffer->Unmap();
	}
}


void MeshletCullApp::Update()
{
	m_controller.Update(m_inputSystem.get(), (float)m_timer.GetElapsedSeconds(), m_mouseMoveHandled);
	m_debugController.Update(m_inputSystem.get(), (float)m_timer.GetElapsedSeconds(), m_mouseMoveHandled);

	if (m_inputSystem->IsPressed(DigitalInput::kKey_space))
	{
		m_drawMeshlets = !m_drawMeshlets;
		m_selectedIndex = uint32_t(-1);
		m_highlightedIndex = uint32_t(-1);
	}
	else if (m_inputSystem->IsPressed(DigitalInput::kKey_tab))
	{
		m_selectedIndex = m_highlightedIndex;
	}

	UpdateConstantBuffer();
}


void MeshletCullApp::Render()
{
	auto& context = GraphicsContext::Begin("Scene");

	context.TransitionResource(GetColorBuffer(), ResourceState::RenderTarget);
	context.TransitionResource(GetDepthBuffer(), ResourceState::DepthWrite);

	const Color clearColor{ 0.0f, 0.2f, 0.4f, 1.0f };
	context.ClearColor(GetColorBuffer(), clearColor);
	context.ClearDepth(GetDepthBuffer());

	context.BeginRendering(GetColorBuffer(), GetDepthBuffer());

	context.SetViewportAndScissor(0u, 0u, GetWindowWidth(), GetWindowHeight());

	context.SetRootSignature(m_rootSignature);
	context.SetMeshletPipeline(m_meshletPipeline);

	//context.SetConstantBuffer(0, m_constantBuffer);
	context.SetDescriptors(0, m_cbvDescriptorSet);

	uint32_t i = 0;
	for (auto& obj : m_objects)
	{
		//context.SetConstantBuffer(2, obj.instanceBuffer);
		context.SetDescriptors(2, m_objectCbvDescriptorSets[i]);

		uint32_t j = 0;
		for (auto& mesh : obj.model)
		{
			// TODO: Support this, with push descriptors or DynamicDescriptorHead
			//context.SetConstantBuffer(1, mesh.meshInfoResource);
			//context.SetSRV(3, mesh.vertexResources[0]);
			//context.SetSRV(4, mesh.meshletResource);
			//context.SetSRV(5, mesh.uniqueVertexIndexResource);
			//context.SetSRV(6, mesh.primitiveIndexResource);
			//context.SetSRV(7, mesh.cullDataResource);

			context.SetDescriptors(1, m_meshCbvDescriptorSets[i][j]);
			context.SetDescriptors(3, m_srvDescriptorSets[i][j]);

			const uint32_t meshletCount = (uint32_t)mesh.meshlets.size();
			context.DispatchMesh(DivideByMultiple(meshletCount, AS_GROUP_SIZE), 1, 1);

			++j;
		}

		++i;
	}

	// Draw the frustum bounds of the culling camera.
	m_frustumVisualizer.Render(context);

	// Culling data visualization - only in meshlet viewing mode and have a meshlet selected.
	if (m_drawMeshlets && m_selectedIndex != uint32_t(-1))
	{
		auto& obj = m_objects[ViewModel];

		Matrix4 world = obj.worldMatrix;
		Matrix4 view = m_camera.GetViewMatrix();
		Matrix4 proj = m_camera.GetProjectionMatrix();

		m_cullDataVisualizer.Update(world, view, proj, Vector4(DirectX::Colors::Yellow));
		m_cullDataVisualizer.Render(context, obj.model.GetMesh(0), m_selectedIndex, 1);
	}

	RenderUI(context);

	context.EndRendering();
	context.TransitionResource(GetColorBuffer(), ResourceState::Present);

	context.Finish();
}


void MeshletCullApp::CreateDeviceDependentResources()
{
	m_camera.SetPerspectiveMatrix(
		DirectX::XMConvertToRadians(60.0f),
		GetWindowAspectRatio(),
		0.1f,
		1000.0f);
	m_camera.SetPosition(Vector3(0.0f, -15.0f, -40.0f));

	m_debugCamera.SetPerspectiveMatrix(
		DirectX::XMConvertToRadians(60.0f),
		GetWindowAspectRatio(),
		1.0f,
		300.0f);
	m_debugCamera.SetPosition(Vector3(0.0f, -10.0f, -21.0f));

	m_constantBuffer = CreateConstantBuffer("Constant Buffer", 1, sizeof(Constants));

	InitRootSignature();

	LoadModels();

	InitDescriptorSets();

	auto device = GetDevice();
	m_frustumVisualizer.CreateDeviceDependentResources(device);
	m_cullDataVisualizer.CreateDeviceDependentResources(device);

	Vector3 center = m_objects[ViewModel].model.GetBoundingSphere().Center;

	m_controller.SetSpeedScale(0.025f);
	m_controller.SetCameraMode(CameraMode::WASD);
	m_controller.AllowCameraRotation(false);
	m_controller.RefreshFromCamera();
	//m_controller.SetOrbitTarget(center, Length(m_camera.GetPosition()), 0.25f);

	m_debugController.SetSpeedScale(0.025f);
	m_debugController.SetCameraMode(CameraMode::WASD);
	m_debugController.AllowCameraRotation(false);
	m_debugController.RefreshFromCamera();
	//m_debugController.SetOrbitTarget(center, Length(m_camera.GetPosition()), 0.25f);
}


void MeshletCullApp::CreateWindowSizeDependentResources()
{
	m_camera.SetAspectRatio(GetWindowAspectRatio());
	m_debugCamera.SetAspectRatio(GetWindowAspectRatio());

	if (!m_pipelineCreated)
	{
		InitPipeline();
		m_pipelineCreated = true;
	}

	auto device = GetDevice();
	m_frustumVisualizer.CreateWindowSizeDependentResources(device);
	m_cullDataVisualizer.CreateWindowSizeDependentResources(device);
}


void MeshletCullApp::InitRootSignature()
{
	constexpr ShaderStage stages = ShaderStage::Amplification | ShaderStage::Mesh | ShaderStage::Pixel;

	RootSignatureDesc desc{
		.name = "Root Signature",
		.rootParameters = {
			RootCBV(0, stages),
			RootCBV(1, stages),
			RootCBV(2, stages),
			//RootSRV(0, stages), // TODO: use push descriptors for these
			//RootSRV(1, stages),
			//RootSRV(2, stages),
			//RootSRV(3, stages),
			//RootSRV(4, stages)
			Table({ StructuredBufferSRV, StructuredBufferSRV, RawBufferSRV, StructuredBufferSRV, StructuredBufferSRV }, stages)
		}
	};
	m_rootSignature = CreateRootSignature(desc);
}


void MeshletCullApp::InitPipeline()
{
	MeshletPipelineDesc desc{
		.name					= "Meshlet Pipeline",
		.blendState				= CommonStates::BlendDisable(),
		.depthStencilState		= CommonStates::DepthStateReadWriteReversed(),
		.rasterizerState		= CommonStates::RasterizerTwoSided(),
		.rtvFormats				= { GetColorFormat()},
		.dsvFormat				= GetDepthFormat(),
		.topology				= PrimitiveTopology::TriangleList,
		.amplificationShader	= { .shaderFile = "MeshletAS" },
		.meshShader				= { .shaderFile = "MeshletMS" },
		.pixelShader			= { .shaderFile = "MeshletPS" },
		.rootSignature			= m_rootSignature
	};
	m_meshletPipeline = CreateMeshletPipeline(desc);
}


void MeshletCullApp::InitDescriptorSets()
{
	m_cbvDescriptorSet = m_rootSignature->CreateDescriptorSet(0);
	m_cbvDescriptorSet->SetCBV(0, m_constantBuffer);

	m_objectCbvDescriptorSets.resize(m_objects.size());
	m_meshCbvDescriptorSets.resize(m_objects.size());
	m_srvDescriptorSets.resize(m_objects.size());

	for (size_t i = 0; i < m_objects.size(); ++i)
	{
		const auto& obj = m_objects[i];

		auto objectCbvDescriptorSet = m_rootSignature->CreateDescriptorSet(2);
		objectCbvDescriptorSet->SetCBV(0, obj.instanceBuffer);
		m_objectCbvDescriptorSets[i] = objectCbvDescriptorSet;
		
		m_meshCbvDescriptorSets[i].resize(obj.model.GetMeshCount());
		m_srvDescriptorSets[i].resize(obj.model.GetMeshCount());

		for (size_t j = 0; j < obj.model.GetMeshCount(); ++j)
		{
			const auto& mesh = obj.model.GetMesh((uint32_t)j);

			auto meshCbvDescriptorSet = m_rootSignature->CreateDescriptorSet(1);
			meshCbvDescriptorSet->SetCBV(0, mesh.meshInfoResource);
			m_meshCbvDescriptorSets[i][j] = meshCbvDescriptorSet;

			auto srvDescriptorSet = m_rootSignature->CreateDescriptorSet(3);
			srvDescriptorSet->SetSRV(0, mesh.vertexResources[0]);
			srvDescriptorSet->SetSRV(1, mesh.meshletResource);
			srvDescriptorSet->SetSRV(2, mesh.uniqueVertexIndexResource);
			srvDescriptorSet->SetSRV(3, mesh.primitiveIndexResource);
			srvDescriptorSet->SetSRV(4, mesh.cullDataResource);

			m_srvDescriptorSets[i][j] = srvDescriptorSet;
		}
	}
}


void MeshletCullApp::LoadModels()
{
	m_objects.resize(_countof(s_sceneDefinition));

	for (uint32_t i = 0; i < (uint32_t)m_objects.size(); ++i)
	{
		auto& obj = m_objects[i];
		auto& def = s_sceneDefinition[i];

		// Copy over the render flags
		obj.flags |= def.cull ? CULL_FLAG : 0;
		obj.flags |= def.drawMeshlets ? MESHLET_FLAG : 0;

		// Convert the transform definition to a matrix
		XMMATRIX world = XMMatrixAffineTransformation(
			XMVectorReplicate(def.scale),
			g_XMZero,
			XMQuaternionRotationRollPitchYawFromVector(XMLoadFloat3(&def.rotation)),
			XMLoadFloat3(&def.position)
		);
		obj.worldMatrix = Matrix4(world);

		// Load model from disk
		obj.model.LoadFromFile(s_modelFilenames[def.modelIndex]);
		obj.model.InitResources(GetDevice());

		// Create per-object instance data buffer
		GpuBufferDesc desc{
			.name			= "Per-Instance Data",
			.resourceType	= ResourceType::ConstantBuffer,
			.memoryAccess	= MemoryAccess::GpuRead | MemoryAccess::CpuWrite,
			.elementCount	= 1,
			.elementSize	= sizeof(Instance),
			.initialData	= &obj.instanceData
		};
		obj.instanceBuffer = CreateGpuBuffer(desc);

		obj.instanceData = obj.instanceBuffer->Map();
	}
}


void MeshletCullApp::UpdateConstantBuffer()
{
	// TODO: Convert all this math to use Luna types.  Fill in missing parts of the math API.
	Matrix4 view = m_camera.GetViewMatrix();
	Matrix4 proj = m_camera.GetProjectionMatrix();
	Matrix4 viewInv = Invert(view);

	Vector4 scale, rot, viewPosition;
	Decompose(scale, rot, viewPosition, viewInv);

	// Calculate the debug camera's properties to extract plane data.
	Matrix4 cullView = m_debugCamera.GetViewMatrix();
	Matrix4 cullWorld = Invert(cullView);
	
	Matrix4 cullProj = m_debugCamera.GetProjectionMatrix();

	Vector4 cullScale, cullRot, cullPos;
	Decompose(cullScale, cullRot, cullPos, cullWorld);

	// Extract the planes from the debug camera view-projection matrix.
	Matrix4 viewProj = Transpose(cullProj * cullView);
	Vector4 planes[6] =
	{
		PlaneNormalize(viewProj.GetW() + viewProj.GetX()),	// Left
		PlaneNormalize(viewProj.GetW() - viewProj.GetX()),	// Right
		PlaneNormalize(viewProj.GetW() + viewProj.GetY()),	// Bottom
		PlaneNormalize(viewProj.GetW() - viewProj.GetY()),	// Top
		PlaneNormalize(viewProj.GetZ()),					// Near
		PlaneNormalize(viewProj.GetW() - viewProj.GetZ()),	// Far
	};

	m_constants.HighlightedIndex = m_highlightedIndex;
	m_constants.SelectedIndex = m_selectedIndex;
	m_constants.DrawMeshlets = m_drawMeshlets;

	XMStoreFloat4x4(&m_constants.View, view);
	XMStoreFloat4x4(&m_constants.ViewProj, proj * view);
	XMStoreFloat3(&m_constants.ViewPosition, viewPosition);
	XMStoreFloat3(&m_constants.CullViewPosition, cullPos);

	for (uint32_t i = 0; i < _countof(planes); ++i)
	{
		XMStoreFloat4(&m_constants.Planes[i], planes[i]);
	}

	m_constantBuffer->Update(sizeof(Constants), &m_constants);

	Matrix4 viewProjMatrix{ proj * view };

	// Update the planes for the Frustum drawer.
	m_frustumVisualizer.Update(viewProjMatrix, planes);

	m_objects[DebugCamera].worldMatrix = Matrix4(cullWorld);

	// Update the scene objects with potentially modified data.
	for (auto& obj : m_objects)
	{
		Vector4 scale, rot, pos;
		Decompose(scale, rot, pos, obj.worldMatrix);

		// TODO: the original looks like this:
		// auto& instance = *(reinterpret_cast<Instance*>(obj.InstanceData) + m_frameIndex);
		// Add support for m_frameIndex
		auto& instance = *(reinterpret_cast<Instance*>(obj.instanceData));
		//XMStoreFloat4x4(&instance.World, XMMatrixTranspose(obj.worldMatrix));
		//XMStoreFloat4x4(&instance.WorldInvTrans, XMMatrixTranspose(XMMatrixInverse(nullptr, XMMatrixTranspose(obj.worldMatrix))));
		XMStoreFloat4x4(&instance.World, obj.worldMatrix);
		XMStoreFloat4x4(&instance.WorldInvTrans, XMMatrixTranspose(XMMatrixInverse(nullptr, obj.worldMatrix)));
		instance.Scale = XMVectorGetX(scale);
		instance.Flags = obj.flags;
	}

	// Do ray intersection routine of the view model if in meshlet viewing mode.
	if (m_drawMeshlets)
	{
		Pick();
	}
}


void MeshletCullApp::Pick()
{
	m_highlightedIndex = uint32_t(-1);

	// Cache the object and its mesh (using only the first mesh of the model)
	auto& obj = m_objects[ViewModel];
	auto& mesh = obj.model.GetMesh(0);

	// Grab the world, view, & proj matrices
	XMMATRIX world = obj.worldMatrix;
	XMMATRIX view = m_camera.GetViewMatrix();
	XMMATRIX proj = m_camera.GetProjectionMatrix();

	// Determine the ray cast location in world space (sampling done in pixel space)
	XMVECTOR sampleSS = GetSamplePoint();
	XMVECTOR sampleWS = XMVector3Unproject(sampleSS, 0, 0, (float)GetWindowWidth(), (float)GetWindowHeight(), 0, 1, proj, view, XMMatrixIdentity());

	XMVECTOR viewPosWS = XMVector3Transform(g_XMZero, XMMatrixInverse(nullptr, view));
	XMVECTOR viewDirWS = XMVector3Normalize(sampleWS - viewPosWS);

	// Grab the vertex positions array
	const uint8_t* vbMem = reinterpret_cast<const uint8_t*>(mesh.vertices[0].data());
	uint32_t stride = mesh.vertexStrides[0];
	uint32_t offset = ComputeSemanticByteOffset(mesh.layoutElems, "POSITION");
	assert(offset != uint32_t(-1));

	// Transform ray into object space for intersection tests
	XMMATRIX invWorld = XMMatrixInverse(nullptr, world);
	XMVECTOR dir = XMVector3Normalize(XMVector3TransformNormal(viewDirWS, invWorld));
	XMVECTOR org = XMVector3TransformCoord(viewPosWS, invWorld);

	XMVECTOR minT = g_XMFltMax;

	for (uint32_t i = 0; i < static_cast<uint32_t>(mesh.meshlets.size()); ++i)
	{
		auto& meshlet = mesh.meshlets[i];
		auto& cull = mesh.cullingData[i];

		// Quick narrow-phase test against the meshlet's sphere bounds.
		XMFLOAT4 boundingSphereVec{ cull.boundingSphere.Center.x, cull.boundingSphere.Center.y, cull.boundingSphere.Center.z, cull.boundingSphere.Radius };
		if (!RayIntersectSphere(org, dir, XMLoadFloat4(&boundingSphereVec)))
		{
			continue;
		}

		// Test each triangle of the meshlet.
		for (uint32_t j = 0; j < meshlet.primCount; ++j)
		{
			uint32_t i0, i1, i2;
			mesh.GetPrimitive(meshlet.primOffset + j, i0, i1, i2);

			uint32_t v0 = mesh.GetVertexIndex(meshlet.vertOffset + i0);
			uint32_t v1 = mesh.GetVertexIndex(meshlet.vertOffset + i1);
			uint32_t v2 = mesh.GetVertexIndex(meshlet.vertOffset + i2);

			XMVECTOR p0 = XMLoadFloat3((XMFLOAT3*)(vbMem + v0 * stride + offset));
			XMVECTOR p1 = XMLoadFloat3((XMFLOAT3*)(vbMem + v1 * stride + offset));
			XMVECTOR p2 = XMLoadFloat3((XMFLOAT3*)(vbMem + v2 * stride + offset));

			XMVECTOR t = RayIntersectTriangle(org, dir, p0, p1, p2);
			if (XMVector4Less(t, minT))
			{
				minT = t;
				m_highlightedIndex = i;
			}
		}
	}
}


Vector4 MeshletCullApp::GetSamplePoint()
{
	return Vector4((float)m_mouseX, (float)m_mouseY, 1.0f, 1.0f);
}