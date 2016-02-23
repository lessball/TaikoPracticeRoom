#ifndef GAME2DIMAGENODE
#define GAME2DIMAGENODE

#include <RenderResource/TextureManager.h>
#include <Matrix2DMath.h>
#include <string>
#include <vector>

/**
 * 2d图形节点
 * 使用3x2的2D矩阵表示变换。无变换的情况下对应坐标范围0-1的正方形
 */
class Game2DImageNode
{
private:
	friend class Game2DImageRender;
	std::vector<Game2DImageNode*> m_children;
	float m_localMatrix[6];
	float m_objectMatrix[6];
	float m_absoluteMatrix[6];
	float m_finalMatrix[6];
	short m_drawMode; //最高位表示是否控制texture引用
	short m_drawFlag;
	TextureResource *m_texture;
	float m_texcoordRange[4]; //x min, x max, y min, y max
	float m_color[4];
	int m_clipRect[4]; //x min, x max, y min, y max
	Game2DImageNode &operator=(const Game2DImageNode &other);
public:
	/** 渲染模式 */
	enum DrawMode
	{
		DRAW_MODE_TEXTURE, //渲染贴图
		DRAW_MODE_FRAME, //渲染边框
		DRAW_MODE_FRAME_TEXTURE, //渲染贴图和边框
		DRAW_MODE_SOLID, //纯色填充
//		DRAW_CUSTOM, //自定义
	};
	enum DrawFlag
	{
		DRAW_FLAG_HIDE = 1, //不渲染此节点
		DRAW_FLAG_HIDE_CHILD = 2, //不渲染子节点
		DRAW_FLAG_COLOR_CHILD = 4, //表示自身颜色将影响子节点
		DRAW_FLAG_CLIP_CHILD = 8, //表示自身裁剪将影响子节点
		DRAW_FLAG_BLEND_ADD = 16, //使用加法模式混合
		DRAW_FLAG_BLEND_ADD_CHILD = 32, //使用加法模式混合所有子节点
	};
	Game2DImageNode();
	/**
	 * 拷贝构造函数
	 * 仅复制节点本身，不会复制子节点
	 * texRef 是否增加贴图资源的引用
	 */
	Game2DImageNode(const Game2DImageNode &src, bool texRef=true);
	~Game2DImageNode();

	/**
	 * 将当前节点连同所有子节点一起拷贝
	 * texRef 是否增加贴图资源的引用
	 */
	Game2DImageNode *cloneBranch(bool texRef) const;
	/**
	 * 删除指定节点及其所有子节点
	 * node 要删除的根节点
	 * delay 贴图资源是否延迟释放
	 */
	static void deleteBranch(Game2DImageNode *node, bool delay);
	/**
	 * 删除当前节点的所有子节点，但保留当前节点
	 * node 要删除的根节点
	 * delay 贴图资源是否延迟释放
	 */
	void deleteChildBranch(bool delay);

	/** 返回子节点数量 */
	int getChildCount() const;
	/** 返回index位置的子节点 */
	Game2DImageNode *getChild(int index);
	/** 添加子节点 */
	void addChild(Game2DImageNode *node);
	/** 替换index位置的子节点 */
	void replaceChild(Game2DImageNode *node, int index);
	/** 在index位置插入子节点 */
	void insertChild(Game2DImageNode *node, int index);
	/** 移除index位置的子节点 */
	void removeChild(int index);
	/** 将from位置的子节点移动到to位置 */
	void moveChild(int from, int to);
	/** 交换a与b位置的子节点 */
	void swapChild(int a, int b);
	/** 移除所有子节点 */
	void clearChild();
	/** 与另一个节点交换全部子节点 */
	void swapChildren(Game2DImageNode *other);

	/**
	 * 设置局部矩阵。表示相对与父节点的变换，会影响自身所有子节点
	 * data 3x2矩阵。可以用NULL表示单位矩阵
	 */
	void setLocalMatrix(const float *data);
	/**
	 * 返回局部矩阵指针
	 */
	const float *getLocalMatrix() const;
	/**
	 * 标记矩阵需要更新，但并不立即更新矩阵。如：父节点发生变化，而子节点处于DRAW_INVISIBLE_BRANCH状态时，可调用此方法
	 */
	void dirtyMatrix()
	{
		m_absoluteMatrix[0] = FLOAT_NAN;
	}
	/**
	 * 设置物体矩阵。
	 * 物体矩阵只影响自身的绘制，不会影响子节点。如：自身大小，相对自身原点的变换
	 * data 3x2矩阵。如果为NULL，则使用贴图的长宽和纹理坐标范围计算显示大小，生成缩放矩阵
	 */
	void setObjectMatrix(const float *data);
	/**
	 * 返回物体矩阵指针
	 */
	const float *getObjectMatrix() const
	{
		return m_objectMatrix;
	}
	/**
	 * 返回绝对矩阵。表示相对于世界原点的变换，不包含物体矩阵。可以用于子节点的矩阵计算。
	 */
	const float *getAbsoluteMatrix() const
	{
		return m_absoluteMatrix;
	}
	/**
	 * 返回最终矩阵的指针
	 */
	const float *getFinalMatrix() const
	{
		return m_finalMatrix;
	}
	/**
	 * 使用父节点矩阵更新矩阵
	 * parentMatrix 父节点的矩阵
	 * parentDirty 父节点的矩阵是否有变化
	 * return 自身的绝对矩阵是否发生变化
	 */
	bool updateMatrix(const float *parentMatrix, bool parentDirty);
	/**
	 * 更新自身及所有子节点的矩阵
	 * parentMatrix 父节点的矩阵
	 * parentDirty 父节点的矩阵是否有变化
	 */
	void updateBranchMatrix(const float *parentMatrix, bool parentDirty);
	/**
	 * 更新自身及子节点的矩阵，只更新可见的节点
	 * parentMatrix 父节点的矩阵
	 * parentDirty 父节点的矩阵是否有变化
	 */
	void updateVisibleBranchMatrix(const float *parentMatrix, bool parentDirty);
	/**
	 * 设置渲染模式
	 * mode DrawMode枚举值
	 */
	void setDrawMode(short mode)
	{
		m_drawMode = (m_drawMode & 0x8000) | mode;
	}
	/**
	 * 返回渲染模式
	 */
	short getDrawMode() const
	{
		return m_drawMode & 0x7fff;
	}
	/**
	 * 设置或渲染标志
	 * flag DrawFlag枚举值组合
	 */
	void setDrawFlag(short flag)
	{
		m_drawFlag = flag;
	}
	/**
	 * 返回渲染标志
	 */
	short getDrawFlag() const
	{
		return m_drawFlag;
	}
	/**
	 * 设置整个分支是否渲染
	 */
	void setBranchVisible(bool visible)
	{
		if(visible)
		{
			m_drawFlag &= ~(DRAW_FLAG_HIDE | DRAW_FLAG_HIDE_CHILD);
		}else
		{
			m_drawFlag |= DRAW_FLAG_HIDE | DRAW_FLAG_HIDE_CHILD;
		}
	}
	/**
	 * 返回是否需要渲染
	 */
	bool isNeedRender() const
	{
		return (m_drawFlag & DRAW_FLAG_HIDE) == 0 || ((m_drawFlag & DRAW_FLAG_HIDE_CHILD) == 0 && m_children.size() != 0);
	}
	/**
	 * 根据资源名称设置贴图
	 * delayLoad 是否延迟加载
	 */
	void setTextureName(const char *name, bool delayLoad);
	/**
	 * 返回贴图资源名称
	 */
	const char *getTextureName() const;
	/**
	 * 设置贴图。不增加贴图资源的引用，小心控制贴图资源的生命周期
	 */
	void setTextureResource(TextureResource *tex);
	/**
	 * 返回贴图资源
	 */
	TextureResource *getTextureResource();
	/**
	 * 返回贴图对象
	 */
	RenderTexture *getRenderTexture();
	/**
	 * 释放贴图资源
	 * delay 是否延迟释放
	 */
	void releaseTexture(bool delay);
	/**
	 * 设置贴图坐标范围
	 */
	void setTexcoordRange(float xmin, float xmax, float ymin, float ymax);
	/**
	 * 返回贴图坐标范围
	 */
	void getTexcoordRange(float &xmin, float &xmax, float &ymin, float &ymax) const;
	/**
	 * 根据贴图尺寸和贴图坐标范围计算显示宽度
	 */
	int getTextureUsedWidth() const;
	/**
	 * 根据贴图尺寸和贴图坐标范围计算显示高度
	 */
	int getTextureUsedHeight() const;
	/**
	 * 设置颜色
	 */
	void setColor(float red, float green, float blue, float alpha);
	/**
	 * 返回颜色
	 */
	void getColor(float &red, float &green, float &blue, float &alpha) const;
	/**
	 * 设置裁剪范围，width或height小于0表示不裁剪
	 */
	void setClipRect(int left, int top, int width, int height);
	/**
	 * 返回裁剪范围，width或height小于0表示不裁剪
	 */
	void getClipRect(int &left, int &top, int &width, int &height) const;
};

#endif
