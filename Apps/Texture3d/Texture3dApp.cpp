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

#include "Texture3dApp.h"

#include "Graphics\CommandContext.h"
#include "Graphics\CommonStates.h"
#include "Graphics\Device.h"
#include "Graphics\DeviceManager.h"

#include <numeric>
#include <random>

using namespace Luna;
using namespace Math;
using namespace std;


// TODO Move this into engine
// Translation of Ken Perlin's JAVA implementation (http://mrl.nyu.edu/~perlin/noise/)
template <typename T>
class PerlinNoise
{
private:
	uint32_t permutations[512];
	T fade(T t)
	{
		return t * t * t * (t * (t * (T)6 - (T)15) + (T)10);
	}
	T lerp(T t, T a, T b)
	{
		return a + t * (b - a);
	}
	T grad(int hash, T x, T y, T z)
	{
		// Convert LO 4 bits of hash code into 12 gradient directions
		int h = hash & 15;
		T u = h < 8 ? x : y;
		T v = h < 4 ? y : h == 12 || h == 14 ? x : z;
		return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
	}
public:
	PerlinNoise()
	{
		// Generate random lookup for permutations containing all numbers from 0..255
		vector<uint8_t> plookup;
		plookup.resize(256);
		iota(plookup.begin(), plookup.end(), 0);
		default_random_engine rndEngine(random_device{}());
		shuffle(plookup.begin(), plookup.end(), rndEngine);

		for (uint32_t i = 0; i < 256; i++)
		{
			permutations[i] = permutations[256 + i] = plookup[i];
		}
	}
	T noise(T x, T y, T z)
	{
		// Find unit cube that contains point
		int32_t X = (int32_t)floor(x) & 255;
		int32_t Y = (int32_t)floor(y) & 255;
		int32_t Z = (int32_t)floor(z) & 255;
		// Find relative x,y,z of point in cube
		x -= floor(x);
		y -= floor(y);
		z -= floor(z);

		// Compute fade curves for each of x,y,z
		T u = fade(x);
		T v = fade(y);
		T w = fade(z);

		// Hash coordinates of the 8 cube corners
		uint32_t A = permutations[X] + Y;
		uint32_t AA = permutations[A] + Z;
		uint32_t AB = permutations[A + 1] + Z;
		uint32_t B = permutations[X + 1] + Y;
		uint32_t BA = permutations[B] + Z;
		uint32_t BB = permutations[B + 1] + Z;

		// And add blended results for 8 corners of the cube;
		T res = lerp(w, lerp(v,
			lerp(u, grad(permutations[AA], x, y, z), grad(permutations[BA], x - 1, y, z)), lerp(u, grad(permutations[AB], x, y - 1, z), grad(permutations[BB], x - 1, y - 1, z))),
			lerp(v, lerp(u, grad(permutations[AA + 1], x, y, z - 1), grad(permutations[BA + 1], x - 1, y, z - 1)), lerp(u, grad(permutations[AB + 1], x, y - 1, z - 1), grad(permutations[BB + 1], x - 1, y - 1, z - 1))));
		return res;
	}
};

// Fractal noise generator based on perlin noise above
template <typename T>
class FractalNoise
{
private:
	PerlinNoise<float> perlinNoise;
	uint32_t octaves;
	T frequency;
	T amplitude;
	T persistence;
public:

	FractalNoise(const PerlinNoise<T>& perlinNoise)
	{
		this->perlinNoise = perlinNoise;
		octaves = 6;
		persistence = (T)0.5;
	}

	T noise(T x, T y, T z)
	{
		T sum = 0;
		T frequency = (T)1;
		T amplitude = (T)1;
		T max = (T)0;
		for (uint32_t i = 0; i < octaves; i++)
		{
			sum += perlinNoise.noise(x * frequency, y * frequency, z * frequency) * amplitude;
			max += amplitude;
			amplitude *= persistence;
			frequency *= (T)2;
		}

		sum = sum / max;
		return (sum + (T)1.0) / (T)2.0;
	}
};


Texture3dApp::Texture3dApp(uint32_t width, uint32_t height)
	: Application{ width, height, s_appName }
{}


int Texture3dApp::ProcessCommandLine(int argc, char* argv[])
{
	// Handle commandline arguments here
	return Application::ProcessCommandLine(argc, argv);
}


void Texture3dApp::Configure()
{
	// Application config, before device creation
	Application::Configure();
}


void Texture3dApp::Startup()
{
	// Application initialization, after device creation

	m_camera.SetPerspectiveMatrix(
		DirectX::XMConvertToRadians(60.0f),
		GetWindowAspectRatio(),
		0.1f,
		256.0f);
	m_camera.SetPosition(Vector3(0.0f, 0.0f, -m_zoom));

	m_controller.SetSpeedScale(0.025f);
	m_controller.SetCameraMode(CameraMode::ArcBall);
	m_controller.RefreshFromCamera();
	m_controller.SetOrbitTarget(Vector3(0.0f, 0.0f, 0.0f), Length(m_camera.GetPosition()), 0.25f);
}


void Texture3dApp::Shutdown()
{
	// Application cleanup on shutdown
}


void Texture3dApp::Update()
{
	// Application update tick

	m_controller.Update(m_inputSystem.get(), (float)m_timer.GetElapsedSeconds(), m_mouseMoveHandled);

	UpdateConstantBuffer();
}


void Texture3dApp::Render()
{
	// Application main render loop
	
	auto& context = GraphicsContext::Begin("Scene");

	context.TransitionResource(GetColorBuffer(), ResourceState::RenderTarget);
	context.TransitionResource(GetDepthBuffer(), ResourceState::DepthWrite);
	context.ClearColor(GetColorBuffer());
	context.ClearDepth(GetDepthBuffer());

	context.BeginRendering(GetColorBuffer(), GetDepthBuffer());

	context.SetViewportAndScissor(0u, 0u, GetWindowWidth(), GetWindowHeight());

	context.SetRootSignature(m_rootSignature);
	context.SetGraphicsPipeline(m_graphicsPipeline);

	// Bind descriptor sets
#if APP_DYNAMIC_DESCRIPTORS
	context.SetCBV(0, 0, m_constantBuffer);
	context.SetSRV(1, 0, m_texture);
#else
	context.SetDescriptors(0, m_cbvDescriptorSet);
	context.SetDescriptors(1, m_srvDescriptorSet);
#endif // APP_DYNAMIC_DESCRIPTORS

	context.SetVertexBuffer(0, m_vertexBuffer);
	context.SetIndexBuffer(m_indexBuffer);

	context.DrawIndexed((uint32_t)m_indexBuffer->GetElementCount());

	RenderUI(context);

	context.EndRendering();
	context.TransitionResource(GetColorBuffer(), ResourceState::Present);

	context.Finish();
}


void Texture3dApp::CreateDeviceDependentResources()
{
	// Create any resources that depend on the device, but not the window size

	vector<Vertex> vertexData =
	{
		{ {  1.0f,  1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 1.0f } },
		{ { -1.0f,  1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 1.0f } },
		{ { -1.0f, -1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f } },
		{ {  1.0f, -1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 0.0f } }
	};
	m_vertexBuffer = CreateVertexBuffer<Vertex>("VertexBuffer", vertexData);

	// Setup indices
	vector<uint16_t> indexData = { 0,1,2, 2,3,0 };
	m_indexBuffer = CreateIndexBuffer("Index Buffer", indexData);

	InitRootSignature();

	// Constant buffer
	m_constantBuffer = CreateConstantBuffer("Constant Buffer", 1, sizeof(Constants));

	InitTexture();
#if !APP_DYNAMIC_DESCRIPTORS
	InitDescriptorSets();
#endif // !APP_DYNAMIC_DESCRIPTORS
}


void Texture3dApp::CreateWindowSizeDependentResources()
{
	// Create any resources that depend on window size.  May be called when the window size changes.
	if (!m_pipelineCreated)
	{
		InitPipeline();
		m_pipelineCreated = true;
	}

	// Update the camera since the aspect ration might have changed
	m_camera.SetPerspectiveMatrix(
		DirectX::XMConvertToRadians(60.0f),
		GetWindowAspectRatio(),
		0.1f,
		256.0f);
}


void Texture3dApp::InitRootSignature()
{
	auto rootSignatureDesc = RootSignatureDesc{
		.name				= "Root Signature",
		.rootParameters		= {
			Table({ ConstantBuffer }, ShaderStage::Vertex),
			Table({ TextureSRV }, ShaderStage::Pixel)
		},
		.staticSamplers		= { StaticSampler(CommonStates::SamplerLinearWrap()) }
	};

	m_rootSignature = CreateRootSignature(rootSignatureDesc);
}


void Texture3dApp::InitPipeline()
{
	VertexStreamDesc vertexStreamDesc{
		.inputSlot = 0,
		.stride = sizeof(Vertex),
		.inputClassification = InputClassification::PerVertexData
	};

	vector<VertexElementDesc> vertexElements{
		{ "POSITION", 0, Format::RGB32_Float, 0, offsetof(Vertex, position), InputClassification::PerVertexData, 0 },
		{ "NORMAL", 0, Format::RGB32_Float, 0, offsetof(Vertex, normal), InputClassification::PerVertexData, 0 },
		{ "TEXCOORD", 0, Format::RG32_Float, 0, offsetof(Vertex, uv), InputClassification::PerVertexData, 0 }
	};

	GraphicsPipelineDesc skyBoxDesc
	{
		.name				= "Graphics PSO",
		.blendState			= CommonStates::BlendDisable(),
		.depthStencilState	= CommonStates::DepthStateReadOnlyReversed(),
		.rasterizerState	= CommonStates::RasterizerTwoSided(),
		.rtvFormats			= { GetColorFormat() },
		.dsvFormat			= GetDepthFormat(),
		.topology			= PrimitiveTopology::TriangleList,
		.vertexShader		= { .shaderFile = "Texture3dVS" },
		.pixelShader		= { .shaderFile = "Texture3dPS" },
		.vertexStreams		= { vertexStreamDesc },
		.vertexElements		= vertexElements,
		.rootSignature		= m_rootSignature
	};

	m_graphicsPipeline = CreateGraphicsPipeline(skyBoxDesc);
}


void Texture3dApp::InitTexture()
{
	uint32_t width = 64;
	uint32_t height = 64;
	uint32_t depth = 64;

	unique_ptr<std::byte[]> data;
	data.reset(new std::byte[width * height * depth]);

	PerlinNoise<float> perlinNoise;
	FractalNoise<float> fractalNoise(perlinNoise);

	default_random_engine rndEngine(random_device{}());
	const int32_t noiseType = rand() % 2;
	const float noiseScale = static_cast<float>(rand() % 10) + 4.0f;

	for (uint32_t z = 0; z < depth; z++)
	{
		for (uint32_t y = 0; y < height; y++)
		{
			for (uint32_t x = 0; x < width; x++)
			{
				float nx = (float)x / (float)width;
				float ny = (float)y / (float)height;
				float nz = (float)z / (float)depth;
#define FRACTAL
#ifdef FRACTAL
				float n = fractalNoise.noise(nx * noiseScale, ny * noiseScale, nz * noiseScale);
#else
				float n = 20.0 * perlinNoise.noise(nx, ny, nz);
#endif
				n = n - floor(n);

				data[x + y * width + z * width * height] = static_cast<std::byte>(floor(n * 255));
			}
		}
	}

	// Create the texture
	TextureDesc desc{
		.name		= "Noise Texture",
		.width		= width,
		.height		= height,
		.depth		= depth,
		.format		= Format::R8_UNorm,
		.dataSize	= width * height * depth,
		.data		= data.get()
	};

	auto device = m_deviceManager->GetDevice();
	m_texture = device->CreateTexture3D(desc);
}


#if !APP_DYNAMIC_DESCRIPTORS
void Texture3dApp::InitDescriptorSets()
{
	m_cbvDescriptorSet = m_rootSignature->CreateDescriptorSet(0);
	m_srvDescriptorSet = m_rootSignature->CreateDescriptorSet(1);

	m_cbvDescriptorSet->SetCBV(0, m_constantBuffer);
	m_srvDescriptorSet->SetSRV(0, m_texture);
}
#endif // !APP_DYNAMIC_DESCRIPTORS


void Texture3dApp::UpdateConstantBuffer()
{
	Matrix4 modelMatrix = Matrix4(kIdentity);

	m_constants.viewProjectionMatrix = m_camera.GetViewProjectionMatrix();
	m_constants.modelMatrix = modelMatrix;
	m_constants.viewPos = Vector4(m_camera.GetPosition(), 0.0f);
	m_constants.depth += (float)m_timer.GetElapsedSeconds() * 0.15f;
	if (m_constants.depth > 1.0f)
	{
		m_constants.depth -= 1.0f;
	}
	m_constantBuffer->Update(sizeof(m_constants), &m_constants);
}