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

// Forward declarations
class IResourceManager;


//class __declspec(uuid("76EB2254-E7A6-4F2D-9037-A0FE41926CE2")) ResourceHandleType : public RefCounted<ResourceHandleType>
class ResourceHandleType
{
public:
	ResourceHandleType(uint32_t index, uint32_t type, IResourceManager* const manager)
		: m_index{ index }
		, m_type{ type }
		, m_manager{ manager }
	{}

	~ResourceHandleType();

	uint32_t GetIndex() const { return m_index; }
	uint32_t GetType() const { return m_type; }

private:
	const uint32_t m_index{ 0 };
	const uint32_t m_type{ 0 };
	IResourceManager* const m_manager{ nullptr };
};

using ResourceHandle = std::shared_ptr<ResourceHandleType>;

} // namespace Luna