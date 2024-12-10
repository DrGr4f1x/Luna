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

#include <unknwn.h>
#include <winrt/Windows.ApplicationModel.Core.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Graphics.Display.h>
#include <winrt/Windows.UI.Core.h>

namespace Luna
{

class Application;


struct AppWindow : winrt::implements<AppWindow, winrt::Windows::ApplicationModel::Core::IFrameworkViewSource, winrt::Windows::ApplicationModel::Core::IFrameworkView>
{
	AppWindow(Application* pApplication);

	winrt::Windows::ApplicationModel::Core::IFrameworkView CreateView();

	void Initialize(const winrt::Windows::ApplicationModel::Core::CoreApplicationView& applicationView);

	void Load(const winrt::hstring& /* entryPoint */);

	void Uninitialize()
	{
	}

	void Run();

	void SetWindow(const winrt::Windows::UI::Core::CoreWindow& window);

	void OnActivated(const winrt::Windows::ApplicationModel::Core::CoreApplicationView& applicationView, const winrt::Windows::ApplicationModel::Activation::IActivatedEventArgs& args);
	
	void OnVisibilityChanged(const winrt::Windows::UI::Core::CoreWindow& sender, const winrt::Windows::UI::Core::VisibilityChangedEventArgs& args);
	
	void OnWindowActivationChanged(const winrt::Windows::UI::Core::CoreWindow& sender, const winrt::Windows::UI::Core::WindowActivatedEventArgs& args);
	
	winrt::fire_and_forget OnSuspending(const IInspectable& sender, const winrt::Windows::ApplicationModel::SuspendingEventArgs& args);
	
	void OnResuming(const IInspectable& sender, const IInspectable& args);
	
	void OnStereoEnabledChanged(const winrt::Windows::Graphics::Display::DisplayInformation& sender, const IInspectable& args);
	
	void OnWindowSizeChanged(const winrt::Windows::UI::Core::CoreWindow& window, const winrt::Windows::UI::Core::WindowSizeChangedEventArgs& args);
	
	void OnWindowClosed(const winrt::Windows::UI::Core::CoreWindow& sender, const winrt::Windows::UI::Core::CoreWindowEventArgs& args);
	
	void OnDpiChanged(const winrt::Windows::Graphics::Display::DisplayInformation& sender, const IInspectable& args);
	
	void OnOrientationChanged(const winrt::Windows::Graphics::Display::DisplayInformation& sender, const IInspectable& args);
	
	void OnDisplayContentsInvalidated(const winrt::Windows::Graphics::Display::DisplayInformation& sender, const IInspectable& args);
	
private:
	Application* m_pApplication{ nullptr };
};

} // namespace Luna
