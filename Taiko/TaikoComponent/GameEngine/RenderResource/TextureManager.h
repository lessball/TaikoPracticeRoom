#ifndef TEXTUREMANAGER_H
#define TEXTUREMANAGER_H

#include <ResourceManagerT.h>
#include <RenderResource/ImageIO.h>
#include <RenderCore/RenderCore.h>

/**
 * 贴图资源管理。参考ResourceManagerT.h
 */

/**
 * class TextureResource
 * 受资源管理器管理的贴图资源
 */
class TextureResource
{
private:
	friend class TextureFactory;
	char *m_name;
	RenderTexture *m_tex;
public:
	/**
	 * 用指定名称初始化，不加载
	 */
	TextureResource(const char *name);
	/**
	 * 允许创建自定义的贴图资源
	 */
	TextureResource(RenderTexture *texture)
		:m_tex(texture), m_name(NULL)
	{}
	/**
	 * 加载贴图
	 * name 贴图文件路径
	 * rc 渲染核心
	 */
	bool load(const char *name, RenderCore *rc);

	int getWidth() const
	{
		return m_tex != NULL ? m_tex->getWidth() : 0;
	}
	int getHeight() const
	{
		return m_tex != NULL ? m_tex->getHeight() : 0;
	}
	int getFormat() const
	{
		return m_tex != NULL ? m_tex->getPixelFormat() : 0;
	}
	RenderTexture *getTexture() const
	{
		return m_tex;
	}
	const char *getName() const
	{
		return m_name;
	}
};

/**
 * class TextureFactory
 * 用于资源管理器的对应工厂类
 */
class TextureFactory
{
private:
	RenderCore *m_renderCore;
public:
	TextureFactory() : m_renderCore(NULL) {}
	/**
	 * 初始化
	 * rc 渲染核心
	 */
	void init(RenderCore *rc)
	{
		m_renderCore = rc;
	}

	TextureResource *create(const char *name, bool delayLoad);
	void destroy(TextureResource *res);
	bool isLoaded(TextureResource *res)
	{
		assert(res != NULL && m_renderCore != NULL);
		return res->m_tex != NULL;
	}
	void load(TextureResource *res, const char *name)
	{
		assert(m_renderCore != NULL);
		res->load(name, m_renderCore);
	}
};

/**
 * 贴图资源管理器
 */
class TextureManager : public ResourceManagerT<TextureResource, TextureFactory>
{
private:
	static TextureManager s_singleton;
public:
	static TextureManager *getSingleton()
	{
		return &s_singleton;
	}
	/**
	 * 初始化
	 * rc 渲染核心
	 */
	void init(RenderCore *rc)
	{
		getFactory().init(rc);
	}
	/**
	 * 释放贴图资源
	 */
	void release(TextureResource *tex, bool delayDestroy)
	{
		assert(tex->getName() != NULL);
		release(tex->getName(), delayDestroy);
	}
	/**
	 * 释放贴图资源
	 */
	void release(const char *name, bool delayDestroy)
	{
		assert(name != NULL);
		ResourceManagerT<TextureResource, TextureFactory>::release(name, delayDestroy);
	}
};

#endif
