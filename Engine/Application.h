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

class IApplication
{
public:
	IApplication(uint32_t width, uint32_t height, const std::string& appTitle);

	uint32_t GetWidth() const { return m_width; }
	uint32_t GetHeight() const { return m_height; }
	const std::string& GetTitle() const { return m_appTitle; }
	const std::wstring GetWTitle() const;

	virtual void OnInit() = 0;
	virtual void OnUpdate() = 0;
	virtual void OnRender() = 0;
	virtual void OnDestroy() = 0;

	virtual void OnKeyDown(UINT8 /*key*/) {}
	virtual void OnKeyUp(UINT8 /*key*/) {}

	virtual void ParseCommandLineArgs(WCHAR* argv[], int argc) {}

protected:
	uint32_t m_width{ 0 };
	uint32_t m_height{ 0 };
	std::string m_appTitle;
};

} // namespace Luna