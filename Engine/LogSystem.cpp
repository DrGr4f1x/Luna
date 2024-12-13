//
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
// Author:  David Elder
//

#include "Stdafx.h"

#include "LogSystem.h"

#include "FileSystem.h"

#include <iostream>

using namespace std;


namespace
{

Luna::LogSystem* g_logSystem{ nullptr };

bool outputToFile{ true };
bool outputToConsole{ true };
bool outputToDebug{ true };
bool allowThreadedLogging{ true };


string SeverityToString(Luna::Severity level)
{
	using enum Luna::Severity;

	switch (level)
	{
	case Fatal:		return "Fatal";	break;
	case Error:		return "Error";	break;
	case Warning:	return "Warning";	break;
	case Debug:		return "Debug";	break;
	case Notice:	return "Notice";	break;
	case Info:		return "Info"; break;
	default:		return "";	break;
	}
}

} // anonymous namespace


namespace Luna
{

void PostLogMessage(LogMessage&& message)
{
	auto* logSystem = GetLogSystem();
	if (logSystem)
	{
		logSystem->PostLogMessage(move(message));
	}
}


LogSystem::LogSystem()
	: m_initialized(false)
{
	Initialize();

	assert(g_logSystem == nullptr);
	g_logSystem = this;
}


LogSystem::~LogSystem()
{
	g_logSystem = nullptr;
	Shutdown();
}


void LogSystem::PostLogMessage(LogMessage&& message)
{
	if (allowThreadedLogging)
	{
		m_messageQueue.push(move(message));
	}
	else
	{
		OutputLogMessage(message);
	}
}


void LogSystem::CreateLogFile()
{
	// Get the log directory path
	FileSystem* fs = GetFileSystem();
	fs->EnsureLogDirectory();
	auto logPath = fs->GetLogPath();

	// Build the filename
	namespace chr = std::chrono;
	auto systemTime = chr::system_clock::now();
	auto localTime = chr::zoned_time{ chr::current_zone(), systemTime }.get_local_time();
	string filename = format("Log-{:%Y%m%d%H%M%S}.txt", chr::floor<chr::seconds>(localTime));

	// Build the full path
	auto fullPath = logPath / filename;

	// Open the file stream
	m_file.open(fullPath.c_str(), ios::out | ios::trunc);
	m_file << fixed;

	// Create a hard link to the non-timestamped file
	auto logFilePath = logPath / "Log.txt";
	filesystem::remove(logFilePath);
	try
	{
		filesystem::create_hard_link(fullPath, logFilePath);
	}
	catch (filesystem::filesystem_error e)
	{
		cerr << e.what() << " " << e.path1().string() << " " << e.path2().string() << endl;
	}
}


void LogSystem::Initialize()
{
	lock_guard<mutex> lock(m_initializationMutex);

	// Only initialize if we are in uninitialized state
	if (m_initialized)
	{
		return;
	}

	CreateLogFile();

	m_haltLogging = false;

	if (allowThreadedLogging)
	{
		m_workerLoop = async(launch::async,
			[&]
			{
				while (!m_haltLogging)
				{
					LogMessage message{};
					if (m_messageQueue.try_pop(message))
					{
						OutputLogMessage(message);
					}
				}
			}
		);
	}

	m_initialized = true;
}


void LogSystem::Shutdown()
{
	lock_guard<mutex> lock{ m_initializationMutex };

	// Only shutdown if we are in initialized state
	if (!m_initialized)
	{
		return;
	}

	m_haltLogging = true;
	if (allowThreadedLogging)
	{
		m_workerLoop.get();
	}

	m_file.flush();
	m_file.close();

	m_initialized = false;
}


void LogSystem::OutputLogMessage(const LogMessage& message)
{
	using enum Severity;

	namespace chr = std::chrono;
	const auto systemTime = chr::system_clock::now();
	const auto localTime = chr::zoned_time{ chr::current_zone(), systemTime }.get_local_time();
	const auto localTimeStr = format("[{:%Y.%m.%d-%H.%M.%S}]", chr::floor<chr::milliseconds>(localTime));

	const string categoryStr = format("{}: ", message.category.GetName());
	const string severityStr = (message.severity == Severity::Log) ? "" : format("{}: ", SeverityToString(message.severity));

	string messageStr;
	if (message.category.IsValid())
	{
		messageStr = format("{} {}{}{}", localTimeStr, categoryStr, severityStr, message.messageStr);
	}
	else
	{
		messageStr = format("{} {}{}", localTimeStr, severityStr, message.messageStr);
	}


	if (outputToFile)
	{
		m_file.flush();
		m_file << messageStr;
	}

	if (outputToConsole)
	{
		if (message.severity == Fatal || message.severity == Error)
		{
			cerr << messageStr;
		}
		else
		{
			cout << messageStr;
		}
	}

	if (outputToDebug)
	{
		OutputDebugStringA(messageStr.c_str());
	}

	if (message.severity == Fatal)
	{
		m_file.flush();
		m_file.close();

		cerr.flush();
		cout.flush();

		Utility::ExitFatal(messageStr, "Fatal Error");
	}
}


LogSystem* GetLogSystem()
{
	return g_logSystem;
}

} // namespace Luna