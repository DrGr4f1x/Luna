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

class IHandlePool;

class __declspec(uuid("18F28EAE-2263-4957-AE67-A8043A83B15C")) HandleType : public RefCounted<HandleType>
{
public:
	HandleType(uint32_t index, uint32_t generation, IHandlePool* pool)
		: m_index{ index }
		, m_generation{ generation }
		, m_pool{ pool }
	{}

	uint32_t GetIndex() const { return m_index; }
	uint32_t GetGeneration() const { return m_index; }

private:
	uint32_t m_index : 16;
	uint32_t m_generation : 16;
	IHandlePool* m_pool{ nullptr };
};

using Handle = wil::com_ptr<HandleType>;


class IHandlePool
{
public:
	virtual void FreeHandle(HandleType* handle) = 0;
};

} // namespace Luna