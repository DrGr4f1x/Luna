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

enum class GraphicsApi
{
	Unknown,
	D3D12,
	Vulkan
};

inline std::string GraphicsApiToString(GraphicsApi graphicsApi)
{
	switch (graphicsApi)
	{
	case GraphicsApi::D3D12:
		return "D3D12";
	case GraphicsApi::Vulkan:
		return "Vulkan";
	default:
		return "Unknown";
	}
}

} // namespace Luna