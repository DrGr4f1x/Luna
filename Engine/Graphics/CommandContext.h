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

#include "Graphics\Enums.h"


namespace Luna
{

class __declspec(uuid("ECBD0FFD-6571-4836-9DBB-7DC6436E086F")) ICommandContext : public IUnknown
{
public:
	virtual ~ICommandContext() = default;

	virtual void SetId(const std::string& id) = 0;
	virtual CommandListType GetType() const = 0;

	virtual void Reset() = 0;
	virtual void Initialize() = 0;
};


class __declspec(uuid("8D423DA1-A002-4F6E-995C-0610FFB3220A")) IComputeContext : public ICommandContext
{
public:
	virtual ~IComputeContext() = default;
};


class __declspec(uuid("E59FD1DC-2D7E-42ED-9135-47C3CB23E399")) IGraphicsContext : public IComputeContext
{
public:
	virtual ~IGraphicsContext() = default;
};


struct CommandContext
{
	static ICommandContext& Begin(const std::string id = "");
};


struct GraphicsContext
{
	static IGraphicsContext& Begin(const std::string id = "");
};


struct ComputeContext
{
	static IComputeContext& Begin(const std::string id = "", bool bAsync = false);
};

} // namespace Luna
