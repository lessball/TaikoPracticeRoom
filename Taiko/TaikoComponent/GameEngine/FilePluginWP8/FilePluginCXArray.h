#ifndef FILEPLUGINCXARRAY_H
#define FILEPLUGINCXARRAY_H

#include <FileResource/FileResource.h>
#include <codecvt>
#include <string>
#include <unordered_map>

class FilePluginCXArray : public FileResourceManagerPlugin
{
private:
	class FileResourceCXArray : public FileResource
	{
	private:
		const Platform::Array<byte> ^m_data;
		int m_position;
	public:
		FileResourceCXArray()
			: m_data(nullptr), m_position(0)
		{
			m_size = 0;
		}
		FileResourceCXArray(const Platform::Array<byte> ^data)
			: m_data(data), m_position(0)
		{
			m_size = data->Length;
		}
		~FileResourceCXArray()
		{}
		int read(void *p, int size)
		{
			if (m_position >= m_size) return 0;
			size = m_size <= (m_size - m_position) ? size : (m_size - m_position);
			memcpy(p, m_data->begin() + m_position, size);
			m_position += size;
			return size;
		}
		int tell()
		{
			return m_position;
		}
	};

	std::unordered_map<std::string, FileResourceCXArray> m_file;

public:
	void addFile(Platform::String ^path, const Platform::Array<byte> ^data)
	{
		std::wstring_convert<std::codecvt_utf8<wchar_t>> cv;
		m_file[cv.to_bytes(path->Data())] = FileResourceCXArray(data);
	}
	void clearAllFile()
	{
		m_file.clear();
	}

	FileResource *create(const char *path)
	{
		auto ifind = m_file.find(path);
		if (ifind != m_file.end())
		{
			return new FileResourceCXArray(ifind->second);
		}
		return NULL;
	}
};

#endif
