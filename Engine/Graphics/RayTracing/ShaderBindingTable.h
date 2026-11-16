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

struct StridedAddressRegion
{
	uint64_t bufferAddress{ 0 };
	size_t stride{ 0 };
	size_t size{ 0 };
};


class IShaderBindingTable
{
public:
	virtual ~IShaderBindingTable() = default;

	virtual StridedAddressRegion GetRayGenerationShaderRecord() const = 0;
	virtual StridedAddressRegion GetMissShaderTable() const = 0;
	virtual StridedAddressRegion GetHitGroupTable() const = 0;
	virtual StridedAddressRegion GetCallableShaderTable() const = 0;
};

using ShaderBindingTablePtr = std::shared_ptr<IShaderBindingTable>;

} // namespace Luna