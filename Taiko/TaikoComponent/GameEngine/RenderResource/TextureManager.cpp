#include <RenderResource/TextureManager.h>
#include <RenderResource/ImageIO.h>
#include <LogPrint.h>
#include <assert.h>

TextureManager TextureManager::s_singleton;

TextureResource::TextureResource(const char *name)
{
	assert(name != NULL);
	m_tex = NULL;
	int len = strlen(name);
	m_name = new char[len+1];
	memcpy(m_name, name ,len+1);
}
bool TextureResource::load(const char *name, RenderCore *rc)
{
	assert(m_tex == NULL && rc != NULL && strcmp(m_name, name)==0);
	m_tex = loadTexture(name, rc);
	if(m_tex != NULL)
	{
		return true;
	}else
	{
		LOG_PRINT("failed to load texture : %s\n", name);
		return false;
	}
}

TextureResource *TextureFactory::create(const char *name, bool delayLoad)
{
	assert(m_renderCore != NULL);
	if(delayLoad)
	{
		return new TextureResource(name);
	}else
	{
		TextureResource *texRes = new TextureResource(name);
		if(texRes->load(name, m_renderCore))
		{
			return texRes;
		}else
		{
			delete texRes;
			return NULL;
		}
	}
}
void TextureFactory::destroy(TextureResource *res)
{
	assert(res != NULL && m_renderCore != NULL);
	if(res->m_tex != NULL)
		m_renderCore->destroyTexture(res->m_tex);
	delete[] res->m_name;
	delete res;
}
