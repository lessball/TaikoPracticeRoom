#ifndef MEMORYFILEREADER_H
#define MEMORYFILEREADER_H

#include <stdio.h>

/**
 * class MemoryFileReader
 * 将内存中的一段数据封装为文件流形式读取数据
 */
class MemoryFileReader
{
private:
	char *m_data;
	int m_size;
	int m_position;
public:
	/**
	 * 构造函数
	 * data 数据
	 * size 数据大小
	 */
	MemoryFileReader(void *data, int size);
	int read(void *ptr, int size);
	int seek(int offset, int origin);
	int tell()
	{
		return m_position;
	}
	/**
	 * 返回数据总大小
	 */
	int size()
	{
		return m_size;
	}
	/**
	 * 读取数据，直接返回指针，免去内存拷贝操作
	 * return 数据指针， NULL表示读取失败
	 */
	void *readData(int size);
	/**
	 * 读取指定的数据类型
	 * return 读取是否成功
	 */
	template<typename T>
	bool readData(T *p)
	{
		if(m_size-m_position < (int)sizeof(T))
			return false;
		memcpy(p, m_data+m_position, sizeof(T));
		m_position += sizeof(T);
		return true;
	}
	/**
	 * 读取字符串。
	 * return 字符串。需要自行释放。
	 */
	char *readString();
	char *readStringAligned();
};

#endif
