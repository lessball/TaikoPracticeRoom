#ifndef GAME2DIMAGERENDER_H
#define GAME2DIMAGERENDER_H

#include <RenderCore/RenderCore.h>
#include <Game2DImage/Game2DImageNode.h>

/**
 * 2D图形的渲染器
 */
class Game2DImageRender
{
private:
	RenderAtom *m_texAtom;
	RenderAtom *m_solidAtom;
	RenderAtom *m_frameAtom;
	RenderMesh *m_texMesh;
	RenderMesh *m_frameMesh;
	float m_matrix[6];
	float m_color[4];
	float m_texRange[4];
	RenderSampler m_sampler;
	Game2DImageNode m_root;
	int m_blendState;
	RenderBlendState *m_blendStateAdd;
	RenderBlendState *m_blendStateAlpha;
	void renderNodeIteration(RenderCore *rc, Game2DImageNode *node, const float *parentColor, const int *parentClip);
public:
	Game2DImageRender();
	/**
	 * 初始化
	 * rc 渲染核心对象
	 * elableSolidMode 是否允许纯色填充模式
	 * enableFrameMode 是否允许边框模式
	 */
	void init(RenderCore *rc, bool enableSolidMode, bool enableFrameMode);
	/**
	 * 释放资源
	 */
	void release(RenderCore *rc);
	/**
	 * 返回根节点
	 */
	Game2DImageNode *getRootNode();
	/**
	 * 设置视口大小，将会重新计算根节点的矩阵(用于像素空间->投影空间变换)。特殊情况也可不使用此方法，直接对根节点操作。
	 */
	void setViewPortSize(int w, int h);
	/**
	 * 渲染所有节点。会更新节点的矩阵
	 */
	void render(RenderCore *rc);
	/**
	 * 渲染一个节点及其子节点。会更新节点的矩阵
	 * node 要渲染的节点
	 * parentColor 父节点的颜色
	 * parentClip 父节点的裁剪([left, top, width, height])，将与自身裁剪合并,
	 */
	void renderNode(RenderCore *rc, Game2DImageNode *node, const float *parentColor, const int *parentClip);
};

#endif
