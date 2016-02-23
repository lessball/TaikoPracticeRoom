#ifndef FILERESOURCE_H
#define FILERESOURCE_H

#include <FileResource/FileResourceManager.h>

class FileResource
{
protected:
	friend class FileResourceManager;
	int m_size; //init m_size by derived class
	virtual ~FileResource() {};
public:
	static FileResource *open(const char *path)
	{
		return FileResourceManager::getSingleton()->open(path);
	}
	static void close(FileResource *file)
	{
		return FileResourceManager::getSingleton()->close(file);
	}
	virtual int read(void *p, int size) =0;
	virtual int tell() =0;
	int size()
	{
		return m_size;
	}
	char *readAll();
	char *readAllString();
};

#endif
