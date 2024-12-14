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

#include "FileSystem.h"

// Forward declarations
struct GLFWwindow;


namespace Luna
{

// Forward declarations
class FileSystem;
class LogSystem;


class Application
{
public:
	Application(uint32_t width, uint32_t height, const std::string& appTitle);
	virtual ~Application();

	virtual void ProcessCommandLine(int argc, char* argv[]) {}

	virtual void Configure();
	virtual void Startup() {}
	virtual void Shutdown() {}

	virtual bool Update() { return true; }
	virtual void UpdateUI() {}
	virtual void Render() {}

	void Run();
	void Close() { m_bWindowClosed = true; }

protected:
	uint32_t m_width{ 0 };
	uint32_t m_height{ 0 };
	std::string m_appTitle;

	bool m_bIsRunning{ false };
	bool m_bWindowClosed{ false };
	bool m_bIsVisible{ true };

	// Engine systems
	std::unique_ptr<FileSystem> m_fileSystem;
	//std::unique_ptr<LogSystem> m_logSystem;

private:
	bool Initialize();
	void Finalize();
	bool CreateAppWindow();
	void CreateDevice();

private:
	GLFWwindow* m_pWindow{ nullptr };
};


int Run(Application* pApplication);

} // namespace Luna