#ifndef FILERESOURCEMANAGER_H
#define FILERESOURCEMANAGER_H

#include <minizip/unzip.h>
#include <string>
#include <vector>

class FileResource;

class FileResourceManagerPlugin
{
public:
	virtual ~FileResourceManagerPlugin() {};
	virtual FileResource *create(const char *path) =0;
};

class FileResourceManager
{
private:
	std::vector<FileResourceManagerPlugin*> m_plugin;
	static FileResourceManager s_singleton;
public:
	static FileResourceManager *getSingleton()
	{
		return &s_singleton;
	}
	void release();
	void addPlugin(FileResourceManagerPlugin *plugin)
	{
		m_plugin.push_back(plugin);
	}
	FileResource *open(const char *path);
	void close(FileResource *file);
};

#endif
