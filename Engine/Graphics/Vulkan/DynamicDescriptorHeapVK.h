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

class RootSignature;

} // namespace Luna


namespace Luna::VK
{

class IDynamicDescriptorHeap
{
public:
	virtual void CleanupUsedHeaps(uint64_t fenceValue) = 0;

	virtual void ParseGraphicsRootSignature(const RootSignature& rootSignature) = 0;
	virtual void ParseComputeRootSignature(const RootSignature& rootSignature) = 0;
};

} // namespace Luna::VK