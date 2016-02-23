#ifndef XMLFILEREADER_H
#define XMLFILEREADER_H

#include <rapidxml.hpp>

class XmlFileReader
{
private:
	char *m_data;
	rapidxml::xml_document<> m_doc;
public:
	XmlFileReader(const char *path);
	~XmlFileReader();
	rapidxml::xml_node<> *getRootNode();
};

#endif
