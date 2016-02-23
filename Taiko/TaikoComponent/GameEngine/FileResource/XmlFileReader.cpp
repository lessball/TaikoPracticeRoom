#include "FileResource/XmlFileReader.h"
#include <FileResource/FileResource.h>
#include <LogPrint.h>
using namespace rapidxml;

XmlFileReader::XmlFileReader(const char *path)
{
	m_data = NULL;
	FileResource *file = FileResource::open(path);
	if(file == NULL)
		return;
	m_data = file->readAllString();
	FileResource::close(file);
	try
	{
		m_doc.parse<0>(m_data);
	}
	catch(parse_error &e)
	{
		LOG_PRINT("xml file parse error: %s\n", e.what());
		m_doc.clear();
	}
}
XmlFileReader::~XmlFileReader()
{
	m_doc.clear();
	delete[] m_data;
}
rapidxml::xml_node<> *XmlFileReader::getRootNode()
{
	return m_doc.first_node();
}
