#ifndef RESOURCEMANAGERT_H
#define RESOURCEMANAGERT_H

#include <map>
#include <string>
#include <assert.h>
#include <LogPrint.h>

/**
 * 资源管理通用工具类，实现引用计数和延迟/增量加载功能
 * 基于引用计数原理，get和release要成对使用
 * 调用get和release方法时使delayLoad/delayDestroy参数为true则为延迟加载/释放
 * 延迟加载/释放的资源，要等到调用doIncrementLoad时才会真正加载/销毁
 * 增量加载的一般流程：延迟释放原有资源，然后延迟或立即加载新资源，然后调用doIncrementLoad处理延迟的释放/加载
 * 需要针对资源类型定义对应的工厂类
 */

/**
//ResourceFactoryType
//针对特定资源类型的工厂类，负责创建/加载/销毁实际资源
//接口要求
class ResourceFactoryType
{
public:
	//根据name创建ResourceType类型资源。delayLoad指定是否延迟加载。
	ResourceType *create(const char *name, bool delayLoad);
	//销毁指定资源
	void destroy(ResourceType *res);
	//返回指定资源是否已经加载成功
	bool isLoaded(ResourceType *res);
	//加载指定资源
	void load(ResourceType *res, const char *name);
};
//不实现延迟加载功能的ResourceFactoryType接口示例
class ResourceFactorySample
{
public:
	ResourceType *create(const char *name, bool delayLoad)
	{
		return new ResourceType(name);
	}
	void destroy(ResourceType *res)
	{
		delete res;
	}
	bool isLoaded(ResourceType *res)
	{
		return true;
	}
	void load(ResourceType *res, const char *name)
	{
		return;
	}
};
*/

/**
 * class ResourceManagerT
 * 参考本文件头
 * PrintLog 是否输出log
 */
template<typename ResourceType, typename ResourceFactoryType, int PrintLog=0>
class ResourceManagerT
{
private:
	struct ResourceRef
	{
		ResourceType *res;
		int ref;
	};
	typedef std::map<std::string, ResourceRef> ResourceMap;
	typedef typename ResourceMap::iterator ResourceMapIterator;
	ResourceMap m_map;
	ResourceFactoryType m_factory;
	int m_numToLoad;
	int m_numToDestroy;
	float m_loadRatio;
	ResourceMapIterator m_iLoad;
public:
	ResourceManagerT()
		: m_numToLoad(0), m_numToDestroy(0), m_loadRatio(-1.0f)
	{}
	ResourceManagerT(const ResourceFactoryType &factory)
		: m_factory(factory), m_numToLoad(0), m_numToDestroy(0), m_loadRatio(-1.0f)
	{}
	ResourceFactoryType &getFactory()
	{
		return m_factory;
	}
	/**
	 * 根据name获得资源。如果已存在则引用计数值加一，如果不存在则创建新资源对象，根据delayLoad决定是延迟加载资源数据还是立即加载
	 * 立即加载失败会返回NULL。延迟加载无法预知加载是否成功，因此总是会返回一个非空指针
	 */
	ResourceType *get(const char *name, bool delayLoad)
	{
		assert(m_loadRatio < 0.0f);
		std::string sname(name);
		ResourceMapIterator ires = m_map.find(sname);
		if(ires != m_map.end())
		{
			ires->second.ref++;
			if(PrintLog)
				LOG_PRINT("get %d %s\n", ires->second.ref, name);
			if(!m_factory.isLoaded(ires->second.res))
			{
				if(!delayLoad)
				{
					m_factory.load(ires->second.res, name);
					if(ires->second.ref > 1 && m_factory.isLoaded(ires->second.res))
						m_numToLoad--;  //尚未加载且ref+1>1，说明资源原本处于delayLoad状态等待加载。这里应立即加载并取消delayLoad状态
				}
			}else if(ires->second.ref == 1)
			{
				m_numToDestroy--;  //已经加载且ref+1==1，说明资源原本处于delayDestroy状态等待销毁，这里应取消delayDestroy状态
			}
			return ires->second.res;
		}
		ResourceRef tref = {m_factory.create(name, delayLoad), 1};
		if(tref.res == NULL)
			return NULL;
		if(!delayLoad && !m_factory.isLoaded(tref.res))
		{
			m_factory.destroy(tref.res);
			return NULL;
		}
		if(PrintLog)
			LOG_PRINT("get 1 %s\n", name);
		if(delayLoad)
			m_numToLoad++;
		m_map[sname] = tref;
		return tref.res;
	}
	/**
	 * 根据name释放资源。引用计数值减一，如果减到零，根据delayLoad决定是延迟释放资源数据还是立即释放
	 */
	void release(const char *name, bool delayDestroy)
	{
		assert(m_loadRatio < 0.0f);
		ResourceMapIterator ires = m_map.find(name);
		if(PrintLog)
		{
			if(ires != m_map.end())
				LOG_PRINT("release %d %s\n", ires->second.ref-1, name);
		}
		if(ires != m_map.end() && --(ires->second.ref) <= 0)
		{
			assert(ires->second.ref == 0);
			if(!m_factory.isLoaded(ires->second.res))
			{
				m_factory.destroy(ires->second.res);
				m_map.erase(ires);
				m_numToLoad--; //尚未加载且ref-1==0，说明资源原本处于delayLoad状态等待加载，这里应取消delayLoad状态
			}else
			{
				if(delayDestroy)
				{
					m_numToDestroy++;
				}else
				{
					m_factory.destroy(ires->second.res);
					m_map.erase(ires);
				}
			}
		}
	}
	/**
	 * 处理处于延迟加载/释放状态等待操作的资源
	 * 如果stepLoad为false，则一次处理所有资源并返回进度值1
	 * 如果stepLoad为true，则每次调用只处理一部分并返回进度值(0-1)，需要反复调用直到全部处理完成（返回1）。在全部处理完成前，不得调用get/release
	 */
	float doIncrementLoad(bool stepLoad)
	{
		if(stepLoad)
		{
			if(m_loadRatio < 0.0f)
			{
				m_loadRatio = 1.0f / (m_numToLoad + m_numToDestroy);
				m_iLoad = m_map.begin();
			}
			if(m_numToDestroy > 0)
			{
				for(; m_iLoad != m_map.end(); ++m_iLoad)
				{
					if(m_iLoad->second.ref <= 0)
					{
						m_factory.destroy(m_iLoad->second.res);
						m_map.erase(m_iLoad++);
						if(--m_numToDestroy == 0)
							m_iLoad = m_map.begin();
						break;
					}
				}
			}else if(m_numToLoad > 0)
			{
				for(; m_iLoad != m_map.end(); ++m_iLoad)
				{
					if(!m_factory.isLoaded(m_iLoad->second.res))
					{
						m_factory.load(m_iLoad->second.res, m_iLoad->first.c_str());
						m_numToLoad--;
						++m_iLoad;
						break;
					}
				}
			}
			int numLeft = m_numToDestroy + m_numToLoad;
			if(numLeft > 0)
			{
				assert(m_iLoad != m_map.end());
				return 1.0f - m_loadRatio * numLeft;
			}else
			{
				m_loadRatio = -1.0f;
				return 1.0f;
			}
		}else
		{
			for(ResourceMapIterator i = m_map.begin(); i != m_map.end();)
			{
				if(i->second.ref <= 0)
				{
					m_factory.destroy(i->second.res);
					m_map.erase(i++);
					if(--m_numToDestroy == 0)
						break;
				}else
				{
					++i;
				}
			}
			for(ResourceMapIterator i = m_map.begin(); i != m_map.end(); ++i)
			{
				if(!m_factory.isLoaded(i->second.res))
				{
					m_factory.load(i->second.res, i->first.c_str());
					if(--m_numToLoad == 0)
						break;
				}
			}
			assert(m_numToDestroy == 0 && m_numToLoad == 0);
			m_loadRatio = -1.0f;
			return 1.0f;
		}
	}
	/**
	 * 强制释放全部资源。通常情况下资源要通过release一一释放，这里是对应如程序结束等情况
	 */
	void releaseAll()
	{
		for(ResourceMapIterator i = m_map.begin(); i != m_map.end(); ++i)
			m_factory.destroy(i->second.res);
		m_map.clear();
		m_numToDestroy = m_numToLoad = 0;
		m_loadRatio = -1.0f;
	}
};

#endif
