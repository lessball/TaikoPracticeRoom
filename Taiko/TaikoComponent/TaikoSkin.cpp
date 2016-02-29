#include "TaikoSkin.h"
#include "TaikoGame.h"
#include "GameBGM.h"
#include "time.h"

TaikoSkin::TaikoSkin()
{
	for (int i = 0; i < (int)(sizeof(m_bk) / sizeof(m_bk[0])); i++)
		m_bk[i] = NULL;
	m_fg = NULL;
	m_hitJudge = NULL;
	for(int i=0; i<(int)(sizeof(m_number)/sizeof(m_number[0])); i++)
		m_number[i] = NULL;
	for(int i=0; i<(int)(sizeof(m_balloonNumber)/sizeof(m_balloonNumber[0])); i++)
		m_balloonNumber[i] = NULL;
	m_soulAni = NULL;
	m_branch = NULL;
	m_gogoBK = NULL;
	m_soulBurnTex = NULL;
}
TaikoSkin::~TaikoSkin()
{
}
void TaikoSkin::init()
{
	m_bk[0] = Animation2DManager::getSingleton()->get("res/bk.ste", false);
	if(m_bk[0] == NULL)
		return;
	m_bk[0]->changeAction("bk");
	m_bk[0]->play();
	if(m_bk[0]->getReferencePointCount() < 4)
		return;
	m_bk[1] = Animation2DManager::getSingleton()->get("res/bk.ste", false);
	m_bk[1]->changeAction("drum");
	m_bk[1]->play();
	m_bk[2] = Animation2DManager::getSingleton()->get("res/bk.ste", false);
	m_bk[2]->changeAction("bk1");
	m_bk[2]->play();
	float localMat[6] = {1.0f,0.0f,0.0f, 0.0f,1.0f,0.0f};
	m_bk[0]->getReferencePoint(0, localMat[2], localMat[5]);
	m_noteRoot.setLocalMatrix(localMat);
	m_lineRoot.setLocalMatrix(localMat);
	m_balloonRoot.setLocalMatrix(localMat);
	float hitRange[4];
	m_bk[0]->getReferencePoint(1, hitRange[0], hitRange[1]);
	m_bk[0]->getReferencePoint(2, hitRange[2], hitRange[3]);
	m_hitCenter[0] = hitRange[2];
	m_hitCenter[1] = hitRange[1];
	m_hitScale[0] = 1.0f / (hitRange[2]-hitRange[0]);
	m_hitScale[1] = 1.0f / (hitRange[1]-hitRange[3]);
	m_bk[0]->getReferencePoint(3, localMat[2], localMat[5]);
	m_soulBar[0].setLocalMatrix(localMat);
	m_fg = Animation2DManager::getSingleton()->get("res/fg.ste", false);
	if(m_fg == NULL)
		return;
	m_fg->changeAction("fg");
	m_fg->play();
	if(m_fg->getReferencePointCount() < 1)
		return;
	m_fg->getReferencePoint(0, localMat[2], localMat[5]);
	m_comboRoot.setLocalMatrix(localMat);
	m_width = 1280.0f - localMat[2];
	m_hitJudge = Animation2DManager::getSingleton()->get("res/judge.ste", false);
	if(m_hitJudge == NULL)
		return;
	m_hitJudgeBk = Animation2DManager::getSingleton()->get("res/judge_bk.ste", false);
	if(m_hitJudgeBk == NULL)
		return;
	m_hitFlash = Animation2DManager::getSingleton()->get("res/hitflash.ste", false);
	if(m_hitFlash == NULL)
		return;
	const char *drumAction[4] = {"left_red", "right_red", "left_blue", "right_blue"};
	for(int i=0; i<4; i++)
	{
		m_hitDrum[i] = Animation2DManager::getSingleton()->get("res/hitdrum.ste", false);
		if(m_hitDrum[i] == NULL)
			return;
		m_hitDrum[i]->changeAction(drumAction[i]);
	}
	m_hitMark = Animation2DManager::getSingleton()->get("res/hitmark.ste", false);
	if(m_hitMark == NULL)
		return;
	m_hitMark->changeAction("mark");
	m_hitMark->play();
	for(int i=0; i<(int)(sizeof(m_number)/sizeof(m_number[0])); i++)
		m_number[i] = Animation2DManager::getSingleton()->get("res/combo.ste", false);
	for(int i=0; i<(int)(sizeof(m_balloonNumber)/sizeof(m_balloonNumber[0])); i++)
		m_balloonNumber[i] = Animation2DManager::getSingleton()->get("res/combo.ste", false);
	m_soulAni = Animation2DManager::getSingleton()->get("res/soul.ste", false);
	if(m_soulAni == NULL)
		return;
	for(int i=1; i<(int)(sizeof(m_soulBar)/sizeof(m_soulBar[0])); i++)
		m_soulBar[0].addChild(m_soulBar+i);
	m_soulBar[0].addChild(m_soulAni->getNode());
	m_soulAni->changeAction("burn");
	Game2DImageNode *pBurnNode = m_soulAni->getNode();
	while(pBurnNode->getChildCount() > 0)
		pBurnNode = pBurnNode->getChild(0);
	m_soulBurnTex = pBurnNode->getTextureResource();
	pBurnNode->getTexcoordRange(m_soulBurnTexRange[0], m_soulBurnTexRange[1], m_soulBurnTexRange[2], m_soulBurnTexRange[3]);
	m_branch = Animation2DManager::getSingleton()->get("res/branch.ste", false);
	if(m_branch == NULL)
		return;
	m_gogoBK = Animation2DManager::getSingleton()->get("res/gogobk.ste", false);
	if(m_gogoBK == NULL)
		return;
	m_gogoBK->changeAction("gogobk");
	m_gogoBK->play();
	m_note.push_back(Animation2DManager::getSingleton()->get("res/note.ste", false));
	for (int i = 0; i < (int)(sizeof(m_bk)/sizeof(m_bk[0])); i++)
		m_imageRoot.addChild(m_bk[i]->getNode());
	m_imageRoot.addChild(m_branch->getNode());
	m_imageRoot.addChild(m_gogoBK->getNode());
	m_imageRoot.addChild(m_hitFlash->getNode());
	m_imageRoot.addChild(m_hitMark->getNode());
	m_imageRoot.addChild(&m_lineRoot);
	m_imageRoot.addChild(m_soulBar);
	m_imageRoot.addChild(m_hitJudgeBk->getNode());
	m_imageRoot.addChild(&m_noteRoot);
	m_imageRoot.addChild(m_fg->getNode());
	for(int i=0; i<4; i++)
		m_imageRoot.addChild(m_hitDrum[i]->getNode());
	m_imageRoot.addChild(m_hitJudge->getNode());
	m_imageRoot.addChild(&m_comboRoot);
	m_imageRoot.addChild(&m_balloonRoot);
	begin(3, -1);
}
void TaikoSkin::release()
{
	m_imageRoot.clearChild();
	m_noteRoot.clearChild();
	m_comboRoot.clearChild();
	m_balloonRoot.clearChild();
	for (int i = 0; i < (int)(sizeof(m_bk) / sizeof(m_bk[0])); i++)
	{
		if (m_bk[i] != NULL)
		{
			Animation2DManager::getSingleton()->release(m_bk[i], false);
			m_bk[i] = NULL;
		}
	}
	if(m_fg != NULL)
	{
		Animation2DManager::getSingleton()->release(m_fg, false);
		m_fg = NULL;
	}
	if(m_hitJudge != NULL)
	{
		Animation2DManager::getSingleton()->release(m_hitJudge, false);
		m_hitJudge = NULL;
	}
	if(m_hitJudgeBk != NULL)
	{
		Animation2DManager::getSingleton()->release(m_hitJudgeBk, false);
		m_hitJudgeBk = NULL;
	}
	if(m_hitFlash != NULL)
	{
		Animation2DManager::getSingleton()->release(m_hitFlash, false);
		m_hitFlash = NULL;
	}
	for(int i=0; i<4; i++)
	{
		if(m_hitDrum[i] != NULL)
		{
			Animation2DManager::getSingleton()->release(m_hitDrum[i], false);
			m_hitDrum[i] = NULL;
		}
	}
	if(m_hitMark != NULL)
	{
		Animation2DManager::getSingleton()->release(m_hitMark, false);
		m_hitMark = NULL;
	}
	for(int i=0; i<_countof(m_number); i++)
	{
		if(m_number[i] != NULL)
		{
			Animation2DManager::getSingleton()->release(m_number[i], false);
			m_number[i] = NULL;
		}
	}
	for(int i=0; i<_countof(m_balloonNumber); i++)
	{
		if(m_balloonNumber[i] != NULL)
		{
			Animation2DManager::getSingleton()->release(m_balloonNumber[i], false);
			m_balloonNumber[i] = NULL;
		}
	}
	for(int i=0; i<(int)m_note.size(); i++)
		Animation2DManager::getSingleton()->release(m_note[i], false);
	m_note.clear();
	m_lineRoot.deleteChildBranch(false);
	m_soulBar[0].clearChild();
	m_soulBurnTex = NULL;
	if(m_soulAni != NULL)
	{
		Animation2DManager::getSingleton()->release(m_soulAni, false);
		m_soulAni = NULL;
	}
	if(m_branch != NULL)
	{
		Animation2DManager::getSingleton()->release(m_branch, false);
		m_branch = NULL;
	}
	if(m_gogoBK != NULL)
	{
		Animation2DManager::getSingleton()->release(m_gogoBK, false);
		m_gogoBK = NULL;
	}
}
void TaikoSkin::begin(int course, int branch)
{
	m_soulAni->changeAction(course >= 3 ? "40" : "30");
	m_soulAni->play();
	if(m_soulAni->getReferencePointCount() >= 3)
	{
		for(int i=0; i<3; i++)
			m_soulAni->getReferencePoint(i, m_soulPos[i*2], m_soulPos[i*2+1]);
	}else
	{
		memset(m_soulPos, 0, sizeof(m_soulPos));
	}
	clearNote();
	setNoteCount(-1, -1);
	m_hitFlash->getNode()->setBranchVisible(false);
	setHit(false, false, false, false);
	setHitJudge(-1);
	for(int i=0; i<4; i++)
		m_hitDrum[i]->getNode()->setBranchVisible(false);
	setCombo(0);
	setSoul(0);
	setGogo(false);
	setBranch(branch);
}
void TaikoSkin::clearNote()
{
	for(int i=0; i<m_lineRoot.getChildCount(); i++)
		m_lineRoot.getChild(i)->setBranchVisible(false);
	m_noteRoot.clearChild();
	m_showNote = 0;
}
void TaikoSkin::addNote(float pos, int type)
{
	const char *action = NULL;
	switch(type)
	{
	case TaikoGame::RED:
		action = "red";
		break;
	case TaikoGame::BLUE:
		action = "blue";
		break;
	case TaikoGame::RED_BIG:
		action = "red_big";
		break;
	case TaikoGame::BLUE_BIG:
		action = "blue_big";
		break;
	case TaikoGame::BALLOON:
		action = "balloon";
		break;
	case TaikoGame::POTATO:
		action = "potato";
		break;
	default:
		return;
	}
	if((int)m_note.size() <= m_showNote)
		m_note.push_back(Animation2DManager::getSingleton()->get("res/note.ste", false));
	Animation2D *pani = m_note[m_showNote];
	pani->changeAction(action);
	pani->play();
	float localMat[6] = {1.0f,0.0f,floor(m_width*pos + 0.5f), 0.0f,1.0f,0.0f};
	pani->getNode()->setLocalMatrix(localMat);
	m_noteRoot.addChild(pani->getNode());
	m_showNote++;
}
void TaikoSkin::addBar(float left, float right, bool big)
{
	while((int)m_note.size() < m_showNote+3)
		m_note.push_back(Animation2DManager::getSingleton()->get("res/note.ste", false));
	float xleft = floor(left*m_width + 0.5f);
	float xright = floor(right*m_width + 0.5f);
	Animation2D *ptail = m_note[m_showNote];
	ptail->changeAction(big ? "yellow_big_tail" : "yellow_tail");
	ptail->play();
	float localMat[6] = {1.0f,0.0f,xright, 0.0f,1.0f,0.0f};
	ptail->getNode()->setLocalMatrix(localMat);
	m_noteRoot.addChild(ptail->getNode());
	m_showNote++;
	Animation2D *pbody = m_note[m_showNote];
	pbody->changeAction(big ? "yellow_big_body" : "yellow_body");
	pbody->play();
	float bodyMat[6] = {(xright-xleft)/32.0f, 0.0f, xleft, 0.0f,1.0f,0.0f};
	pbody->getNode()->setLocalMatrix(bodyMat);
	m_noteRoot.addChild(pbody->getNode());
	m_showNote++;
	Animation2D *phead = m_note[m_showNote];
	phead->changeAction(big ? "yellow_big" : "yellow");
	phead->play();
	localMat[2] = xleft;
	phead->getNode()->setLocalMatrix(localMat);
	m_noteRoot.addChild(phead->getNode());
	m_showNote++;
}
void TaikoSkin::addLine(float pos)
{
	Game2DImageNode *pnode = NULL;
	for(int i=0; i<m_lineRoot.getChildCount(); i++)
	{
		if((m_lineRoot.getChild(i)->getDrawFlag() & Game2DImageNode::DRAW_FLAG_HIDE) != 0)
		{
			pnode = m_lineRoot.getChild(i);
			break;
		}
	}
	if(pnode == NULL)
	{
		pnode = new Game2DImageNode();
		pnode->setDrawMode(Game2DImageNode::DRAW_MODE_SOLID);
		m_lineRoot.addChild(pnode);
	}
	pnode->setDrawFlag(0);
	pnode->setColor(1.0f, 1.0f, 1.0f, 0.5f);
	float objMat[6] = {3.0f,0.0f,-1.0f, 0.0f,154.0f,-77.0f};
	pnode->setObjectMatrix(objMat);
	float localMat[6] = {1.0f,0.0f,floor(m_width*pos+0.5f), 0.0f,1.0f,0.0f};
	pnode->setLocalMatrix(localMat);
}
void TaikoSkin::setNoteCount(int count, int type)
{
	m_balloonRoot.clearChild();
	if(count > 0)
	{
		int space = 40;
		char ts[32] = "";
		sprintf(ts, "%d", count);
		int length = strlen(ts);
		if(length > _countof(m_balloonNumber))
			length = _countof(m_balloonNumber);
		for(int i=0; i<length; i++)
		{
			char act[2] = {ts[i], '\0'};
			m_balloonNumber[i]->changeAction(act);
			m_balloonNumber[i]->play();
			m_balloonRoot.addChild(m_balloonNumber[i]->getNode());
			float lmat[6] = {1.0f,0.0f,i*space-0.5f*(length-1)*space, 0.0f,1.0f,0.0f};
			m_balloonNumber[i]->getNode()->setLocalMatrix(lmat);
		}
	}
}
void TaikoSkin::setHit(bool leftRed, bool rightRed, bool leftBlue, bool rightBlue)
{
	if(leftRed || rightRed)
	{
		m_hitFlash->getNode()->setDrawFlag(Game2DImageNode::DRAW_FLAG_HIDE);
		m_hitFlash->changeAction("red");
		m_hitFlash->play();
	}
	else if(leftBlue || rightBlue)
	{
		m_hitFlash->getNode()->setDrawFlag(Game2DImageNode::DRAW_FLAG_HIDE);
		m_hitFlash->changeAction("blue");
		m_hitFlash->play();
	}
	if(leftRed)
	{
		m_hitDrum[0]->getNode()->setDrawFlag(Game2DImageNode::DRAW_FLAG_HIDE);
		m_hitDrum[0]->play();
	}
	if(rightRed)
	{
		m_hitDrum[1]->getNode()->setDrawFlag(Game2DImageNode::DRAW_FLAG_HIDE);
		m_hitDrum[1]->play();
	}
	if(leftBlue)
	{
		m_hitDrum[2]->getNode()->setDrawFlag(Game2DImageNode::DRAW_FLAG_HIDE);
		m_hitDrum[2]->play();
	}
	if(rightBlue)
	{
		m_hitDrum[3]->getNode()->setDrawFlag(Game2DImageNode::DRAW_FLAG_HIDE);
		m_hitDrum[3]->play();
	}
}
void TaikoSkin::setHitJudge(int judge)
{
	m_hitJudge->getNode()->setDrawFlag(Game2DImageNode::DRAW_FLAG_HIDE);
	m_hitJudgeBk->getNode()->setDrawFlag(Game2DImageNode::DRAW_FLAG_HIDE);
	switch(judge)
	{
	case 0:
		m_hitJudge->changeAction("hit_0");
		m_hitJudgeBk->changeAction("hit_0");
		break;
	case 1:
		m_hitJudge->changeAction("hit_1");
		m_hitJudgeBk->changeAction("hit_1");
		break;
	case 2:
		m_hitJudge->changeAction("hit_2");
		m_hitJudgeBk->changeAction("hit_2");
		break;
	default:
		m_hitJudge->getNode()->setBranchVisible(false);
		m_hitJudgeBk->getNode()->setBranchVisible(false);
		break;
	}
	m_hitJudge->play();
	m_hitJudgeBk->play();
}
void TaikoSkin::setCombo(int combo)
{
	int space = 40;
	char ts[32] = "";
	sprintf(ts, "%d", combo);
	int length = strlen(ts);
	if(length > _countof(m_number))
		length = _countof(m_number);
	m_comboRoot.clearChild();
	for(int i=0; i<length; i++)
	{
		char act[2] = {ts[i], '\0'};
		m_number[i]->changeAction(act);
		m_number[i]->play();
		m_comboRoot.addChild(m_number[i]->getNode());
		float lmat[6] = {1.0f,0.0f,i*space-0.5f*(length-1)*space, 0.0f,1.0f,0.0f};
		m_number[i]->getNode()->setLocalMatrix(lmat);
	}
}
void TaikoSkin::setSoul(int soul)
{
	if(soul <= 50)
	{
		float len = (m_soulPos[4] - m_soulPos[0]) * soul / 50.0f;
		float leftMat[6] = {len <= m_soulPos[2] ? len : m_soulPos[2], 0.0f, m_soulPos[0], 0.0f, m_soulPos[3]-m_soulPos[1], m_soulPos[1]};
		m_soulBar[0].setObjectMatrix(leftMat);
		m_soulBar[0].setDrawFlag(soul > 0 ? 0 : Game2DImageNode::DRAW_FLAG_HIDE);
		m_soulBar[0].setDrawMode(Game2DImageNode::DRAW_MODE_SOLID);
		m_soulBar[0].setColor(0.921568627f,0.301960784f,0.337254902f,1.0f);
		if(len < m_soulPos[2])
		{
			float midMat[6] = {m_soulPos[2]-len, 0.0f, len, 0.0f, m_soulPos[3]-m_soulPos[1], m_soulPos[1]};
			m_soulBar[1].setObjectMatrix(midMat);
			m_soulBar[1].setColor(0.15f,0.15f,0.15f,1.0f);
		}
		else
		{
			float midMat[6] = {len-m_soulPos[2], 0.0f, m_soulPos[2], 0.0f, m_soulPos[5]-m_soulPos[1], m_soulPos[1]};
			m_soulBar[1].setObjectMatrix(midMat);
			m_soulBar[1].setColor(0.968627451f,0.862745098f,0.274509804f,1.0f);
		}
		m_soulBar[1].setDrawFlag(0);
		m_soulBar[1].setDrawMode(Game2DImageNode::DRAW_MODE_SOLID);
		if(soul < 50)
		{
			float rightPos = len <= m_soulPos[2] ? m_soulPos[2] : len;
			float rightMat[6] = {m_soulPos[4]-rightPos, 0.0f, rightPos, 0.0f, m_soulPos[5]-m_soulPos[1], m_soulPos[1]};
			m_soulBar[2].setObjectMatrix(rightMat);
			m_soulBar[2].setDrawFlag(0);
			m_soulBar[2].setDrawMode(Game2DImageNode::DRAW_MODE_SOLID);
			m_soulBar[2].setColor(0.15f,0.15f,0.15f,1.0f);
		}
		else
		{
			m_soulBar[2].setDrawFlag(Game2DImageNode::DRAW_FLAG_HIDE);
		}
		m_soulBar[3].setDrawFlag(Game2DImageNode::DRAW_FLAG_HIDE);
	}
	else
	{
		float pos = m_soulPos[0] + (m_soulPos[4] - m_soulPos[0]) * (clock() % CLOCKS_PER_SEC) / CLOCKS_PER_SEC;
		int inode = 0;
		for(int i=0; i<4; i++)
		{
			float left = floor((m_soulPos[4] - m_soulPos[0]) * (i - 2) / 2 + pos);
			float right = floor((m_soulPos[4] - m_soulPos[0]) * (i - 1) / 2 + pos);
			if(left >= m_soulPos[4] || right <= m_soulPos[0])
				continue;
			float ltex = i % 2 == 0 ? m_soulBurnTexRange[0] : m_soulBurnTexRange[1];
			float rtex = i % 2 == 0 ? m_soulBurnTexRange[1] : m_soulBurnTexRange[0];
			float keypos[3] = {left, right, right};
			float keytex[3] = {ltex, rtex, rtex};
			float top[2] = {left < m_soulPos[2] ? m_soulPos[3] : m_soulPos[5], m_soulPos[5]};
			int nseg = 1;
			if(left < m_soulPos[0])
			{
				keypos[0] = m_soulPos[0];
				keytex[0] = ltex + (rtex - ltex) * (m_soulPos[0] - left) / (right - left);
			}
			if(right > m_soulPos[4])
			{
				keypos[1] = keypos[2] = m_soulPos[4];
				keytex[1] = keytex[2] = ltex + (rtex - ltex) * (m_soulPos[4] - left) / (right - left);
			}
			if(left < m_soulPos[2] && right > m_soulPos[2])
			{
				keypos[1] = m_soulPos[2];
				keytex[1] = ltex + (rtex - ltex) * (m_soulPos[2] - left) / (right - left);
				nseg = 2;
			}
			for(int j=0; j<nseg; j++)
			{
				float objMat[6] = {keypos[j+1]-keypos[j], 0.0f, keypos[j], 0.0f, top[j] - m_soulPos[1], m_soulPos[1]};
				m_soulBar[inode].setObjectMatrix(objMat);
				m_soulBar[inode].setDrawFlag(0);
				m_soulBar[inode].setDrawMode(Game2DImageNode::DRAW_MODE_TEXTURE);
				m_soulBar[inode].setColor(1.0f, 1.0f, 1.0f, 1.0f);
				m_soulBar[inode].setTextureResource(m_soulBurnTex);
				m_soulBar[inode].setTexcoordRange(keytex[j], keytex[j+1], m_soulBurnTexRange[2], m_soulBurnTexRange[3]);
				inode++;
			}
		}
		for(; inode < 4; inode++)
			m_soulBar[inode].setDrawFlag(Game2DImageNode::DRAW_FLAG_HIDE);
	}
}
void TaikoSkin::setGogo(bool gogo)
{
	m_gogoBK->getNode()->setBranchVisible(gogo);
	const char *action = gogo ? "gogo" : "mark";
	if(strcmp(m_hitMark->getCurrentAction(), action) != 0)
		m_hitMark->changeAction(action);
}
void TaikoSkin::setBranch(int branch)
{
	if(branch >= 0)
	{
		char action[2] = {'0'+branch, '\0'};;
		m_branch->changeAction(action);
		m_branch->getNode()->setBranchVisible(true);
		m_branch->play();
	}
	else
	{
		m_branch->getNode()->setBranchVisible(false);
	}
}
int TaikoSkin::getHitType(float x, float y, bool *pleft)
{
	if(pleft != NULL)
		*pleft = x < m_hitCenter[0];
	float dx = (x-m_hitCenter[0]) * m_hitScale[0];
	float dy = (y-m_hitCenter[1]) * m_hitScale[1];
	return dx*dx+dy*dy < 1.0f ? TaikoGame::RED_BIG : TaikoGame::BLUE_BIG;
}

void TaikoSkin::setDrumScale(float scale)
{
	if (m_bk[0] != NULL && m_bk[0]->getReferencePointCount() >= 4 && m_bk[1] != NULL)
	{
		float mat[6];
		matrix2DScalePosition(mat, scale, scale, m_hitCenter[0] * (1 - scale), m_hitCenter[1] * (1 - scale));
		m_bk[1]->getNode()->setLocalMatrix(mat);
		float hitRange[4];
		m_bk[0]->getReferencePoint(1, hitRange[0], hitRange[1]);
		m_bk[0]->getReferencePoint(2, hitRange[2], hitRange[3]);
		m_hitScale[0] = 1.0f / (hitRange[2] - hitRange[0]) / scale;
		m_hitScale[1] = 1.0f / (hitRange[1] - hitRange[3]) / scale;
	}
}
