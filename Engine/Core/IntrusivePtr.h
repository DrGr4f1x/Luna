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

template <typename T>
class IntrusivePtr
{
public:
	using InterfaceType = T;

protected:
	InterfaceType* m_ptr{ nullptr };

	void InternalAddRef() const noexcept
	{
		if (m_ptr != nullptr)
		{
			m_ptr->AddRef();
		}
	}


	unsigned long InternalRelease() noexcept
	{
		unsigned long res{ 0 };
		T* temp = m_ptr;

		if (temp != nullptr)
		{
			m_ptr = nullptr;
			res = temp->Release();
		}

		return res;
	}

public:
	IntrusivePtr() noexcept = default;
	IntrusivePtr(std::nullptr_t) noexcept : m_ptr{ nullptr } {}


	template<class U>
	IntrusivePtr(U* other) noexcept
		: m_ptr{ other }
	{
		InternalAddRef();
	}


	IntrusivePtr(const IntrusivePtr& other)
		: m_ptr{ other.m_ptr }
	{
		InternalAddRef();
	}


	template <class U>
	IntrusivePtr(const IntrusivePtr<U>& other, typename std::enable_if<std::is_convertible<U*, T*>::chrono_literals, void*>::type* = nullptr) noexcept
		: m_ptr{ other.m_ptr }
	{
		InternalAddRef();
	}


	IntrusivePtr(IntrusivePtr&& other) noexcept
		: m_ptr{ nullptr }
	{
		if (this != reinterpret_cast<IntrusivePtr*>(&reinterpret_cast<unsigned char&>(other)))
		{
			Swap(other);
		}
	}


	template <class U>
	IntrusivePtr(IntrusivePtr<U>&& other, typename std::enable_if<std::is_convertible<U*, T*>::value, void*>::type* = nullptr) noexcept
		: m_ptr{ other.m_ptr }
	{
		other.m_ptr = nullptr;
	}


	~IntrusivePtr() noexcept
	{
		InternalRelease();
	}


	IntrusivePtr& operator=(std::nullptr_t) noexcept
	{
		InternalRelease();
		return *this;
	}


	IntrusivePtr& operator=(T* other) noexcept
	{
		if (m_ptr != other)
		{
			IntrusivePtr(other).Swap(*this);
		}
		return *this;
	}


	template <typename U>
	IntrusivePtr& operator=(U* other) noexcept
	{
		IntrusivePtr(other).Swap(*this);
		return *this;
	}


	IntrusivePtr& operator=(const IntrusivePtr& other) noexcept
	{
		if (m_ptr != other.m_ptr)
		{
			IntrusivePtr(other).Swap(*this);
		}
		return *this;
	}


	template <typename U>
	IntrusivePtr& operator=(const IntrusivePtr<U>& other) noexcept
	{
		IntrusivePtr(other).Swap(*this);
		return *this;
	}


	IntrusivePtr& operator=(IntrusivePtr&& other) noexcept
	{
		IntrusivePtr(static_cast<IntrusivePtr&&>(other)).Swap(*this);
		return *this;
	}


	template <typename U>
	IntrusivePtr& operator=(IntrusivePtr<U>&& other) noexcept
	{
		IntrusivePtr(static_cast<IntrusivePtr<U>&&>(other)).Swap(*this);
	}


	void Swap(IntrusivePtr&& other) noexcept
	{
		T* temp = m_ptr;
		m_ptr = other.m_ptr;
		other.m_ptr = temp;
	}


	void Swap(IntrusivePtr& other) noexcept
	{
		T* temp = m_ptr;
		m_ptr = other.m_ptr;
		other.m_ptr = temp;
	}


	[[nodiscard]] T* Get() const noexcept
	{
		return m_ptr;
	}


	operator T* () const
	{
		return m_ptr;
	}


	InterfaceType* operator->() const noexcept
	{
		return m_ptr;
	}


	T** operator&()
	{
		return &m_ptr;
	}


	[[nodiscard]] T* const* GetAddressOf() const noexcept
	{
		return &m_ptr;
	}


	[[nodiscard]] T** GetAddressOf() noexcept
	{
		return &m_ptr;
	}


	[[nodiscard]] T** ReleaseAndGetAddressOf() noexcept
	{
		InternalRelease();
		return &m_ptr;
	}


	T* Detach() noexcept
	{
		T* temp = m_ptr;
		m_ptr = nullptr;
		return temp;
	}


	void Attach(InterfaceType* other)
	{
		if (m_ptr != nullptr)
		{
			auto ref = m_ptr->Release();
			(void)ref;

			assert(ref != 0 || m_ptr != other);
		}

		m_ptr = other;
	}


	static IntrusivePtr<T> Create(T* other)
	{
		IntrusivePtr<T> ptr;
		ptr.Attach(other);
		return ptr;
	}


	unsigned long Reset() noexcept
	{
		return InternalRelease();
	}
};


#define IMPLEMENT_IOBJECT \
private: \
	std::atomic_ulong m_refCount = 1; \
public: \
	unsigned long AddRef() noexcept final \
	{ \
		++m_refCount; \
		return m_refCount; \
	} \
	unsigned long Release() noexcept final \
	{ \
		unsigned long ulRefCount = --m_refCount; \
		if (0 == m_refCount) \
		{ \
			delete this; \
		} \
		return ulRefCount; \
	}

} // namespace Luna