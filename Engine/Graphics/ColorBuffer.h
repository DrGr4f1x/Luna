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

#include "Graphics\PixelBuffer.h"

namespace Luna
{

class __declspec(uuid("F49A3931-9E4F-4B90-8BFD-B912A13ED31E")) IColorBuffer : public IPixelBuffer
{
public:
	virtual ~IColorBuffer() = default;

	virtual void SetClearColor(Color clearColor) noexcept = 0;
	virtual Color GetClearColor() const noexcept = 0;
};

} // namespace Luna