#ifndef COMMONPLUGIN_H
#define COMMONPLUGIN_H

#include <FileResource/FileResource.h>
#include <minizip/unzip.h>
#include <stdio.h>
#include <string>

class FilePluginStdio : public FileResourceManagerPlugin
{
private:
	std::string m_path;
	class FileResourceStdio : public FileResource
	{
	private:
		FILE *m_file;
	public:
		FileResourceStdio(FILE *file);
		~FileResourceStdio();
		int read(void *p, int size)
		{
			return (int)fread(p, 1, size, m_file);
		}
		int tell()
		{
			return (int)ftell(m_file);
		}
	};
public:
	FilePluginStdio(const char *path)
		:m_path(path)
	{}
	~FilePluginStdio()
	{}
	FileResource *create(const char *path);
};

class FilePluginZip : public FileResourceManagerPlugin
{
private:
	unzFile m_unzFile;
	class FileResourceZip : public FileResource
	{
	private:
		unzFile m_unzFile;
	public:
		FileResourceZip(unzFile file);
		~FileResourceZip();
		int read(void *p, int size)
		{
			return unzReadCurrentFile(m_unzFile, p, size);
		}
		int tell()
		{
			return (int)unztell64(m_unzFile);
		}
	};
public:
	FilePluginZip(const char *path)
	{
		m_unzFile = unzOpen64(path);
	}
	~FilePluginZip();
	bool valid()
	{
		return m_unzFile != NULL;
	}
	FileResource *create(const char *path);
};

#endif
