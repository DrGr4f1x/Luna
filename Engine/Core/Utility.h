//
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
// Adapted from Utility.h in Miniengine
// https://github.com/Microsoft/DirectX-Graphics-Samples
//

#pragma once

namespace Utility
{
const uint32_t BUFFER_SIZE = 1024;

inline void Print(const char* msg) { printf(msg); }
inline void Print(const wchar_t* msg) { wprintf(msg); }

inline void Printf(const char* format, ...)
{
	char buffer[BUFFER_SIZE];
	va_list ap;
	va_start(ap, format);
	vsprintf_s(buffer, BUFFER_SIZE, format, ap);
	Print(buffer);
}

inline void Printf(const wchar_t* format, ...)
{
	wchar_t buffer[BUFFER_SIZE];
	va_list ap;
	va_start(ap, format);
	vswprintf(buffer, BUFFER_SIZE, format, ap);
	Print(buffer);
}

#ifndef RELEASE
inline void PrintSubMessage(const char* format, ...)
{
	Print("--> ");
	char buffer[BUFFER_SIZE];
	va_list ap;
	va_start(ap, format);
	vsprintf_s(buffer, BUFFER_SIZE, format, ap);
	Print(buffer);
	Print("\n");
}
inline void PrintSubMessage(const wchar_t* format, ...)
{
	Print("--> ");
	wchar_t buffer[BUFFER_SIZE];
	va_list ap;
	va_start(ap, format);
	vswprintf(buffer, BUFFER_SIZE, format, ap);
	Print(buffer);
	Print("\n");
}
inline void PrintSubMessage(void)
{
}
#endif

void ExitFatal(const std::string& message, const std::string& caption);

} // namespace Utility


#define halt( ... ) ERROR( __VA_ARGS__ ) __debugbreak();

#if defined(_RELEASE)

#define assert_msg( isTrue, ... ) (void)(isTrue)
#define warn_once_if( isTrue, ... ) (void)(isTrue)
#define warn_once_if_not( isTrue, ... ) (void)(isTrue)
#define error( msg, ... )
#define debugprint( msg, ... ) do {} while(0)
#define assert_succeeded( hr, ... ) (void)(hr)

#else	// !RELEASE

#define STRINGIFY(x) #x
#define STRINGIFY_BUILTIN(x) STRINGIFY(x)
#define assert_msg( isFalse, ... ) \
	if (!(bool)(isFalse)) { \
		Utility::Print("\nAssertion failed in " STRINGIFY_BUILTIN(__FILE__) " @ " STRINGIFY_BUILTIN(__LINE__) "\n"); \
		Utility::PrintSubMessage("\'" #isFalse "\' is false"); \
		Utility::PrintSubMessage(__VA_ARGS__); \
		Utility::Print("\n"); \
		__debugbreak(); \
	}

#define assert_succeeded( hr, ... ) \
	if (FAILED(hr)) { \
		Utility::Print("\nHRESULT failed in " STRINGIFY_BUILTIN(__FILE__) " @ " STRINGIFY_BUILTIN(__LINE__) "\n"); \
		Utility::PrintSubMessage("hr = 0x%08X", hr); \
		_com_error err(hr); \
		Utility::PrintSubMessage(L"hr = %s", err.ErrorMessage()); \
		Utility::PrintSubMessage(__VA_ARGS__); \
		Utility::Print("\n"); \
		__debugbreak(); \
	}


#define warn_once_if( isTrue, ... ) \
{ \
	static bool s_TriggeredWarning = false; \
	if ((bool)(isTrue) && !s_TriggeredWarning) { \
		s_TriggeredWarning = true; \
		Utility::Print("\nWarning issued in " STRINGIFY_BUILTIN(__FILE__) " @ " STRINGIFY_BUILTIN(__LINE__) "\n"); \
		Utility::PrintSubMessage("\'" #isTrue "\' is true"); \
		Utility::PrintSubMessage(__VA_ARGS__); \
		Utility::Print("\n"); \
	} \
}

#define warn_once_if_not( isTrue, ... ) warn_once_if(!(isTrue), __VA_ARGS__)

#define error( ... ) \
	Utility::Print("\nError reported in " STRINGIFY_BUILTIN(__FILE__) " @ " STRINGIFY_BUILTIN(__LINE__) "\n"); \
	Utility::PrintSubMessage(__VA_ARGS__); \
	Utility::Print("\n");

#define debugprint( msg, ... ) \
Utility::Printf( msg "\n", ##__VA_ARGS__ );

#endif

#define BreakIfFailed( hr ) if (FAILED(hr)) __debugbreak()


void SIMDMemCopy(void* __restrict dest, const void* __restrict source, size_t numQuadwords);
void SIMDMemFill(void* __restrict dest, __m128 fillVector, size_t numQuadwords);


// Smart pointer helpers
struct HandleCloser
{
	void operator()(HANDLE h) { if (h) CloseHandle(h); }
};

typedef public std::unique_ptr<void, HandleCloser> ScopedHandle;

inline HANDLE SafeHandle(HANDLE h)
{
	return (h == INVALID_HANDLE_VALUE) ? 0 : h;
}


// Helper class for COM exceptions
class com_exception : public std::exception
{
public:
	com_exception(HRESULT hr) noexcept : result(hr) {}

	const char* what() const noexcept override
	{
		static char s_str[64] = {};
		sprintf_s(s_str, "Failure with HRESULT of %08X", static_cast<unsigned int>(result));
		return s_str;
	}

private:
	HRESULT result;
};


// Helper utility converts D3D API failures into exceptions.
inline void ThrowIfFailed(HRESULT hr)
{
	if (FAILED(hr))
	{
		// Set a breakpoint on this line to catch DirectX API errors
		throw com_exception(hr);
	}
}

// wstring-string and string-wstring converters
inline std::wstring MakeWStr(const std::string& str)
{
	if (str.empty())
	{
		return std::wstring();
	}
	int numChars = MultiByteToWideChar(CP_ACP, WC_ERR_INVALID_CHARS, str.c_str(), (int)str.size(), nullptr, 0);
	std::wstring wstr;
	if (numChars)
	{
		wstr.resize(numChars);
		if (MultiByteToWideChar(CP_ACP, WC_ERR_INVALID_CHARS, str.c_str(), (int)str.size(), &wstr[0], numChars))
		{
			return wstr;
		}
	}
	return std::wstring();
}


inline std::string MakeStr(const std::wstring& wstr)
{
	if (wstr.empty())
	{
		return std::string();
	}
	int numChars = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, wstr.c_str(), (int)wstr.size(), nullptr, 0, nullptr, nullptr);
	std::string str;
	if (numChars)
	{
		str.resize(numChars);
		if (WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, wstr.c_str(), (int)wstr.size(), &str[0], numChars, nullptr, nullptr))
		{
			return str;
		}
	}
	return std::string();
}