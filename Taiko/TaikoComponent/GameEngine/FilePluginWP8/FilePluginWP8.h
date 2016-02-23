#ifndef FILEPLUGINWP8_H
#define FILEPLUGINWP8_H

#include <FileResource/FileResource.h>
#include <windows.h>
#include <string>

class FilePluginWP8 : public FileResourceManagerPlugin
{
private:
	std::wstring m_path;
	class FileResourceWP8 : public FileResource
	{
	private:
		HANDLE m_hfile;
	public:
		FileResourceWP8(HANDLE hfile);
		~FileResourceWP8();
		int read(void *p, int size);
		int tell();
	};
public:
	FilePluginWP8(const wchar_t *path)
		: m_path(path)
	{}
	~FilePluginWP8()
	{}
	FileResource *create(const char *path);
};

#endif
