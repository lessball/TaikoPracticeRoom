#include "Game2DImageNode.h"
#include <assert.h>

Game2DImageNode::Game2DImageNode()
	:m_drawMode(DRAW_MODE_TEXTURE), m_drawFlag(DRAW_FLAG_HIDE), m_texture(NULL)
{
	m_absoluteMatrix[0] = FLOAT_NAN;
	m_finalMatrix[0] = FLOAT_NAN;
	matrix2DIdentity(m_objectMatrix);
	m_localMatrix[0] = FLOAT_NAN;
	m_texcoordRange[0] = m_texcoordRange[2] = 0.0f;
	m_texcoordRange[1] = m_texcoordRange[3] = 1.0f;
	m_color[0] = m_color[1] = m_color[2] = m_color[3] = 1.0f;
	m_clipRect[2] = -1;
}
Game2DImageNode::Game2DImageNode(const Game2DImageNode &src, bool texRef)
{
	memcpy(m_localMatrix, src.m_localMatrix, sizeof(m_localMatrix));
	memcpy(m_objectMatrix, src.m_objectMatrix, sizeof(m_objectMatrix));
	dirtyMatrix();
	m_drawMode = texRef ? src.m_drawMode | 0x8000 : src.m_drawMode & 0x7fff;
	m_drawFlag = src.m_drawFlag;
	if(texRef && src.m_texture != NULL)
	{
		m_texture = TextureManager::getSingleton()->get(src.m_texture->getName(), true);
	}else
	{
		m_texture = src.m_texture;
	}
	memcpy(m_texcoordRange, src.m_texcoordRange, sizeof(m_texcoordRange));
	memcpy(m_color, src.m_color, sizeof(m_color));
	memcpy(m_clipRect, src.m_clipRect, sizeof(m_clipRect));
}
Game2DImageNode::~Game2DImageNode()
{
	releaseTexture(false);
}
Game2DImageNode *Game2DImageNode::cloneBranch(bool texRef) const
{
	Game2DImageNode *newNode = new Game2DImageNode(*this, texRef);
	for(int i=0; i<(int)m_children.size(); i++)
		newNode->addChild(m_children[i]->cloneBranch(texRef));
	return newNode;
}
void Game2DImageNode::deleteBranch(Game2DImageNode *node, bool delay)
{
	int numChild = node->getChildCount();
	for(int i=0; i<numChild; i++)
		deleteBranch(node->getChild(i), delay);
	node->releaseTexture(delay);
	delete node;
}
void Game2DImageNode::deleteChildBranch(bool delay)
{
	for(int i=0; i<(int)m_children.size(); i++)
		deleteBranch(m_children[i], delay);
	m_children.clear();
}
int Game2DImageNode::getChildCount() const
{
	return m_children.size();
}
Game2DImageNode *Game2DImageNode::getChild(int index)
{
	assert(index>=0 && index < (int)m_children.size());
	return m_children[index];
}
void Game2DImageNode::addChild(Game2DImageNode *node)
{
	assert(node != NULL);
	m_children.push_back(node);
	node->dirtyMatrix();
}
void Game2DImageNode::replaceChild(Game2DImageNode *node, int index)
{
	assert(index>=0 && index < (int)m_children.size() && node != NULL);
	m_children[index] = node;
	node->dirtyMatrix();
}
void Game2DImageNode::insertChild(Game2DImageNode *node, int index)
{
	assert(index>=0 && index <= (int)m_children.size() && node != NULL);
	if(index > (int)m_children.size())
		index = (int)m_children.size();
	m_children.insert(m_children.begin()+index, node);
	node->dirtyMatrix();
}
void Game2DImageNode::removeChild(int index)
{
	assert(index>=0 && index < (int)m_children.size());
	m_children.erase(m_children.begin()+index);
}
void Game2DImageNode::moveChild(int from, int to)
{
	assert(from != to && from>=0 && from < (int)m_children.size() && to>=0 && to < (int)m_children.size());
	Game2DImageNode *tnode = m_children[from];
	int direction = from < to ? 1 : -1;
	for(int i=from; i!=to; i+=direction)
		m_children[i] = m_children[i+1];
	m_children[to] = tnode;
}
void Game2DImageNode::swapChild(int a, int b)
{
	assert(a != b && a>=0 && a < (int)m_children.size() && b>=0 && b < (int)m_children.size());
	Game2DImageNode *tnode = m_children[a];
	m_children[a] = m_children[b];
	m_children[b] = tnode;
}
void Game2DImageNode::clearChild()
{
	m_children.clear();
}
void Game2DImageNode::swapChildren(Game2DImageNode *other)
{
	assert(other != NULL);
	for(int i=0; i<(int)m_children.size(); i++)
		m_children[i]->dirtyMatrix();
	m_children.swap(other->m_children);
	for(int i=0; i<(int)m_children.size(); i++)
		m_children[i]->dirtyMatrix();
}

void Game2DImageNode::setLocalMatrix(const float *data)
{
	if(data == NULL)
	{
		m_localMatrix[0] = FLOAT_NAN;
	}else
	{
		memcpy(m_localMatrix, data, sizeof(m_localMatrix));
	}
	dirtyMatrix();
}
const float *Game2DImageNode::getLocalMatrix() const
{
	return m_localMatrix;
}
void Game2DImageNode::setObjectMatrix(const float *data)
{
	if(data == NULL)
	{
		if(m_texture != NULL)
		{
			matrix2DScalePosition(m_objectMatrix, m_texture->getWidth()*(m_texcoordRange[1]-m_texcoordRange[0]), m_texture->getHeight()*(m_texcoordRange[3]-m_texcoordRange[2]), 0.0f, 0.0f);
		}else
		{
//			m_objectMatrix.identity();
			m_objectMatrix[0] = FLOAT_NAN;
		}
	}else
	{
		memcpy(m_objectMatrix, data, sizeof(m_objectMatrix));
	}
	m_finalMatrix[0] = FLOAT_NAN;
}
bool Game2DImageNode::updateMatrix(const float *parentMatrix, bool parentDirty)
{
	if(parentDirty || isNaN(m_absoluteMatrix[0]))
	{
		if(!isNaN(m_localMatrix[0]))
		{
			if(parentMatrix == NULL)
			{
				memcpy(m_absoluteMatrix, m_localMatrix, sizeof(m_absoluteMatrix));
			}else
			{
				matrix2DMul(m_absoluteMatrix, m_localMatrix, parentMatrix);
			}
		}else
		{
			if(parentMatrix != NULL)
			{
				memcpy(m_absoluteMatrix, parentMatrix, sizeof(m_absoluteMatrix));
			}else
			{
				matrix2DIdentity(m_absoluteMatrix);
			}
		}
		matrix2DMul(m_finalMatrix, m_objectMatrix, m_absoluteMatrix);
		return true;
	}else if(isNaN(m_finalMatrix[0]))
	{
		matrix2DMul(m_finalMatrix, m_objectMatrix, m_absoluteMatrix);
	}
	return false;
}
void Game2DImageNode::updateBranchMatrix(const float *parentMatrix, bool parentDirty)
{
	bool dirty = updateMatrix(parentMatrix, parentDirty);
	for(int i=0; i<(int)m_children.size(); i++)
		m_children[i]->updateBranchMatrix(m_absoluteMatrix, dirty);
}
void Game2DImageNode::updateVisibleBranchMatrix(const float *parentMatrix, bool parentDirty)
{
	int numChild = m_children.size();
	if(!isNeedRender())
	{
		//不可见，不需要立即更新矩阵
		if(parentDirty)
			dirtyMatrix(); //标记矩阵需要更新
		return;
	}
	bool dirty = updateMatrix(parentMatrix, parentDirty);
	for(int i=0; i<numChild; i++)
		m_children[i]->updateVisibleBranchMatrix(m_absoluteMatrix, dirty);
}
void Game2DImageNode::setTextureName(const char *name, bool delayLoad)
{
	releaseTexture(delayLoad);
	if(name != NULL)
	{
		m_texture = TextureManager::getSingleton()->get(name, delayLoad);
		m_drawMode |= 0x8000;
	}
}
const char *Game2DImageNode::getTextureName() const
{
	return m_texture != NULL ? m_texture->getName() : NULL;
}
void Game2DImageNode::setTextureResource(TextureResource *tex)
{
	releaseTexture(false);
	m_texture = tex;
	m_drawMode &= 0x7fff;
}
TextureResource *Game2DImageNode::getTextureResource()
{
	return m_texture;
}
RenderTexture *Game2DImageNode::getRenderTexture()
{
	return m_texture != NULL ? m_texture->getTexture() : NULL;
}
void Game2DImageNode::releaseTexture(bool delay)
{
	if(m_texture != NULL && (m_drawMode & 0x8000) != 0)
	{
		TextureManager::getSingleton()->release(m_texture, delay);
		m_texture = NULL;
	}
}
void Game2DImageNode::setTexcoordRange(float xmin, float xmax, float ymin, float ymax)
{
	m_texcoordRange[0] = xmin;
	m_texcoordRange[1] = xmax;
	m_texcoordRange[2] = ymin;
	m_texcoordRange[3] = ymax;
}
void Game2DImageNode::getTexcoordRange(float &xmin, float &xmax, float &ymin, float &ymax) const
{
	xmin = m_texcoordRange[0];
	xmax = m_texcoordRange[1];
	ymin = m_texcoordRange[2];
	ymax = m_texcoordRange[3];
}
int Game2DImageNode::getTextureUsedWidth() const
{
	return m_texture != NULL ? (int)(m_texture->getWidth() * (m_texcoordRange[1] - m_texcoordRange[0])) : 0;
}
int Game2DImageNode::getTextureUsedHeight() const
{
	return m_texture != NULL ?  (int)(m_texture->getHeight() * (m_texcoordRange[3] - m_texcoordRange[2])) : 0;
}
void Game2DImageNode::setColor(float red, float green, float blue, float alpha)
{
	m_color[0] = red;
	m_color[1] = green;
	m_color[2] = blue;
	m_color[3] = alpha;
}
void Game2DImageNode::getColor(float &red, float &green, float &blue, float &alpha) const
{
	red = m_color[0];
	green = m_color[1];
	blue = m_color[2];
	alpha = m_color[3];
}
void Game2DImageNode::setClipRect(int left, int top, int width, int height)
{
	m_clipRect[0] = left;
	m_clipRect[1] = top;
	m_clipRect[2] = width;
	m_clipRect[3] = height;
}
void Game2DImageNode::getClipRect(int &left, int &top, int &width, int &height) const
{
	left = m_clipRect[0];
	top = m_clipRect[1];
	width = m_clipRect[2];
	height = m_clipRect[3];
}
