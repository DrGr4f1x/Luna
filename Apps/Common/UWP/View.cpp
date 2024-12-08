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

#include "View.h"

#include "..\..\..\Engine\Application.h"

using namespace Windows::Foundation;

namespace Luna
{

View::View(UINT_PTR pApplication) 
	: m_pApplication{ reinterpret_cast<IApplication*>(pApplication) }
	, m_bWindowClosed{ false }
{}


void View::Initialize(CoreApplicationView^ applicationView)
{
	applicationView->Activated += ref new TypedEventHandler<CoreApplicationView^, IActivatedEventArgs^>(this, &View::OnActivated);

	// For simplicity, this sample ignores CoreApplication's Suspend and Resume
	// events which a typical app should subscribe to.
}


void View::SetWindow(CoreWindow^ window)
{
	window->KeyDown += ref new TypedEventHandler<CoreWindow^, KeyEventArgs^>(this, &View::OnKeyDown);
	window->KeyUp += ref new TypedEventHandler<CoreWindow^, KeyEventArgs^>(this, &View::OnKeyUp);
	window->Closed += ref new TypedEventHandler<CoreWindow^, CoreWindowEventArgs^>(this, &View::OnClosed);

	// For simplicity, this sample ignores a number of events on CoreWindow that a
	// typical app should subscribe to.
}


void View::Load(String^ /*entryPoint*/)
{
}


void View::Run()
{
	auto applicationView = ApplicationView::GetForCurrentView();
	applicationView->Title = ref new Platform::String(m_pApplication->GetWTitle().c_str());

	m_pApplication->OnInit();

	while (!m_bWindowClosed)
	{
		CoreWindow::GetForCurrentThread()->Dispatcher->ProcessEvents(CoreProcessEventsOption::ProcessAllIfPresent);

		m_pApplication->OnUpdate();
		m_pApplication->OnRender();
	}

	m_pApplication->OnDestroy();
}


void View::Uninitialize()
{
}


void View::OnActivated(CoreApplicationView^ /*applicationView*/, IActivatedEventArgs^ args)
{
	CoreWindow::GetForCurrentThread()->Activate();
}


void View::OnKeyDown(CoreWindow^ /*window*/, KeyEventArgs^ args)
{
	if (static_cast<UINT>(args->VirtualKey) < 256)
	{
		m_pApplication->OnKeyDown(static_cast<UINT8>(args->VirtualKey));
	}
}


void View::OnKeyUp(CoreWindow^ /*window*/, KeyEventArgs^ args)
{
	if (static_cast<UINT>(args->VirtualKey) < 256)
	{
		m_pApplication->OnKeyUp(static_cast<UINT8>(args->VirtualKey));
	}
}

void View::OnClosed(CoreWindow^ /*sender*/, CoreWindowEventArgs^ /*args*/)
{
	m_bWindowClosed = true;
}

} // namespace Luna