#include <FileResource/MemoryFileReader.h>
#include <stdio.h>
#include <string.h>

MemoryFileReader::MemoryFileReader(void *data, int size)
	: m_size(size), m_position(0)
{
	m_data = (char *)data;
}
int MemoryFileReader::read(void *ptr, int size)
{
	int leftsize = m_size - m_position;
	if(leftsize == 0)
		return 0;
	if(leftsize < size)
		size = leftsize;
	memcpy(ptr, m_data+m_position, size);
	m_position += size;
	return size;
}
int MemoryFileReader::seek(int offset, int origin)
{
	switch(origin)
	{
	case SEEK_CUR:
		offset += m_position;
		break;
	case SEEK_END:
		offset += m_size;
		break;
	case SEEK_SET:
		break;
	default:
		return -1;
	}
	if(offset < 0 || offset > m_size)
		return -1;
	m_position = offset;
	return 0;
}
void *MemoryFileReader::readData(int size)
{
	if(m_size - m_position < size)
		return NULL;
	void *ret = m_data + m_position;
	m_position += size;
	return ret;
}
char *MemoryFileReader::readString()
{
	int len;
	if(!readData(&len))
		return NULL;
	if(m_size-m_position < len)
	{
		m_position -= 4;
		return NULL;
	}
	char *str = new char[len+1];
	memcpy(str, m_data+m_position, len);
	str[len] = '\0';
	m_position += len;
	return str;
}
char *MemoryFileReader::readStringAligned()
{
	int len;
	if(!readData(&len))
		return NULL;
	if(m_size-m_position < len)
	{
		m_position -= 4;
		return NULL;
	}
	char *str = new char[len+1];
	memcpy(str, m_data+m_position, len);
	str[len] = '\0';
	len = (len+3) & 0xfffffffc;
	m_position += len;
	return str;
}
