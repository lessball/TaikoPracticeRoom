#include "Game2DImageRender.h"
#include <algorithm>
using namespace std;

void Game2DImageRender::renderNodeIteration(RenderCore *rc, Game2DImageNode *node, const float *parentColor, const int *parentClip)
{
	assert(m_texAtom != NULL);
	if(!node->isNeedRender())
		return;
	int mode = node->getDrawMode();
	int flag = node->getDrawFlag();
	bool colorChild = (flag & Game2DImageNode::DRAW_FLAG_COLOR_CHILD) != 0;
	bool clipChild = (flag & Game2DImageNode::DRAW_FLAG_CLIP_CHILD) != 0;
	bool blendAdd = (flag & Game2DImageNode::DRAW_FLAG_BLEND_ADD) != 0;
	int numChild = node->getChildCount();

	float color[4];
	if(parentColor != NULL)
	{
		//合成父节点与自身的颜色
		for(int i=0; i<4; i++)
			color[i] = parentColor[i] * node->m_color[i];
	}else
	{
		memcpy(color, node->m_color, sizeof(color));
	}
	bool clipVisible = true;
	int clip[4] = {-1, -1, -1, -1};
	if(node->m_clipRect[2] >= 0)
	{
		//设置裁剪
		if(parentClip != NULL && parentClip[2] >= 0)
		{
			//合成父节点与自身的裁剪
			int cxmin = max(node->m_clipRect[0], parentClip[0]);
			int cymin = max(node->m_clipRect[1], parentClip[1]);
			int cxmax = min(node->m_clipRect[0]+node->m_clipRect[2], parentClip[0]+parentClip[2]);
			int cymax = min(node->m_clipRect[1]+node->m_clipRect[3], parentClip[1]+parentClip[3]);
			if(cxmin <= cxmax && cymin <= cymax)
			{
				clip[0] = cxmin;
				clip[1] = cymin;
				clip[2] = cxmax-cxmin;
				clip[3] = cymax-cymin;
				rc->setScissorTest(true, cxmin, cymin, cxmax, cymax);
			}else
			{
				clipVisible = false; //裁剪范围没有交集，节点不可见
			}
		}else
		{
			memcpy(clip, node->m_clipRect, sizeof(clip));
			rc->setScissorTest(true, clip[0], clip[1], clip[0]+clip[2], clip[1]+clip[3]);
		}
	}
	if(clipVisible && (flag & Game2DImageNode::DRAW_FLAG_HIDE) == 0)
	{
		if(m_blendState != 2)
		{
			if(m_blendState != 1 && blendAdd)
			{
				rc->setBlendState(m_blendStateAdd);
				m_blendState = 1;
			}else if(m_blendState != 0 && !blendAdd)
			{
				rc->setBlendState(m_blendStateAlpha);
				m_blendState = 0;
			}
		}
		memcpy(m_matrix, node->getFinalMatrix(), sizeof(m_matrix));
		memcpy(m_color, color, sizeof(m_color));
		memcpy(m_texRange, node->m_texcoordRange, sizeof(m_texRange));
		if(mode == Game2DImageNode::DRAW_MODE_TEXTURE || mode == Game2DImageNode::DRAW_MODE_FRAME_TEXTURE)
		{
			if(node->m_texture != NULL && node->m_texture->getTexture() != NULL)
			{
				m_sampler.texture = node->getRenderTexture();
				rc->accept(m_texAtom);
			}
		}
		if((mode == Game2DImageNode::DRAW_MODE_FRAME || mode == Game2DImageNode::DRAW_MODE_FRAME_TEXTURE) && m_frameAtom != NULL)
			rc->accept(m_frameAtom);
		if(mode == Game2DImageNode::DRAW_MODE_SOLID && m_solidAtom != NULL)
			rc->accept(m_solidAtom);
	}
	if((flag & Game2DImageNode::DRAW_FLAG_HIDE_CHILD) == 0 && numChild > 0 && (clipVisible || (flag & Game2DImageNode::DRAW_FLAG_CLIP_CHILD) == 0))
	{
		bool setBlendAddChild = (flag &  Game2DImageNode::DRAW_FLAG_BLEND_ADD_CHILD) != 0 && m_blendState != 2;
		if(setBlendAddChild)
		{
			if(m_blendState != 1)
				rc->setBlendState(m_blendStateAdd);
			m_blendState = 2;
		}
		const float *childColor = colorChild ? color : parentColor;
		const int *childClip = clipChild ? clip : parentClip;
		for(int i=0; i<numChild; i++)
			renderNodeIteration(rc, node->getChild(i), childColor, childClip);
		if(node->m_clipRect[2] >= 0)
		{
			if(parentClip != NULL && parentClip[2] >= 0)
			{
				rc->setScissorTest(true, parentClip[0], parentClip[1], parentClip[0]+parentClip[2], parentClip[1]+parentClip[3]);
			}else
			{
				rc->setScissorTest(false);
			}
		}
		if(setBlendAddChild)
			m_blendState = 1;
	}
}
Game2DImageRender::Game2DImageRender()
	:m_texAtom(NULL), m_solidAtom(NULL), m_frameAtom(NULL), m_texMesh(NULL), m_frameMesh(NULL), m_blendStateAdd(NULL), m_blendStateAlpha(NULL)
{
	m_sampler.texture = NULL;
	m_sampler.filter = TF_TRILINEAR;
	m_sampler.anisotropy = 0;
	m_sampler.wrapx = TW_REPEAT;
	m_sampler.wrapy = TW_REPEAT;
}
void Game2DImageRender::init(RenderCore *rc, bool enableSolidMode, bool enableFrameMode)
{
	assert(m_texMesh == NULL && rc != NULL);
	m_sampler.texture = NULL;
	m_blendState = -1;
	VertexElement texVertEle[] = {{VE_POSITION, 0, VET_FLOAT2}, {VE_TEXCOORD, 0, VET_FLOAT2}};
	float texVert[] = {0.0f,0.0f,0.0f,0.0f, 0.0f,1.0f,0.0f,1.0f, 1.0f,0.0f,1.0f,0.0f, 1.0f,1.0f,1.0f,1.0f};
	m_texMesh = rc->createMesh(texVertEle, 2, texVert, 4, 0, NULL, 0, MT_TRIANGLE_STRIP);
	RenderParam texParams[] =
	{
		{NULL, {(float*)m_texMesh}, PT_MESH, 1, 1, 1},
		{"g_tex0", {(float*)&(m_sampler)}, PT_SAMPLER, 1, 1, 1},
		{"g_matrix", {m_matrix}, PT_FLOAT, 3, 1, 2},
		{"g_color", {m_color}, PT_FLOAT, 4, 1, 1},
		{"g_texRange", {m_texRange}, PT_FLOAT, 4, 1, 1},
	};
	m_texAtom = rc->createRenderAtom("image2d", texParams, 5);
	if(enableSolidMode)
	{
		RenderParam solidParams[] =
		{
			{NULL, {(float*)m_texMesh}, PT_MESH, 1, 1, 1},
			{"g_matrix", {m_matrix}, PT_FLOAT, 3, 1, 2},
			{"g_color", {m_color}, PT_FLOAT, 4, 1, 1},
		};
		m_solidAtom = rc->createRenderAtom("simple2dcolor", solidParams, 3);
	}
	if(enableFrameMode)
	{
		VertexElement frameVertEle[] = {{VE_POSITION, 0, VET_FLOAT2}};
		float frameVert[] = {0.0f,0.0f, 0.0f,1.0f, 1.0f,1.0f, 1.0f,0.0f, 0.0f,0.0f};
		m_frameMesh = rc->createMesh(frameVertEle, 1, frameVert, 5, 0, NULL, 0, MT_LINE_STRIP);
		RenderParam frameParams[] =
		{
			{NULL, {(float*)m_frameMesh}, PT_MESH, 1, 1, 1},
			{"g_matrix", {m_matrix}, PT_FLOAT, 3, 1, 2},
			{"g_color", {m_color}, PT_FLOAT, 4, 1, 1},
		};
		m_frameAtom = rc->createRenderAtom("simple2dcolor", frameParams, 3);
	}
	BlendStateDesc blendDesc;
	blendDesc.blendEnable = true;
	blendDesc.colorSrc = blendDesc.alphaSrc = BLEND_ONE;
	blendDesc.colorDst = blendDesc.alphaDst = BLEND_ONE;
	blendDesc.colorOp = blendDesc.alphaOp = BLEND_ADD;
	for(int i=0; i<4; i++)
		blendDesc.colorWriteEnable[i] = true;
	m_blendStateAdd = rc->createBlendState(&blendDesc);
	blendDesc.colorSrc = blendDesc.alphaSrc = BLEND_SRC_ALPHA;
	blendDesc.colorDst = blendDesc.alphaDst = BLEND_INV_SRC_ALPHA;
	m_blendStateAlpha = rc->createBlendState(&blendDesc);
}
void Game2DImageRender::release(RenderCore *rc)
{
	m_root.releaseTexture(false);
	if(m_texAtom != NULL)
	{
		rc->destroyRenderAtom(m_texAtom);
		m_texAtom = NULL;
	}
	if(m_solidAtom != NULL)
	{
		rc->destroyRenderAtom(m_solidAtom);
		m_solidAtom = NULL;
	}
	if(m_frameAtom != NULL)
	{
		rc->destroyRenderAtom(m_frameAtom);
		m_frameAtom = NULL;
	}
	if(m_texMesh != NULL)
	{
		rc->destroyMesh(m_texMesh);
		m_texMesh = NULL;
	}
	if(m_frameMesh != NULL)
	{
		rc->destroyMesh(m_frameMesh);
		m_frameMesh = NULL;
	}
	if(m_blendStateAdd != NULL)
	{
		rc->destroyBlendState(m_blendStateAdd);
		m_blendStateAdd = NULL;
	}
	if(m_blendStateAlpha != NULL)
	{
		rc->destroyBlendState(m_blendStateAlpha);
		m_blendStateAlpha = NULL;
	}
}
Game2DImageNode *Game2DImageRender::getRootNode()
{
	return &m_root;
}
void Game2DImageRender::setViewPortSize(int w, int h)
{
	float rootMatrix[] = {2.0f/w,0.0f,-1.0f, 0.0f,-2.0f/h,1.0f};
	m_root.setLocalMatrix(rootMatrix);
}
void Game2DImageRender::render(RenderCore *rc)
{
	renderNode(rc, &m_root, NULL, NULL);
}
void Game2DImageRender::renderNode(RenderCore *rc, Game2DImageNode *node, const float *parentColor, const int *parentClip)
{
	rc->setBlendState(m_blendStateAlpha);
	rc->setCullMode(CULL_NONE);
	rc->setAlphaTest(false);
	m_blendState = 0;
	renderNodeIteration(rc, node, parentColor, parentClip);
	m_blendState = -1;
}
