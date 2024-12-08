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

#include <type_traits>

template<typename E>
struct EnableBitmaskOperators
{
	static const bool enable = false;
};


template <typename E>
constexpr typename std::enable_if<EnableBitmaskOperators<E>::enable, E>::type
operator|(E lhs, E rhs)
{
	using underlying = std::underlying_type_t<E>;
	return static_cast<E>(static_cast<underlying>(lhs) | static_cast<underlying>(rhs));
}


template <typename E>
constexpr typename std::enable_if<EnableBitmaskOperators<E>::enable, E>::type
operator&(E lhs, E rhs)
{
	using underlying = std::underlying_type_t<E>;
	return static_cast<E>(static_cast<underlying>(lhs) & static_cast<underlying>(rhs));
}


template <typename E>
constexpr typename std::enable_if<EnableBitmaskOperators<E>::enable, E>::type
operator^(E lhs, E rhs)
{
	using underlying = std::underlying_type_t<E>;
	return static_cast<E>(static_cast<underlying>(lhs) ^ static_cast<underlying>(rhs));
}


template <typename E>
constexpr typename std::enable_if<EnableBitmaskOperators<E>::enable, E&>::type
operator|=(E& lhs, E rhs)
{
	using underlying = std::underlying_type_t<E>;
	lhs = static_cast<E>(static_cast<underlying>(lhs) | static_cast<underlying>(rhs));
	return lhs;
}


template <typename E>
constexpr typename std::enable_if<EnableBitmaskOperators<E>::enable, E&>::type
operator&=(E& lhs, E rhs)
{
	using underlying = std::underlying_type_t<E>;
	lhs = static_cast<E>(static_cast<underlying>(lhs) & static_cast<underlying>(rhs));
	return lhs;
}


template <typename E>
constexpr typename std::enable_if<EnableBitmaskOperators<E>::enable, E&>::type
operator^=(E& lhs, E rhs)
{
	using underlying = std::underlying_type_t<E>;
	lhs = static_cast<E>(static_cast<underlying>(lhs) ^ static_cast<underlying>(rhs));
	return lhs;
}


template <typename E>
constexpr typename std::enable_if<EnableBitmaskOperators<E>::enable, bool>::type
operator==(E lhs, int rhs)
{
	return static_cast<int>(lhs) == rhs;
}


template <typename E>
constexpr typename std::enable_if<EnableBitmaskOperators<E>::enable, bool>::type
operator==(int lhs, E rhs)
{
	return lhs == static_cast<int>(rhs);
}


template <typename E>
constexpr typename std::enable_if<EnableBitmaskOperators<E>::enable, bool>::type
operator!=(E lhs, int rhs)
{
	return static_cast<int>(lhs) != rhs;
}


template <typename E>
constexpr typename std::enable_if<EnableBitmaskOperators<E>::enable, bool>::type
operator!=(int lhs, E rhs)
{
	return lhs != static_cast<int>(rhs);
}


template <typename E>
constexpr typename std::enable_if<EnableBitmaskOperators<E>::enable, bool>::type
HasFlag(E bitmask, E flag)
{
	using underlying = std::underlying_type_t<E>;
	return (static_cast<underlying>(bitmask) & static_cast<underlying>(flag)) != underlying(0);
}


template <typename E>
constexpr typename std::enable_if<EnableBitmaskOperators<E>::enable, bool>::type
HasAnyFlag(E bitmask, E flags)
{
	return HasFlag(bitmask, flags);
}


template <typename E>
constexpr typename std::enable_if<EnableBitmaskOperators<E>::enable, bool>::type
HasAllFlags(E bitmask, E flags)
{
	using underlying = std::underlying_type_t<E>;
	return (static_cast<underlying>(bitmask) & static_cast<underlying>(flags)) == static_cast<underlying>(flags);
}


template <typename E>
constexpr typename std::enable_if<EnableBitmaskOperators<E>::enable, E>::type
RemoveFlag(E bitmask, E flag)
{
	using underlying = std::underlying_type_t<E>;
	bitmask &= static_cast<E>(~static_cast<underlying>(flag));
	return bitmask;
}