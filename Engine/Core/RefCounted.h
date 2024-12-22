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

template <class T>
struct RefCounted : public IUnknown
{
	virtual ~RefCounted() = default;

	ULONG AddRef() override
	{
		++m_refCount;
		return m_refCount;
	}

	ULONG Release() override
	{
		unsigned long ulRefCount = --m_refCount;
		if (0 == m_refCount)
		{
			delete this;
		}
		return ulRefCount;
	}

	HRESULT QueryInterface(REFIID riid, LPVOID* ppvObj) override
	{ 
		if (!ppvObj)
		{
			return E_INVALIDARG;
		}

		*ppvObj = nullptr;

		if (riid == IID_IUnknown || riid == __uuidof(T))
		{
			*ppvObj = (LPVOID)this;
			AddRef();
			return NOERROR;
		}

		return E_NOINTERFACE;
	}

protected:
	std::atomic_ulong m_refCount{ 1 };
};


template <typename T, typename ...TArgs>
wil::com_ptr<T> Create(TArgs&&... args)
{
	wil::com_ptr<T> object;
	T* ptr = new T(std::forward<TArgs>(args)...);
	object.attach(ptr);
	return object;
}

} // namespace Luna