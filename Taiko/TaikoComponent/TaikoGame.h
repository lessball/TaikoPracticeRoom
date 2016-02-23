#ifndef TAIKOGAME_H
#define TAIKOGAME_H

#include "TaikoSkin.h"
#include <Game2DImage/Game2DImageRender.h>
#include <GameSound/GameSoundManager.h>
#include <string>
#include <vector>
#include <list>
#include <atomic>

class TaikoGame
{
private:
	struct Note
	{
		int type;
		float time;
		float speed;
		int showTo;
		union
		{
			int iparam;
			float fparam;
		};
	};
	std::vector<Note> m_sequence[3];
	int m_fullCombo;
	struct Balloon
	{
		int count;
		float endtime;
	};
	std::vector<Balloon> m_balloon;
	struct BranchCondation
	{
		int type;
		int limit[2];
		int index[3]; // index of BRANCH_BEGIN
	};
	std::vector<BranchCondation> m_branch;
	struct Potato
	{
		int count;
		float scoretime;
		float endtime;
	};
	std::vector<Potato> m_potato;
	int m_course;
	int m_level;
	int m_scoreinit;
	int m_scorediff;
	float m_beginTime;
	//std::wstring m_wave;

	float m_time;
	int m_playIndex;
	int m_playBranch;
	int m_nextBranch;
	int m_nextBranchIndex;
	Note *m_currentNote;
	Note *m_currentGogo;
	BranchCondation *m_currentCondation;
	int m_score;
	int m_soul;
	int m_perfect;
	int m_good;
	int m_bad;
	int m_combo;
	int m_maxCombo;
	bool m_gogo;
	int m_balloonCount;
	int m_branchCombo;
	int m_branchHit;
	int m_branchHitBase;
	int m_branchScore;
	bool m_branchLock;
	std::list<Note*> m_passNote;

	bool m_autoPlay;
	float m_autoHitTime;
	bool m_autoLeft;

	bool m_hitRed[2];
	bool m_hitRedBig;
	bool m_hitBlue[2];
	bool m_hitBlueBig;
	bool m_lastMiss;
	std::atomic_flag m_hitLock;

	GameSound *m_hitSound[2];

public:
	TaikoGame();
	enum NoteType
	{
		NONE,
		RED,
		BLUE,
		RED_BIG,
		BLUE_BIG,
		YELLOW,
		YELLOW_BIG,
		BALLOON,
		END,
		POTATO,
		GOGOSTART,
		GOGOEND,
		SECTION,
		BRANCH_CONDATION,
		BRANCH_BEGIN,
		//BRANCH_END,
		LEVEL_HOLD,
	};
	bool init();
	void release();
	bool loadtja(const char *data, int index);
	void releaseGame();
	void begin(TaikoSkin *skin, bool autoPlay);
	void update(float time, TaikoSkin *skin);
	void input(int type, bool left, TaikoSkin *skin);

	bool isPlaying();
	bool getScore(bool &trueScore, bool &success, bool &fullCombo, int &score, int &combo, int &good, int &normal, int &bad);
};

#endif
