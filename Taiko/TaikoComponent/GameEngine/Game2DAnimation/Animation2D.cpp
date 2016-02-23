#include <Game2DAnimation/Animation2D.h>
#include <algorithm>
#include <math.h>
#include <assert.h>
using namespace std;

Animation2D::~Animation2D()
{
	if(m_currentAction != NULL)
		m_node.swapChildren(m_currentAction->node);
	for(ActionMap::iterator i = m_actionMap.begin(); i!= m_actionMap.end(); ++i)
		Game2DImageNode::deleteBranch(i->second.node, false);
}
void Animation2D::play(int loop)
{
	assert(loop >= -1 && loop <= 1);
	m_playTime = 0;
	m_loop = loop;
	if(m_loop < 0 && m_currentAction != NULL)
		m_loop =  m_currentAction->loop ? 1 : 0;
	if(m_currentAction == NULL || m_node.getChildCount() != 1 || loop == 0)
	{
		Animation2DManager::getSingleton()->addActiveAnimation(this);
	}else
	{
		update(0); //只有一桢
	}
}
void Animation2D::stop()
{
	m_loop = -2;
	Animation2DManager::getSingleton()->removeActiveAnimation(this);
}
void Animation2D::pause()
{
	if(m_loop < -1)
		return;
	m_loop +=2;
	Animation2DManager::getSingleton()->removeActiveAnimation(this);
}
void Animation2D::resume()
{
	if(m_loop <= 1)
		return;
	m_loop -=2;
	Animation2DManager::getSingleton()->addActiveAnimation(this);
}
bool Animation2D::isPlaying(int timeStep)
{
	if(timeStep == 0)
	{
		return m_loop > -2 && m_loop < 2;
	}else
	{
		if(m_loop == -2)
			return false;
		if(m_currentAction == NULL)
			changeAction(m_actionName.empty() ? m_resource.getDefaultActionName() : m_actionName.c_str());
		int numChild = m_node.getChildCount();
		if(numChild <= 0)
			return true;
		int playTime = m_playTime + timeStep;
		float timeLength = m_currentAction->frame[numChild-1].endTime;
		int loop = m_loop < 2 ? m_loop : m_loop-2;
		return loop > 0 || playTime < timeLength;
	}
}
void Animation2D::changeAction(const char *name)
{
	assert(name != NULL);
	string sname(name);
	if(m_actionName == sname && m_currentAction != NULL)
		return;
	ActionMap::iterator iact = m_actionMap.find(sname);
	Animation2DAction *pact;
	if(iact == m_actionMap.end())
	{
		Animation2DAction act = m_resource.createAction(sname.c_str());
		if(act.frame == NULL)
		{
			if(m_currentAction == NULL)
				m_actionName.swap(sname);
			return;
		}
		pact = &(m_actionMap[sname]);
		*pact = act;
	}else
	{
		pact = &(iact->second);
	}
	m_actionName.swap(sname);
	if(m_currentAction != NULL)
		m_node.swapChildren(m_currentAction->node);
	m_node.swapChildren(pact->node);
	m_currentAction = pact;
	if(m_loop == -1)
		m_loop = m_currentAction->loop ? 1 : 0;
}
struct FrameTimeCompare
{
	bool operator () (const Animation2DAction::Frame &a, const Animation2DAction::Frame &b) const
	{
		return a.endTime < b.endTime;
	}
};
bool Animation2D::update(int timeStep)
{
	if(m_loop == -2)
		return false;
	if(m_currentAction == NULL)
		changeAction(m_actionName.empty() ? m_resource.getDefaultActionName() : m_actionName.c_str());
	int numChild = m_node.getChildCount();
	if(numChild <= 0)
		return true;
	m_playTime += timeStep;
	float timeLength = m_currentAction->frame[numChild-1].endTime;
	int loop = m_loop < 2 ? m_loop : m_loop-2;
	bool bcontinue = loop > 0 || m_playTime < timeLength;
	if(numChild > 1 && bcontinue)
	{
		Animation2DAction::Frame tframekey = {(float)m_playTime, NULL};
		if(loop > 0)
			tframekey.endTime = fmod(tframekey.endTime, timeLength);
		m_currentFrame = lower_bound(m_currentAction->frame, m_currentAction->frame + numChild, tframekey, FrameTimeCompare()) - m_currentAction->frame;
		if(m_currentFrame >= numChild)
			m_currentFrame = numChild - 1;
	}else
	{
		m_currentFrame = numChild -1;
	}
	for(int i=0; i<numChild; i++)
	{
		m_node.getChild(i)->setBranchVisible(i == m_currentFrame);
	}
	if(!bcontinue)
		m_loop = -2;
	return bcontinue;
}
bool Animation2D::collisionTest(float x, float y, int frame)
{
	float tmat[6];
	matrix2DInverse(tmat, m_node.getAbsoluteMatrix());
	float tp[2] = {2.0f*x-1.0f, 1.0f-2.0f*y};
	matrix2DTransform(tp, tp, tmat);
	Animation2DFrameInfo *info = m_currentAction->frame[frame < 0 ? m_currentFrame : frame].info;
	int numRect = info->collisionRect.size();
	for(int i=0; i<numRect; i++)
	{
		Animation2DFrameInfo::Rect &rec = info->collisionRect[i];
		if(tp[0] >= rec.xmin && tp[0] <= rec.xmax && tp[1] >= rec.ymin && tp[1] <= rec.ymax)
			return true;
	}
	return false;
}
int Animation2D::getReferencePointCount()
{
	return (int)m_currentAction->frame[m_currentFrame].info->referencePoint.size();
}
void Animation2D::getReferencePoint(int i, float &x, float &y)
{
	Animation2DFrameInfo::Point &pt = m_currentAction->frame[m_currentFrame].info->referencePoint[i];
	x = pt.x;
	y = pt.y;
}
void Animation2D::getReferencePointPosition(int i, float &x, float &y)
{
	Animation2DFrameInfo::Point &pt = m_currentAction->frame[m_currentFrame].info->referencePoint[i];
	float tp[2] ={x, y};
	matrix2DTransform(tp, tp, m_node.getAbsoluteMatrix());
	x = tp[0] * 0.5f + 0.5f;
	y = 0.5f - tp[1] * 0.5f;
}
