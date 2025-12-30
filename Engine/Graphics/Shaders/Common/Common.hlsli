//
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
// Author:  David Elder
//

#if VK

// Macro wrapping [[vk::binding(x)]] and [[vk::binding(x,y)]]
#define GET_VK_BINDING(_1,_2,NAME,...) NAME
#define VK_BINDING(...) GET_VK_BINDING(__VA_ARGS__, VK_BINDING2, VK_BINDING1)(__VA_ARGS__)
#define VK_BINDING2(x,y) [[vk::binding(x,y)]]
#define VK_BINDING1(x) [[vk::binding(x)]]

// Macro wrapping [[vk::location(x)]]
#define VK_LOCATION(x) [[vk::location(x)]]

// Macro wrapping [[vk::push_constant]]
#define VK_PUSH_CONSTANT [[vk::push_constant]]

#define VK_DESCRIPTOR_SET(dset) ,space##dset

#define BINDING(reg, set) register(reg VK_DESCRIPTOR_SET(set))

#else // DX12

// Macro wrapping [[vk::binding(x)]] and [[vk::binding(x,y)]]
#define GET_VK_BINDING(_1,_2,NAME,...) NAME
#define VK_BINDING(...) GET_VK_BINDING(__VA_ARGS__, VK_BINDING2, VK_BINDING1)(__VA_ARGS__)
#define VK_BINDING2(x,y)
#define VK_BINDING1(x)

// Macro wrapping [[vk::location(x)]]
#define VK_LOCATION(x)

// Macro wrapping [[vk::push_constant]]
#define VK_PUSH_CONSTANT

#define VK_DESCRIPTOR_SET(dset)

#define BINDING(reg, set) register(reg)

#endif // VK