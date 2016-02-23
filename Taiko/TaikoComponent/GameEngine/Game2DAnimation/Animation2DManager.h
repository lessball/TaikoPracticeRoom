#ifndef ANIMATION2DMANAGER_H
#define ANIMATION2DMANAGER_H

#include <ResourceManagerT.h>
#include <Game2DImage/Game2DImageNode.h>
#include <map>
#include <set>
#include <string>

/**
 * 2D动画管理器。参考ResourceManagerT.h
 */

class Animation2D;

/**
 * 2D动画桢的附加信息，如碰撞框、引用点
 */
struct Animation2DFrameInfo
{
	struct Rect
	{
		float xmin, xmax, ymin, ymax;
	};
	struct Point
	{
		float x, y;
	};
	std::vector<Rect> collisionRect;
	std::vector<Point> referencePoint;
};
/**
 * 2D动画的一个Action
 */
struct Animation2DAction
{
	/** 桢数据 */
	struct Frame
	{
		float endTime;
		Animation2DFrameInfo *info;
	};
	Frame *frame;
	/** action的根节点，每一桢对应一个子节点 */
	Game2DImageNode *node;
	/** 是否循环 */
	bool loop;
};
/**
 * 2D动画资源类型
 */
class Animation2DResource
{
private:
	std::string m_name;
	typedef std::map<std::string, Animation2DAction> ActionMap;
	ActionMap m_actionMap;
	std::string m_defaultAction;
	Animation2DFrameInfo *m_frameInfo;
	std::vector<std::string> m_image;
	int m_numFrameInfo;
public:
	Animation2DResource(const char *name);
	~Animation2DResource();
	/**
	 * 加载资源
	 * name 名称（文件路径）
	 * delay 动画所引用的贴图是否需要立即加载
	 */
	bool load(const char *name, bool delay);
	/**
	 * 是否已加载
	 */
	bool isLoaded() const
	{
		return m_actionMap.size() > 0;
	}
	/**
	 * 立即释放2D动画资源
	 * delay 2D动画资源引用的贴图资源是否需要延迟释放
	 */
	void release(bool delay);
	/**
	 * 创建2D动画对象
	 */
	Animation2D *createAnimation() const;
	/**
	 * 根据名称创建对应的action
	 * name action名称。如果为NULL则使用第一个action
	 */
	Animation2DAction createAction(const char *name) const;
	/**
	 * 返回默认action名称
	 */
	const char *getDefaultActionName() const
	{
		return m_defaultAction.c_str();
	}
	/**
	 * 返回名称
	 */
	const char *getName() const
	{
		return m_name.c_str();
	}
};
/**
 * 2D动画管理器类型
 */
class Animation2DManager
{
private:
	/**
	 * Animation2DFactory 用于资源管理器的对应工厂类
	 */
	struct Animation2DFactory
	{
		bool m_texDelay;
		Animation2DFactory() : m_texDelay(false) {}
		Animation2DResource *create(const char *name, bool delayLoad);
		void destroy(Animation2DResource *res);
		bool isLoaded(Animation2DResource *res)
		{
			return res->isLoaded();
		}
		void load(Animation2DResource *res, const char *name)
		{
			res->load(name, m_texDelay);
		}
	};
	ResourceManagerT<Animation2DResource, Animation2DFactory> m_resourceManager;
	std::set<Animation2D *> m_activeSet;
	static Animation2DManager s_singleton;
public:
	/**
	 * 返回单件指针
	 */
	static Animation2DManager *getSingleton()
	{
		return &s_singleton;
	}
	/**
	 * 设置动画资源引用的贴图资源是否需要延迟加载/释放
	 */
	void setTextureDelayLoadEnable(bool enable)
	{
		m_resourceManager.getFactory().m_texDelay = enable;
	}
	/**
	 * 获取2D动画对象
	 * name 名称（文件路径）
	 * delayLoad 是否延迟加载动画资源
	 */
	Animation2D *get(const char *name, bool delayLoad);
	/**
	 * 释放2D动画对象
	 * delayDestroy 是否延迟销毁动画资源
	 */
	void release(Animation2D *ani, bool delayDestroy);
	/**
	 * 增量加载 参考ResourceManagerT
	 */
	float doIncrementLoad(bool stepLoad)
	{
		return m_resourceManager.doIncrementLoad(stepLoad);
	}
	/**
	 * 释放全部资源
	 */
	void releaseAll()
	{
		m_resourceManager.releaseAll();
	}
	/**
	 * 更新所有激活的动画
	 * timeStep 距上次update的时间间隔，单位毫秒
	 */
	void update(int timeStep);

	/**
	 * 添加激活动画。由Animation2D::play调用
	 */
	void addActiveAnimation(Animation2D *ani)
	{
		m_activeSet.insert(ani);
	}
	/**
	 * 移除激活动画。由Animation2D::stop调用
	 */
	void removeActiveAnimation(Animation2D *ani)
	{
		m_activeSet.erase(ani);
	}
};

#endif
