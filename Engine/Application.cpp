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

#include "Application.h"

#include "FileSystem.h"
#include "InputSystem.h"

#include "Graphics\CommandContext.h"
#include "Graphics\Device.h"
#include "Graphics\DeviceManager.h"

#include <glfw\glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32 1
#include <glfw\glfw3native.h>

#include "imgui.h"
#include <CLI11\CLI11.hpp>

#pragma comment(lib, "runtimeobject.lib")

using namespace std;


namespace 
{

// GLFW event callbacks

void GlfwErrorCallback(int errorCode, const char* description)
{
	using namespace Luna;
	LogError(LogGlfw) << description << endl;
}


void GlfwWindowIconifyCallback(GLFWwindow* pWindow, int iconified)
{
	using namespace Luna;
	auto pApplication = reinterpret_cast<Application*>(glfwGetWindowUserPointer(pWindow));
	pApplication->OnWindowIconify(iconified);
}


void GlfwWindowFocusCallback(GLFWwindow* pWindow, int focused)
{
	using namespace Luna;
	auto pApplication = reinterpret_cast<Application*>(glfwGetWindowUserPointer(pWindow));
	pApplication->OnWindowFocus(focused);
}


void GlfwWindowRefreshCallback(GLFWwindow* pWindow)
{
	using namespace Luna;
	auto pApplication = reinterpret_cast<Application*>(glfwGetWindowUserPointer(pWindow));
	pApplication->OnWindowRefresh();
}


void GlfwWindowCloseCallback(GLFWwindow* pWindow)
{
	using namespace Luna;
	auto pApplication = reinterpret_cast<Application*>(glfwGetWindowUserPointer(pWindow));
	pApplication->OnWindowClose();
}

void GlfwWindowPositionCallback(GLFWwindow* pWindow, int xPos, int yPos)
{
	using namespace Luna;
	auto pApplication = reinterpret_cast<Application*>(glfwGetWindowUserPointer(pWindow));
	pApplication->OnWindowPosition(xPos, yPos);
}

void GlfwMousePositionCallback(GLFWwindow* pWindow, double xPos, double yPos)
{
	using namespace Luna;
	auto pApplication = reinterpret_cast<Application*>(glfwGetWindowUserPointer(pWindow));
	pApplication->OnMousePosition((uint32_t)xPos, (uint32_t)yPos);
}

} // anonymous namespace


namespace Luna
{
static bool g_isFrameProRunning = false;

FrameProCloser::~FrameProCloser()
{
	FRAMEPRO_SHUTDOWN();
	g_isFrameProRunning = false;
}


bool IsFrameProRunning()
{
	return g_isFrameProRunning;
}


Application::Application(uint32_t width, uint32_t height, const string& appTitle)
{
	m_appInfo.SetName(appTitle);
	m_appInfo.SetWidth(width);
	m_appInfo.SetHeight(height);
}


Application::~Application()
{}


int Application::ProcessCommandLine(int argc, char* argv[])
{
	CLI::App app{ "Luna App", m_appInfo.name };
	
	// Graphic API selection
	bool bDX12{ false };
	bool bVulkan{ false };
	auto dxOpt = app.add_flag("--dx,--dx12,--d3d12", bDX12, "Select DirectX renderer");
	auto vkOpt = app.add_flag("--vk,--vulkan", bVulkan, "Select Vulkan renderer");
	dxOpt->excludes(vkOpt);
	vkOpt->excludes(dxOpt);

	// Width, height
	auto widthOpt = app.add_option("--resx,--width", m_appInfo.width, "Sets initial window width");
	auto heightOpt = app.add_option("--resy,--height", m_appInfo.height, "Sets initial window height");

	// Parse command line
	CLI11_PARSE(app, argc, argv);

	// Set application parameters from command line
	m_appInfo.api = bVulkan ? GraphicsApi::Vulkan : GraphicsApi::D3D12;
	m_appNameWithApi = format("[{}] {}", GraphicsApiToString(m_appInfo.api), m_appInfo.name);

	return 0;
}


void Application::Configure()
{ 
	// Setup file system
	SetDefaultRootPath();
	SetDefaultSearchPaths();
}


void Application::Render()
{
	ScopedEvent event{ "Application::Render" };

	auto& context = GraphicsContext::Begin("Frame");

	auto colorBuffer = GetColorBuffer();
	context.TransitionResource(colorBuffer, ResourceState::RenderTarget);
	context.ClearColor(colorBuffer, DirectX::Colors::CornflowerBlue);

	// Rendering code goes here

	context.TransitionResource(colorBuffer, ResourceState::Present);
	context.Finish();
}


void Application::OnWindowIconify(int iconified)
{ }


void Application::OnWindowFocus(int focused)
{ }


void Application::OnWindowRefresh()
{ }


void Application::OnWindowClose()
{ }


void Application::OnWindowPosition(int xPos, int yPos)
{ }


void Application::OnMousePosition(uint32_t x, uint32_t y)
{
	m_mouseX = x;
	m_mouseY = y;

	if (m_showUI)
	{
		ImGuiIO& io = ImGui::GetIO();
		m_mouseMoveHandled = io.WantCaptureMouse;
	}
}


void Application::Run()
{
	Microsoft::WRL::Wrappers::RoInitializeWrapper InitializeWinRT(RO_INIT_MULTITHREADED);
	assert_succeeded(InitializeWinRT);

	m_hinst = GetModuleHandle(nullptr);

	if (!Initialize())
	{
		return;
	}

	while (m_isRunning && !glfwWindowShouldClose(m_pWindow))
	{
		FRAMEPRO_FRAME_START();
		if (!g_isFrameProRunning)
		{
			g_isFrameProRunning = true;
		}

		glfwPollEvents();

		UpdateWindowSize();

		m_isRunning = Tick();
	}

	m_deviceManager->WaitForGpu();

	Finalize();
}


ColorBufferPtr Application::GetColorBuffer() const
{
	return m_deviceManager->GetColorBuffer();
}


void Application::SetDefaultRootPath()
{
	if (m_fileSystem)
	{
		m_fileSystem->SetDefaultRootPath();
	}
}


void Application::SetDefaultSearchPaths()
{
	if (m_fileSystem)
	{
		m_fileSystem->AddSearchPath("Data");
		m_fileSystem->AddSearchPath("Data\\Shaders");
		m_fileSystem->AddSearchPath("Data\\Shaders\\DXIL");
		m_fileSystem->AddSearchPath("Data\\Shaders\\SPIRV");
		m_fileSystem->AddSearchPath("Data\\Shaders\\DXBC");
		m_fileSystem->AddSearchPath("Data\\Textures");
		m_fileSystem->AddSearchPath("Data\\Models");
		m_fileSystem->AddSearchPath("..\\Data");
		m_fileSystem->AddSearchPath("..\\Data\\Shaders");
		m_fileSystem->AddSearchPath("..\\Data\\Shaders\\DXIL");
		m_fileSystem->AddSearchPath("..\\Data\\Shaders\\SPIRV");
		m_fileSystem->AddSearchPath("..\\Data\\Shaders\\DXBC");
		m_fileSystem->AddSearchPath("..\\Data\\Textures");
		m_fileSystem->AddSearchPath("..\\Data\\Models");
	}
}


void Application::UpdateWindowSize()
{
	int width{ 0 };
	int height{ 0 };
	glfwGetWindowSize(m_pWindow, &width, &height);

	if (width == 0 || height == 0)
	{
		m_isVisible = false;
		return;
	}

	m_isVisible = true;
	m_isWindowFocused = glfwGetWindowAttrib(m_pWindow, GLFW_FOCUSED) == 1;

	if ((int)m_appInfo.width != width || (int)m_appInfo.height != height)
	{
		m_appInfo.width = (uint32_t)width;
		m_appInfo.height = (uint32_t)height;
		
		if (m_deviceManager)
		{
			m_deviceManager->SetWindowSize(m_appInfo.width, m_appInfo.height);

			CreateWindowSizeDependentResources();
		}

		if (m_uiOverlay)
		{
			m_uiOverlay->SetWindowSize(m_appInfo.width, m_appInfo.height);
		}

		if (m_grid)
		{
			m_grid->CreateWindowSizeDependentResources();
		}
	}
}


Format Application::GetColorFormat()
{
	return m_deviceManager->GetColorFormat();
}


Format Application::GetDepthFormat()
{
	return m_deviceManager->GetDepthFormat();
}


void Application::PrepareUI()
{
	if (!m_showUI)
		return;

	ScopedEvent event("PrepareUI");

	ImGuiIO& io = ImGui::GetIO();

	io.DisplaySize = ImVec2((float)GetWindowWidth(), (float)GetWindowHeight());
	io.DeltaTime = (float)m_timer.GetElapsedSeconds();

	io.MousePos = ImVec2((float)m_mouseX, (float)m_mouseY);
	io.MouseDown[0] = m_inputSystem->IsPressed(DigitalInput::kMouse0);
	io.MouseDown[1] = m_inputSystem->IsPressed(DigitalInput::kMouse1);

	ImGui::NewFrame();

	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0);
	ImGui::SetNextWindowPos(ImVec2(10, 10));
	ImGui::SetNextWindowSize(ImVec2(0, 0), ImGuiCond_FirstUseEver);
	string caption = m_appNameWithApi;
	ImGui::Begin(caption.c_str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
	ImGui::TextUnformatted(m_appInfo.name.c_str());
	ImGui::TextUnformatted(m_deviceManager->GetDeviceName().c_str());
	ImGui::Text("%.2f ms/frame (%.1d fps)", (1000.0f / m_timer.GetFramesPerSecond()), m_timer.GetFramesPerSecond());

	ImGui::PushItemWidth(110.0f * m_uiOverlay->GetScale());
	UpdateUI();
	ImGui::PopItemWidth();

	ImGui::End();
	ImGui::PopStyleVar();
	ImGui::Render();

	m_uiOverlay->Update();
}


void Application::RenderUI(GraphicsContext& context)
{
	if (!m_showUI)
		return;

	m_uiOverlay->Render(context);
}


void Application::RenderGrid(GraphicsContext& context)
{
	if (!m_showGrid)
		return;

	m_grid->Render(context);
}


ColorBufferPtr Application::CreateColorBuffer(const ColorBufferDesc& colorBufferDesc)
{ 
	return m_deviceManager->GetDevice()->CreateColorBuffer(colorBufferDesc);
}


DepthBufferPtr Application::CreateDepthBuffer(const DepthBufferDesc& depthBufferDesc)
{
	return m_deviceManager->GetDevice()->CreateDepthBuffer(depthBufferDesc);
}


GpuBufferPtr Application::CreateGpuBuffer(const GpuBufferDesc& gpuBufferDesc)
{
	return m_deviceManager->GetDevice()->CreateGpuBuffer(gpuBufferDesc);
}


RootSignaturePtr Application::CreateRootSignature(const RootSignatureDesc& rootSignatureDesc)
{
	return m_deviceManager->GetDevice()->CreateRootSignature(rootSignatureDesc);
}


GraphicsPipelinePtr Application::CreateGraphicsPipeline(const GraphicsPipelineDesc& pipelineDesc)
{
	return m_deviceManager->GetDevice()->CreateGraphicsPipeline(pipelineDesc);
}


ComputePipelinePtr Application::CreateComputePipeline(const ComputePipelineDesc& pipelineDesc)
{
	return m_deviceManager->GetDevice()->CreateComputePipeline(pipelineDesc);
}


QueryHeapPtr Application::CreateQueryHeap(const QueryHeapDesc& queryHeapDesc)
{
	return m_deviceManager->GetDevice()->CreateQueryHeap(queryHeapDesc);
}


SamplerPtr Application::CreateSampler(const SamplerDesc& samplerDesc)
{
	return m_deviceManager->GetDevice()->CreateSampler(samplerDesc);
}


TexturePtr Application::LoadTexture(const std::string& filename, Format format, bool forceSrgb, bool retainData)
{
	return GetTextureManager()->Load(filename, format, forceSrgb, retainData);
}


ModelPtr Application::LoadModel(const std::string& filename, const VertexLayoutBase& layout, float scale, ModelLoad loadFlags, bool loadMaterials)
{
	return Luna::LoadModel(m_deviceManager->GetDevice(), filename, layout, scale, loadFlags, loadMaterials);
}


bool Application::Initialize()
{
	// Create core engine systems
	m_fileSystem = make_unique<FileSystem>(m_appInfo.name);
	m_fileSystem->SetDefaultRootPath();
	m_logSystem = make_unique<LogSystem>();

	// This is the first place we can post a startup message
	LogInfo(LogApplication) << "App " << m_appInfo.name << " initializing" << endl;

	// Application setup before device creation
	Configure();

	if (!CreateAppWindow())
	{
		return false;
	}

	m_hwnd = glfwGetWin32Window(m_pWindow);
	m_inputSystem = make_unique<InputSystem>(m_hwnd);

	CreateDeviceManager();
	CreateDeviceDependentResources();

	m_grid = make_unique<Grid>();
	m_grid->CreateDeviceDependentResources();

	UpdateWindowSize();

	m_deviceManager->CreateWindowSizeDependentResources();
	CreateWindowSizeDependentResources();
	m_grid->CreateWindowSizeDependentResources();

	m_uiOverlay = make_unique<UIOverlay>();
	m_uiOverlay->Startup(m_pWindow, m_appInfo.api, GetWindowWidth(), GetWindowHeight(), GetColorFormat(), GetDepthFormat());

	Startup();

	m_isRunning = true;

	return true;
}


void Application::Finalize()
{
	Shutdown();

	m_uiOverlay->Shutdown();

	glfwDestroyWindow(m_pWindow);
	m_pWindow = nullptr;
}


bool Application::Tick()
{
	if (!m_isRunning)
	{
		return false;
	}

	ScopedEvent event{ "Application::Tick" };

	m_inputSystem->Update((float)m_timer.GetElapsedSeconds());

	// Close on Escape key
	if (m_inputSystem->IsFirstPressed(DigitalInput::kKey_escape))
	{
		return false;
	}

	// Check toggle UI with '\'
	if (m_inputSystem->IsFirstPressed(DigitalInput::kKey_backslash))
		m_showUI = !m_showUI;

	// Tick the timer and update
	uint32_t frameCount = m_timer.GetFrameCount();
	m_timer.Tick([&]() 
		{ 
			ScopedEvent event("Update");
			
			m_grid->Update(m_camera);

			Update(); 
		});

	// Render Frame
	{
		ScopedEvent event("Frame");

		m_deviceManager->BeginFrame();
		Render();
		m_deviceManager->Present();
	}

	if ((frameCount % 1000) == 0)
	{
		string windowTitle = format("{} - {} fps", m_appNameWithApi, m_timer.GetFramesPerSecond());
		glfwSetWindowTitle(m_pWindow, windowTitle.c_str());
	}

	PrepareUI();

	return m_isRunning;
}


bool Application::CreateAppWindow()
{
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	m_pWindow = glfwCreateWindow(m_appInfo.width, m_appInfo.height, m_appInfo.name.c_str(), nullptr, nullptr);

	if (m_pWindow == nullptr)
	{
		return false;
	}

	glfwSetWindowUserPointer(m_pWindow, this);
	glfwSetWindowIconifyCallback(m_pWindow, GlfwWindowIconifyCallback);
	glfwSetWindowFocusCallback(m_pWindow, GlfwWindowFocusCallback);
	glfwSetWindowRefreshCallback(m_pWindow, GlfwWindowRefreshCallback);
	glfwSetWindowCloseCallback(m_pWindow, GlfwWindowCloseCallback);
	glfwSetWindowPosCallback(m_pWindow, GlfwWindowPositionCallback);
	glfwSetCursorPosCallback(m_pWindow, GlfwMousePositionCallback);

	return true;
}


void Application::CreateDeviceManager()
{
	DeviceManagerDesc deviceManagerDesc{
		.appName				= m_appInfo.name,
		.graphicsApi			= m_appInfo.api,
		.enableValidation		= m_appInfo.useValidation,
		.enableDebugMarkers		= m_appInfo.useDebugMarkers,
		.backBufferWidth		= m_appInfo.width,
		.backBufferHeight		= m_appInfo.height,
		.hwnd					= m_hwnd,
		.hinstance				= m_hinst
	};

	m_deviceManager = Luna::CreateDeviceManager(deviceManagerDesc);
	m_deviceManager->CreateDeviceResources();
}


int Run(Application* pApplication)
{
	glfwSetErrorCallback(GlfwErrorCallback);

	if (!glfwInit())
	{
		return -1;
	}

	pApplication->Run();

	glfwTerminate();

	return 0;
}

} // namespace Luna