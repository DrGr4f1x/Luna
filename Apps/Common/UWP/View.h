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

using namespace Platform;
using namespace Windows::ApplicationModel;
using namespace Windows::ApplicationModel::Activation;
using namespace Windows::ApplicationModel::Core;
using namespace Windows::UI::Core;
using namespace Windows::UI::ViewManagement;

namespace Luna
{

class IApplication;

ref class View sealed : public IFrameworkView
{
public:
	View(UINT_PTR pApplication);

	virtual void Initialize(CoreApplicationView^ applicationView);
	virtual void SetWindow(CoreWindow^ window);
	virtual void Load(String^ entryPoint);
	virtual void Run();
	virtual void Uninitialize();

private:
	void OnActivated(CoreApplicationView^ applicationView, IActivatedEventArgs^ args);
	void OnKeyDown(CoreWindow^ window, KeyEventArgs^ keyEventArgs);
	void OnKeyUp(CoreWindow^ window, KeyEventArgs^ keyEventArgs);
	void OnClosed(CoreWindow^ sender, CoreWindowEventArgs^ args);

	IApplication* m_pApplication{ nullptr };
	bool m_bWindowClosed{ false };
};

} // namespace Luna