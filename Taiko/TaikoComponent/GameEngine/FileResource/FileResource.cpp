#include <FileResource/FileResource.h>
#include <assert.h>

char *FileResource::readAll()
{
	char *data = new char[m_size];
	int readed = read(data, m_size);
	assert(readed == m_size);
	return data;
}
char *FileResource::readAllString()
{
	char *data = new char[m_size+1];
	int readed = read(data, m_size);
	assert(readed == m_size);
	data[m_size] = '\0';
	return data;
}
