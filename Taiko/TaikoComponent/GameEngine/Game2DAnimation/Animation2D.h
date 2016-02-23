#ifndef ANIMATION2D_H
#define ANIMATION2D_H

#include <Game2DAnimation/Animation2DManager.h>
#include <Game2DImage/Game2DImageNode.h>
#include <Matrix2DMath.h>
#include <map>
#include <string>

/**
 * 2D动画类型
 * 使用Animation2DManager管理加载、释放和统一更新
 */

class Animation2D
{
private:
	Game2DImageNode m_node;
	const Animation2DResource &m_resource;
	int m_playTime;
	int m_loop;
	typedef std::map<std::string, Animation2DAction> ActionMap;
	ActionMap m_actionMap;
	Animation2DAction *m_currentAction;
	int m_currentFrame;
	std::string m_actionName;
public:
	/**
	 * 构造函数。由资源管理器调用
	 */
	Animation2D(const Animation2DResource &resource)
		: m_resource(resource), m_playTime(0), m_loop(-1), m_currentAction(NULL)
	{}
	/**
	 * 析构函数。由资源管理器调用
	 */
	~Animation2D();
	/**
	 * 从头播放动画
	 * loop 是否循环。0表示不循环，1表示循环，-1表示使用默认值
	 */
	void play(int loop =-1);
	/**
	 * 停止播放
	 */
	void stop();
	/**
	 * 暂停
	 */
	void pause();
	/**
	 * 继续
	 */
	void resume();
	/**
	 * 是否正在播放
	 * timeStep 距上次update的时间间隔
	 */
	bool isPlaying(int timeStep=0);
	/**
	 * 切换动作
	 */
	void changeAction(const char *name);
	/**
	 * 返回当前动作名称
	 */
	const char *getCurrentAction()
	{
		return m_actionName.c_str();
	}
	/**
	 * 返回默认action名称
	 */
	const char *getDefaultAction() const
	{
		return m_resource.getDefaultActionName();
	}
	/**
	 * 更新动画。一般由Animation2DManager统一调用，或者手动调用以改变播放进度
	 * timeStep 距上次update的时间间隔
	 * return false表示停止播放
	 */
	bool update(int timeStep);
	/**
	 * 返回当前2d动画物体的根节点
	 * 可以对此根节点操作，但不要操作其子节点
	 */
	Game2DImageNode *getNode()
	{
		return &m_node;
	}
	/**
	 * 返回资源名称
	 */
	const char *getName()
	{
		return m_resource.getName();
	}
	/**
	 * 点碰撞检测。点坐标相对与屏幕，范围0-1
	 * frame 使用当前action的第几桢的碰撞数据。-1表示当前桢
	 */
	bool collisionTest(float x, float y, int frame=-1);
	/**
	 * 返回当前桢的引用点数量
	 */
	int getReferencePointCount();
	/**
	 * 返回指定索引的引用点的原始坐标
	 */
	void getReferencePoint(int i, float &x, float &y);
	/**
	 * 返回指定索引的引用点的屏幕坐标，范围0-1
	 */
	void getReferencePointPosition(int i, float &x, float &y);
};

#endif
