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

class FileSystem : NonCopyable, NonMovable
{
public:
	explicit FileSystem(const std::string& appName);
	~FileSystem();

	const std::filesystem::path& GetBinaryPath() const { return m_binaryPath; }
	const std::filesystem::path& GetRootPath() const { return m_rootPath; }
	const std::filesystem::path& GetLogPath() const { return m_logPath; }

	// Sets root path to Bin\..
	void SetDefaultRootPath();
	void SetRootPath(const std::string& rootPathStr);
	void SetRootPath(const std::filesystem::path& rootPath);

	void AddSearchPath(const std::string& searchPathStr, bool appendPath = false);
	void RemoveSearchPath(const std::string& searchPathStr);

	std::vector<std::filesystem::path> GetSearchPaths() const;

	bool Exists(const std::string& fname) const;
	bool IsRegularFile(const std::string& fname) const;
	bool IsDirectory(const std::string& dname) const;
	std::string GetFullPath(const std::string& pathStr);
	std::string GetFileExtension(const std::string& fname);

	bool EnsureDirectory(const std::string& pathStr);
	bool EnsureLogDirectory();

private:
	void Initialize();
	void RemoveAllSearchPaths();

private:
	std::string m_appName;

	std::filesystem::path m_binaryPath;
	std::filesystem::path m_binarySubpath;

	std::filesystem::path m_rootPath;
	std::filesystem::path m_logPath;

	struct PathDesc
	{
		std::filesystem::path localPath;
		std::filesystem::path fullPath;
		PathDesc* next{ nullptr };
	};
	PathDesc* m_searchPaths{ nullptr };
};

FileSystem* GetFileSystem();

} // namespace Luna