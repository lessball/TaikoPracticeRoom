#include "FilePluginWP8.h"

#include <codecvt>
#include <string>
#include <assert.h>

FilePluginWP8::FileResourceWP8::FileResourceWP8(HANDLE hfile)
{
	assert(hfile != INVALID_HANDLE_VALUE);
	m_hfile = hfile;
	LARGE_INTEGER move0;
	move0.QuadPart = 0;
	LARGE_INTEGER pos = move0;
	SetFilePointerEx(m_hfile, move0, &pos, FILE_END);
	SetFilePointerEx(m_hfile, move0, NULL, FILE_BEGIN);
	m_size = (int)(pos.LowPart);
}
FilePluginWP8::FileResourceWP8::~FileResourceWP8()
{
	assert(m_hfile != INVALID_HANDLE_VALUE);
	CloseHandle(m_hfile);
}
int FilePluginWP8::FileResourceWP8::read(void *p, int size)
{
	assert(m_hfile != INVALID_HANDLE_VALUE);
	DWORD readed = 0;
	ReadFile(m_hfile, p, size, &readed, NULL);
	return (int)readed;
}
int FilePluginWP8::FileResourceWP8::tell()
{
	assert(m_hfile != INVALID_HANDLE_VALUE);
	LARGE_INTEGER move0;
	move0.QuadPart = 0;
	LARGE_INTEGER pos = move0;
	SetFilePointerEx(m_hfile, move0, &pos, FILE_CURRENT);
	return (int)pos.LowPart;
}

FileResource *FilePluginWP8::create(const char *path)
{
	std::wstring tpath = m_path;
	std::wstring_convert<std::codecvt_utf8<wchar_t>> cv;
	tpath.append(cv.from_bytes(path));
	HANDLE hfile = CreateFile2(tpath.c_str(), GENERIC_READ, FILE_SHARE_READ, OPEN_EXISTING, NULL);
	return hfile != INVALID_HANDLE_VALUE ? new FileResourceWP8(hfile) : NULL;
}
