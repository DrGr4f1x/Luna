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

class __declspec(uuid("769F56BA-C54D-4BD7-8E6C-F401BD0EDCC7")) IPlatformData : public IUnknown
{
public:
	virtual ~IPlatformData() = default;
};

} // namespace Luna