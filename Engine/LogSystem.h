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

#include <concurrent_queue.h>

namespace Luna
{

enum class Severity
{
	Fatal,
	Error,
	Warning,
	Notice,
	Info,
	Debug,
	Log
};


class LogCategory
{
public:
	LogCategory() = default;
	explicit LogCategory(const std::string& name) : m_name{ name } {}

	bool IsValid() const noexcept
	{
		return !m_name.empty();
	}

	const std::string& GetName() const
	{
		return m_name;
	}

private:
	std::string m_name;
};


struct LogMessage
{
	std::string messageStr;
	Severity severity;
	LogCategory category;
};

void PostLogMessage(LogMessage&& message);


class LogSystem : NonCopyable, NonMovable
{
public:
	LogSystem();
	~LogSystem();

	bool IsInitialized() const { return m_initialized; }

	void PostLogMessage(LogMessage&& message);

private:
	void CreateLogFile();
	void Initialize();
	void Shutdown();

	void OutputLogMessage(const LogMessage& message);

private:
	std::mutex m_initializationMutex;
	std::ofstream m_file;
	Concurrency::concurrent_queue<LogMessage> m_messageQueue;
	std::atomic<bool> m_haltLogging;
	std::future<void> m_workerLoop;
	std::atomic<bool> m_initialized;
};


class LogBase
{
public:
	class LogProxy : NonCopyable
	{
	public:
		LogProxy() = delete;
		LogProxy(Severity severity, LogCategory category)
			: m_severity{ severity }
			, m_category{ category }
		{
		}
		LogProxy(LogProxy&& other) noexcept
			: m_severity{ other.m_severity }
			, m_category{ other.m_category }
			, m_stream{ std::move(other.m_stream) }
		{
		}
		~LogProxy()
		{
			m_stream.flush();
			PostLogMessage({ m_stream.str(), m_severity, m_category });
		}

		template <typename T>
		LogProxy& operator<<(const T& value)
		{
			m_stream << value;
			return *this;
		}

		LogProxy& operator<<(std::ostream& (*os)(std::ostream&))
		{
			m_stream << os;
			return *this;
		}

	private:
		Severity m_severity{ Severity::Info };
		LogCategory m_category;
		std::ostringstream m_stream;
	};

	LogProxy operator()(const LogCategory& category)
	{
		return LogProxy{ m_severity, category };
	}

	template <typename T>
	LogProxy operator<<(const T& value)
	{
		LogProxy proxy{ m_severity, LogCategory{} };
		proxy << value;
		return proxy;
	}

	LogProxy&& operator<<(std::ostream& (*os)(std::ostream&))
	{
		LogProxy proxy{ m_severity, LogCategory{} };
		proxy << os;
		return std::move(proxy);
	}

public:
	LogBase() = delete;
	explicit LogBase(Severity severity) : m_severity{ severity } {}

private:
	const Severity m_severity{ Severity::Info };
};


inline LogBase Log{ Severity::Log };
inline LogBase LogFatal{ Severity::Fatal };
inline LogBase LogError{ Severity::Error };
inline LogBase LogWarning{ Severity::Warning };
inline LogBase LogNotice{ Severity::Notice };
inline LogBase LogInfo{ Severity::Info };
inline LogBase LogDebug{ Severity::Debug };

LogSystem* GetLogSystem();

} // namespace Luna