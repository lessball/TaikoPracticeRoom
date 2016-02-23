#include <FileResource/FileResourceManager.h>
#include <FileResource/FileResource.h>
#include <LogPrint.h>
#include <assert.h>
using namespace std;

FileResourceManager FileResourceManager::s_singleton;

void FileResourceManager::release()
{
	for(int i=0; i<(int)m_plugin.size(); i++)
		delete m_plugin[i];
	m_plugin.clear();
}
FileResource *FileResourceManager::open(const char *path)
{
	for(int i=0; i<(int)m_plugin.size(); i++)
	{
		FileResource *file = m_plugin[i]->create(path);
		if(file != NULL)
			return file;
	}
	LOG_PRINT("failed open file %s\n", path);
	return NULL;
}
void FileResourceManager::close(FileResource *file)
{
	delete file;
}
