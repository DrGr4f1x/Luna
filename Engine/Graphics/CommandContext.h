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

// Forward declarations
class IGpuImage;
class IColorBuffer;


class __declspec(uuid("ECBD0FFD-6571-4836-9DBB-7DC6436E086F")) ICommandContext : public IUnknown
{
public:
	virtual ~ICommandContext() = default;

	virtual void SetId(const std::string& id) = 0;
	virtual CommandListType GetType() const = 0;

	// Debug events and markers
	virtual void BeginEvent(const std::string& label) = 0;
	virtual void EndEvent() = 0;
	virtual void SetMarker(const std::string& label) = 0;

	virtual void Reset() = 0;
	virtual void Initialize() = 0;

	// Flush existing commands and release the current context
	virtual uint64_t Finish(bool bWaitForCompletion = false) = 0;

	virtual void TransitionResource(IGpuImage* gpuImage, ResourceState newState, bool bFlushImmediate = false) = 0;
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

	virtual void ClearColor(IColorBuffer* colorBuffer) = 0;
	virtual void ClearColor(IColorBuffer* colorBuffer, Color clearColor) = 0;
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
