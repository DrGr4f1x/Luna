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

namespace Luna
{

class __declspec(uuid("DBECDD70-7F0B-4C9B-ADFA-048104E474C8")) GraphicsDevice : public IUnknown
{
public:
	virtual ~GraphicsDevice() = default;

	virtual void WaitForGpu() = 0;
};
using DeviceHandle = Microsoft::WRL::ComPtr<GraphicsDevice>;

} // namespace Luna
