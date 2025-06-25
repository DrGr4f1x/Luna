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

#include "Graphics\GraphicsCommon.h"

namespace Luna
{

struct Object
{
	union
	{
		uint64_t integer;
		void* pointer;
	};

	Object(uint64_t i) 
		: integer{ i } 
	{}

	Object(void* p) 
		: pointer{ p }
	{}

	template <typename T> operator T* () const { return static_cast<T*>(pointer); }
};


class IResource
{
protected:
	IResource() = default;
	virtual ~IResource() = default;

	virtual Object GetNativeObject(NativeObjectType objectType)
	{
		(void)objectType;
		return nullptr;
	}
};

using ResourcePtr = std::shared_ptr<IResource>;

} // namespace Luna