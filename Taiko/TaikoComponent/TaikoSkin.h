#ifndef TAIKOSKIN_H
#define TAIKOSKIN_H

#include <Game2DImage/Game2DImageRender.h>
#include <Game2DAnimation/Animation2D.h>
#include <vector>

class TaikoSkin
{
private:
	Game2DImageNode m_imageRoot;
	int m_hit;
	float m_width;

	Animation2D *m_bk;
	Animation2D *m_fg;
	Animation2D *m_hitJudge;
	Animation2D *m_hitJudgeBk;
	Animation2D *m_hitFlash;
	Animation2D *m_hitDrum[4];
	Animation2D *m_hitMark;
	Animation2D *m_number[4];
	Animation2D *m_balloonNumber[4];
	Animation2D *m_soulAni;
	Animation2D *m_branch;
	Animation2D *m_gogoBK;
	Game2DImageNode m_comboRoot;
	Game2DImageNode m_noteRoot;
	Game2DImageNode m_lineRoot;
	Game2DImageNode m_balloonRoot;
	Game2DImageNode m_soulBar[4];
	float m_soulPos[6];
	TextureResource *m_soulBurnTex;
	float m_soulBurnTexRange[4];
	std::vector<Animation2D*> m_note;
	int m_showNote;

	float m_hitCenter[2];
	float m_hitScale[2];
public:
	TaikoSkin();
	~TaikoSkin();
	void init();
	void release();
	void begin(int course, int branch);
	void clearNote();
	void addNote(float pos, int type);
	void addBar(float left, float right, bool big);
	void addLine(float pos);
	void setNoteCount(int count, int type);
	void setHit(bool leftRed, bool rightRed, bool leftBlue, bool rightBlue);
	void setHitJudge(int judge);
	void setCombo(int combo);
	void setSoul(int soul);
	void setGogo(bool gogo);
	void setBranch(int branch);
	void update(double time);
	Game2DImageNode *getImageRoot()
	{
		return &m_imageRoot;
	}

	int getHitType(float x, float y, bool *pleft);
};

#endif
