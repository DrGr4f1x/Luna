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

using namespace Windows::ApplicationModel::Core;

namespace Luna
{

ref class ViewProvider sealed : IFrameworkViewSource
{
public:
	ViewProvider(UINT_PTR pApplication);
	virtual IFrameworkView^ CreateView();

private:
	UINT_PTR m_pApplication;
};

} // namespace Luna