// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
// Adapted from BinaryReader.h in Microsoft's DirectXTK project
// https://github.com/Microsoft/DirectXTK
//

#pragma once

#include <type_traits>

class BinaryReader
{
public:
	// Reads from a file on disk
	explicit BinaryReader(const std::string& fileName);
	// Reads from an existing memory buffer
	BinaryReader(const std::byte* dataBlob, size_t size);

	// Reads a single value
	template<typename T> const T& Read()
	{
		return *ReadArray<T>(1);
	}

	// Reads an array of values
	template<typename T> const T* ReadArray(size_t elementCount)
	{
		static_assert(std::is_pod<T>::value, "Can only read plain-old-data types");

		const byte* newPos = m_pos + sizeof(T) * elementCount;

		if (newPos > m_end)
		{
			throw std::exception("End of file");
		}

		auto result = reinterpret_cast<const T*>(m_pos);
		m_pos = newPos;

		return result;
	}

	// Lower level helper reads directly from filesystem into memory
	static HRESULT ReadEntireFile(const std::string& fileName, std::unique_ptr<std::byte[]>& data, size_t* dataSize);

private:
	// Data currently being read
	const std::byte* m_pos{ nullptr };
	const std::byte* m_end{ nullptr };

	std::unique_ptr<std::byte[]> m_ownedData;

	// Prevent copying 
	BinaryReader(const BinaryReader&) = delete;
	BinaryReader& operator=(const BinaryReader&) = delete;
};