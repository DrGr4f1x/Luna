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
enum class NativeObjectType : uint32_t;


struct NativeObjectPtr
{
	union
	{
		uint64_t integer;
		void* pointer;
	};

	NativeObjectPtr(uint64_t i) noexcept : integer{ i } {}
	NativeObjectPtr(void* p) noexcept : pointer{ p } {}

	template <typename T> operator T* () const noexcept { return static_cast<T*>(pointer); }
};

} // namespace Luna