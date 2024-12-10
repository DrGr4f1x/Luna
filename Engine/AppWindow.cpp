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

#include "AppWindow.h"

#include "Application.h"

#include <winrt/Windows.UI.Input.h>

using namespace winrt::Windows::ApplicationModel;
using namespace winrt::Windows::ApplicationModel::Activation;
using namespace winrt::Windows::ApplicationModel::Core;
using namespace winrt::Windows::Graphics::Display;
using namespace winrt::Windows::UI::Core;
using namespace winrt::Windows::UI::Input;


namespace Luna
{

AppWindow::AppWindow(Application* pApplication)
	: m_pApplication{ pApplication }
{
}


IFrameworkView AppWindow::CreateView()
{
	return *this;
}


void AppWindow::Initialize(const CoreApplicationView& applicationView)
{
	applicationView.Activated({ this, &AppWindow::OnActivated });

	CoreApplication::Suspending({ this, &AppWindow::OnSuspending });

	CoreApplication::Resuming({ this, &AppWindow::OnResuming });

	// At this point we have access to the device. 
	// We can create the device-dependent resources.
	
	// TODO
	// m_deviceResources = std::make_shared<DX::DeviceResources>();

	m_pApplication->Initialize();
}


void AppWindow::Load(const winrt::hstring& entryPoint)
{ 
	// TODO
	// if (!m_main)
	// {
	//     m_main = winrt::make_self<GameMain>(m_deviceResources);
	// }
}


void AppWindow::Run()
{ 
	m_pApplication->Run();
}


void AppWindow::SetWindow(const CoreWindow& window)
{
	window.PointerCursor(CoreCursor(CoreCursorType::Arrow, 0));

	PointerVisualizationSettings visualizationSettings{ PointerVisualizationSettings::GetForCurrentView() };
	visualizationSettings.IsContactFeedbackEnabled(false);
	visualizationSettings.IsBarrelButtonFeedbackEnabled(false);

	// TODO
	// m_deviceResources->SetWindow(window);

	window.Activated({ this, &AppWindow::OnWindowActivationChanged });

	window.SizeChanged({ this, &AppWindow::OnWindowSizeChanged });

	window.Closed({ this, &AppWindow::OnWindowClosed });

	window.VisibilityChanged({ this, &AppWindow::OnVisibilityChanged });

	DisplayInformation currentDisplayInformation{ DisplayInformation::GetForCurrentView() };

	currentDisplayInformation.DpiChanged({ this, &AppWindow::OnDpiChanged });

	currentDisplayInformation.OrientationChanged({ this, &AppWindow::OnOrientationChanged });

	currentDisplayInformation.StereoEnabledChanged({ this, &AppWindow::OnStereoEnabledChanged });

	DisplayInformation::DisplayContentsInvalidated({ this, &AppWindow::OnDisplayContentsInvalidated });
}


void AppWindow::OnActivated(const CoreApplicationView& applicationView, const IActivatedEventArgs& args)
{
	CoreWindow window = CoreWindow::GetForCurrentThread();
	window.Activate();
}


void AppWindow::OnVisibilityChanged(const CoreWindow& sender, const VisibilityChangedEventArgs& args)
{
	// TODO
	// m_main->Visibility(args.Visible());
}


void AppWindow::OnWindowActivationChanged(const CoreWindow& sender, const WindowActivatedEventArgs& args)
{
	// TODO
	// m_main->WindowActivationChanged(args.WindowActivationState());
}


winrt::fire_and_forget AppWindow::OnSuspending(const IInspectable& sender, const SuspendingEventArgs& args)
{
	auto lifetime = get_strong();

	// Save app state asynchronously after requesting a deferral. Holding a deferral
	// indicates that the application is busy performing suspending operations. Be
	// aware that a deferral may not be held indefinitely. After about five seconds,
	// the app will be forced to exit.
	SuspendingDeferral deferral = args.SuspendingOperation().GetDeferral();

	co_await winrt::resume_background();

	// TODO
	// m_deviceResources->Trim();
	// m_main->Suspend();

	deferral.Complete();
}


void AppWindow::OnResuming(const IInspectable& sender, const IInspectable& args)
{
	// Restore any data or state that was unloaded on suspend. By default, data
	// and state are persisted when resuming from suspend. Note that this event
	// does not occur if the app was previously terminated.

	// TODO
	// m_main->Resume();
}


void AppWindow::OnStereoEnabledChanged(const DisplayInformation& sender, const IInspectable& args)
{
	// TODO
	// m_deviceResources->UpdateStereoState();
	// m_main->CreateWindowSizeDependentResources();
}


void AppWindow::OnWindowSizeChanged(const CoreWindow& window, const WindowSizeChangedEventArgs& args)
{
	// TODO
	// m_deviceResources->SetLogicalSize(args.Size());
	// m_main->CreateWindowSizeDependentResources();
}


void AppWindow::OnWindowClosed(const CoreWindow& sender, const CoreWindowEventArgs& args)
{
	m_pApplication->Close();
}


void AppWindow::OnDpiChanged(const DisplayInformation& sender, const IInspectable& args)
{
	// TODO
	// m_deviceResources->SetDpi(sender.LogicalDpi());
	// m_main->CreateWindowSizeDependentResources();
}


void AppWindow::OnOrientationChanged(const DisplayInformation& sender, const IInspectable& args)
{
	// TODO
	// m_deviceResources->SetCurrentOrientation(sender.CurrentOrientation());
	// m_main->CreateWindowSizeDependentResources();
}


void AppWindow::OnDisplayContentsInvalidated(const DisplayInformation& sender, const IInspectable& args)
{
	// TODO
	// m_deviceResources->ValidateDevice();
}

} // namespace Luna