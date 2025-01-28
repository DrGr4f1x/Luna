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

#include "Graphics\Shader.h"

using namespace Microsoft::WRL;


namespace Luna::DX12
{

struct ShaderDescExt
{
	std::unique_ptr<std::byte[]> byteCode;
	size_t byteCodeSize{ 0 };
};


class __declspec(uuid("C716D902-2F80-4E51-BB2A-D1F7522908D0")) ShaderData final
	: public RuntimeClass<RuntimeClassFlags<ClassicCom>, ChainInterfaces<IShaderData, IPlatformData>>
	, NonCopyable
{
public:
	explicit ShaderData(ShaderDescExt& descExt);

	const std::byte* GetByteCode() const override { return m_byteCode.get(); }
	size_t GetByteCodeSize() const override { return m_byteCodeSize; }

private:
	std::unique_ptr<std::byte[]> m_byteCode;
	size_t m_byteCodeSize{ 0 };
};

}