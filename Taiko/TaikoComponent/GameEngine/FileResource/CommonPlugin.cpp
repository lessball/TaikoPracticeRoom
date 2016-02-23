#include <FileResource/CommonPlugin.h>

FilePluginStdio::FileResourceStdio::FileResourceStdio(FILE *file)
{
	m_file = file;
	fseek(m_file, 0, SEEK_END);
	m_size = ftell(m_file);
	fseek(m_file, 0, SEEK_SET);
}
FilePluginStdio::FileResourceStdio::~FileResourceStdio()
{
	fclose(m_file);
}
FileResource *FilePluginStdio::create(const char *path)
{
	FILE *file = fopen((m_path + path).c_str(), "rb");
	return file != NULL ? new FileResourceStdio(file) : NULL;
}

FilePluginZip::FileResourceZip::FileResourceZip(unzFile file)
{
	m_unzFile = file;
	unz_file_info64 info;
	int ret = unzGetCurrentFileInfo64(m_unzFile, &info, NULL, 0, NULL, 0, NULL, 0);
	m_size = (int)info.uncompressed_size;
}
FilePluginZip::FileResourceZip::~FileResourceZip()
{
	unzCloseCurrentFile(m_unzFile);
}
FilePluginZip::~FilePluginZip()
{
	if(m_unzFile != NULL)
	{
		unzCloseCurrentFile(m_unzFile);
		unzClose(m_unzFile);
	}
}
FileResource *FilePluginZip::create(const char *path)
{
	if(UNZ_OK == unzLocateFile(m_unzFile, path, 1))
	{
		unzOpenCurrentFile(m_unzFile);
		return new FileResourceZip(m_unzFile);
	}
	return NULL;
}