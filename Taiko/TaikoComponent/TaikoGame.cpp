#include "TaikoGame.h"
#include "GameBGM.h"
#include <FileResource/FileResource.h>
#include <stdio.h>
using namespace std;

TaikoGame::TaikoGame()
	: m_time(FLT_MAX), m_currentNote(NULL), m_currentGogo(NULL), m_currentCondation(NULL)
{
	m_hitSound[0] = m_hitSound[1] = NULL;
}

bool TaikoGame::init()
{
	if(m_hitSound[0] == NULL)
		m_hitSound[0] = GameSoundManager::getSingleton()->get("dong.wav", false);
	if(m_hitSound[1] == NULL)
		m_hitSound[1] = GameSoundManager::getSingleton()->get("ka.wav", false);
	return true;
}
void TaikoGame::release()
{
	for(int i=0; i<(int)(sizeof(m_hitSound)/sizeof(m_hitSound[0])); i++)
	{
		if(m_hitSound[i] != NULL)
		{
			GameSoundManager::getSingleton()->release(m_hitSound[i], false);
			m_hitSound[i] = NULL;
		}
	}
}

bool TaikoGame::loadtja(const char *path, int index)
{
	if(path == NULL)
		return false;
	FileResource *file = FileResource::open(path);
	if(file == NULL)
		return false;
	char *data = file->readAllString();
	if(data == NULL)
	{
		FileResource::close(file);
		return false;
	}
	FileResource::close(file);
	file = NULL;

	for(int i=0; i<3; i++)
		m_sequence[i].clear();
	m_fullCombo = 0;
	m_branch.clear();
	m_potato.clear();
	m_beginTime = 0.0f;
	m_course = 3;
	m_level = 1;
	m_scoreinit = 0;
	m_scorediff = 0;

	const char *pdata = data;
	const char *pwave = NULL;
	int songvol = 100;
	int sevol = 100;
	float bpm = -1.0f;
	float offset = 0.0f;
	const char *pballoon = NULL;
	while(*pdata != '\0')
	{
		if(strncmp(pdata, "#START", strlen("#START")) == 0)
		{
			if(--index < 0)
				break;
		}if(strncmp(pdata, "WAVE:", strlen("WAVE:")) == 0)
		{
			pwave = data;
		}else if(sscanf(pdata, "SONGVOL: %d", &songvol) > 0)
		{
		}else if(sscanf(pdata, "SEVOL: %d", &sevol) > 0)
		{
		}else if(sscanf(pdata, "BPM: %g", &bpm) > 0)
		{
		}else if(sscanf(pdata, "OFFSET: %g", &offset) > 0)
		{
		}else if(strncmp(pdata, "COURSE:", strlen("COURSE:")) == 0)
		{
			pdata += strlen("COURSE:");
			while(*pdata == ' ')
				pdata++;
			if(sscanf(pdata, " %d", &m_course) > 0)
			{
				m_course = max(0, min(3, m_course));
			}
			else
			{
				const char *scourse[] = {"easy", "normal", "hard", "oni", "edit"};
				for(int i=0; i<(int)(sizeof(scourse)/sizeof(scourse[0])); i++)
				{
					bool bequal = true;
					for(int j=0; scourse[i][j] != '\0' && true; j++)
					{
						if(scourse[i][j] != pdata[j] && scourse[i][j]-'a'+'A' != pdata[j])
							bequal = false;
					}
					if(bequal)
					{
						m_course = min(3, i);
						break;
					}
				}
			}
		}else if(sscanf(pdata, "LEVEL: %d", &m_level))
		{
			m_level = min(max(m_level, 1), 10);
		}else if(sscanf(pdata, "SCOREINIT: %d", &m_scoreinit) > 0)
		{
		}else if(sscanf(pdata, "SCOREDIFF: %d", &m_scorediff) > 0)
		{
		}else if(strncmp(pdata, "BALLOON:" , strlen("BALLOON:")) == 0)
		{
			pballoon = pdata + strlen("BALLOON:");
		}
		while(*pdata != '\r' && *pdata != '\n' && *pdata != '\0')
			pdata++;
		while(*pdata == '\r' || *pdata == '\n')
			pdata++;
	}
	if(pwave == NULL || bpm <= 0.0f || index >= 0)
	{
		delete[] data;
		return false;
	}
	vector<int> balloon;
	int nballon;
	while(pballoon != NULL && sscanf(pballoon, " %d", &nballon) > 0)
	{
		balloon.push_back(nballon);
		while(*pballoon != ','  && *pballoon != '\r' && *pballoon != '\n' && *pballoon != '\0')
			pballoon++;
		if(*pballoon != ',')
			break;
		pballoon++;
	}

	// auto score calculate
	int scoreMeasure[2][11];
	memset(scoreMeasure, 0, sizeof(scoreMeasure));
	int measureBranch = 2;
	int otherScore = 0;
	bool scoregogo = false;
	
	float measure = 1.0f;
	float branchMeasure = 1.0f;
	float scroll = 1.0f;
	float branchScroll = 1.0f;
	int iballoon = 0;
	int linenum = 0;
	float sectiontime = 0;
	int branch = -1;
	float notetime[3] = {-offset, -offset, -offset};
	int notebegin[3] = {-1, -1, -1};
	int lastLineIndex[2][3] = {{0, 0, 0},{0, 0, 0}};
	float lastLineTime[2][3] = {{-offset,-offset,-offset}, {-offset,-offset,-offset}};
	auto breakLine = [&]()
	{
		linenum = 0;
		sectiontime = 0;
		int brbegin = branch < 0 ? 0 : branch;
		int brend = branch < 0 ? 3 : branch+1;
		for(int ib=brbegin; ib<brend; ib++)
		{
			lastLineIndex[0][ib] = lastLineIndex[1][ib];
			lastLineIndex[1][ib] = m_sequence[ib].size();
			lastLineTime[0][ib] = lastLineTime[1][ib];
			lastLineTime[1][ib] = notetime[ib];
		}
	};
	while(*pdata != '\0')
	{
		while(*pdata == '\r' || *pdata == '\n')
			pdata++;
		if(pdata == '\0')
			break;
		int nline = 0;
		while(pdata[nline] != '\r' && pdata[nline] != '\n' && pdata[nline] != '\0')
			nline++;
		if(pdata[0] == '/' && pdata[1] == '/')
		{
		}else if(*pdata == '#')
		{
			struct CmdParser
			{
				const char *data;
				int length;
				const char *value;
				bool operator() (const char *key)
				{
					value = NULL;
					int keylen = strlen(key);
					if(length >= keylen && strncmp(data, key, keylen) == 0)
						value = data + keylen;
					return value != NULL;
				}
			};
			CmdParser cmd = {pdata+1, nline-1, NULL};
			float tfa, tfb;
			char tca;
			int tia, tib;
			if(strncmp(pdata+1, "END", strlen("END")) == 0)
			{
				pdata = NULL;
			}
			else if(sscanf(pdata+1, "MEASURE %g/%g", &tfa, &tfb) == 2)
			{
				if(tfa > 0.0f && tfb > 0.0f)
					measure = tfa/tfb;
			}
			else if(sscanf(pdata+1, "BPMCHANGE %g", &bpm) == 1)
			{
			}
			else if(strncmp(pdata+1, "GOGOSTART", strlen("GOGOSTART")) == 0)
			{
				Note tnote = {GOGOSTART, 0.0f, 0.0f, 0, 0};
				int bbegin = branch < 0 ? 0 : branch;
				int bend = branch < 0 ? 3 : branch+1;
				for(int ib=bbegin; ib<bend; ib++)
				{
					tnote.time = notetime[ib];
					tnote.showTo = (int)m_sequence[ib].size();
					m_sequence[ib].push_back(tnote);
					if(ib == 2)
						scoregogo = true;
				}
			}
			else if(strncmp(pdata+1, "GOGOEND", strlen("GOGOEND")) == 0)
			{
				Note tnote = {GOGOEND, 0.0f, 0.0f, 0, 0};
				int bbegin = branch < 0 ? 0 : branch;
				int bend = branch < 0 ? 3 : branch+1;
				for(int ib=bbegin; ib<bend; ib++)
				{
					tnote.time = notetime[ib];
					tnote.showTo = (int)m_sequence[ib].size();
					m_sequence[ib].push_back(tnote);
					if(ib == 2)
						scoregogo = false;
				}
			}
			else if(sscanf(pdata+1, "SCROLL %g", &scroll) == 1)
			{
			}
			else if(sscanf(pdata+1, "DELAY %g", &tfa) == 1)
			{
				int bbegin = branch < 0 ? 0 : branch;
				int bend = branch < 0 ? 3 : branch+1;
				for(int ib=bbegin; ib<bend; ib++)
					notetime[ib] += tfa;
			}
			else if(strncmp(pdata+1, "SECTION", strlen("SECTION")) == 0)
			{
				Note tnote = {SECTION, 0.0f, 0.0f, 0, 0};
				for(int ib=0; ib<3; ib++)
				{
					tnote.time = notetime[ib];
					tnote.showTo = (int)m_sequence[ib].size();
					m_sequence[ib].push_back(tnote);
				}
			}
			else if(sscanf(pdata+1, "BRANCHSTART %c, %d, %d", &tca, &tia, &tib) == 3 && (tca == 'r' || tca == 'p' || tca == 's' || tca == 'R' || tca == 'P' || tca == 'S'))
			{
				if(sectiontime > 0)
					breakLine();
				if(branch >= 0)
				{
					float ttime = max(max(notetime[0], notetime[1]), notetime[2]);
					for(int ib=0; ib<3; ib++)
						notetime[ib] = ttime;
					branch = -1;
				}
				branchMeasure = measure;
				branchScroll = scroll;
				BranchCondation bc;
				bc.type = (tca >= 'A' && tca <= 'Z') ? (tca - 'A' + 'a') : tca;
				bc.limit[0] = tia;
				bc.limit[1] = tib;
				for(int ib=0; ib<3; ib++)
				{
					Note tnote = {BRANCH_CONDATION, lastLineTime[0][ib], 0.0f, 0, m_branch.size()};
					if(lastLineIndex[0][ib] >= 0)
					{
						if(lastLineIndex[0][ib] < (int)m_sequence[ib].size())
							tnote.showTo = m_sequence[ib][lastLineIndex[0][ib]].showTo;
						m_sequence[ib].insert(m_sequence[ib].begin()+lastLineIndex[0][ib], tnote);
						for(int in = (int)m_sequence[ib].size()-1; in >= 0 && m_sequence[ib][in].showTo >= lastLineIndex[0][ib] && m_sequence[ib][in].type != BRANCH_BEGIN; in--)
							m_sequence[ib][in].showTo++;
					}
					else
					{
						m_sequence[ib].push_back(tnote);
					}
					lastLineIndex[0][ib] = -1;
				}
				for(int ib=0; ib<3; ib++)
				{
					bc.index[ib] = m_sequence[ib].size();
					Note tnote = {BRANCH_BEGIN, notetime[ib], 0.0f, m_sequence[ib].size()-1, m_branch.size()};
					m_sequence[ib].push_back(tnote);
					lastLineIndex[1][ib] = m_sequence[ib].size();
				}
				m_branch.push_back(bc);
				measureBranch = 2;
				if(bc.type == 'p')
				{
					if(bc.limit[0] > 100 && bc.limit[1] > 100)
					{
						measureBranch = 0;
					}
					else if(bc.limit[1] > 100)
					{
						measureBranch = 1;
					}
				}
			}
			else if(strncmp(pdata+1, "BRANCHEND", strlen("BRANCHEND")) == 0)
			{
				//Note tnote = {BRANCH_END, 0.0f, 0.0f, 0, 0};
				//float ttime = max(max(notetime[0], notetime[1]), notetime[2]);
				//for(int ib=0; ib<3; ib++)
				//{
				//	notetime[ib] = ttime;
				//	tnote.time = ttime;
				//	tnote.showTo = (int)m_sequence[ib].size();
				//	m_sequence[ib].push_back(tnote);
				//}
				if(sectiontime > 0)
					breakLine();
				float ttime = max(max(notetime[0], notetime[1]), notetime[2]);
				for(int ib=0; ib<3; ib++)
					notetime[ib] = ttime;
				branch = -1;
				measureBranch = 2;
			}
			else if(strncmp(pdata+1, "LEVELHOLD", strlen("LEVELHOLD")) == 0)
			{
				Note tnote = {LEVEL_HOLD, 0.0f, 0.0f, 0, 0};
				int bbegin = branch < 0 ? 0 : branch;
				int bend = branch < 0 ? 3 : branch+1;
				for(int ib=bbegin; ib<bend; ib++)
				{
					tnote.time = notetime[ib];
					tnote.showTo = (int)m_sequence[ib].size();
					m_sequence[ib].push_back(tnote);
				}
			}
			else if(pdata[1] == 'N')
			{
				if(sectiontime > 0)
					breakLine();
				measure = branchMeasure;
				scroll = branchScroll;
				branch = 0;
			}
			else if(pdata[1] == 'E')
			{
				if(sectiontime > 0)
					breakLine();
				measure = branchMeasure;
				scroll = branchScroll;
				branch = 1;
			}
			else if(pdata[1] == 'M')
			{
				if(sectiontime > 0)
					breakLine();
				measure = branchMeasure;
				scroll = branchScroll;
				branch = 2;
			}
		}else
		{
			if(linenum == 0)
			{
				for(int i=0; pdata[i] != ',' && pdata[i] != '\0'; i++)
				{
					if(pdata[i] == '#')
					{
						if(strncmp(pdata+i, "#BRANCH", strlen("#BRANCH")) == 0
							|| pdata[i+1] == 'N'
							|| pdata[i+1] == 'E'
							|| pdata[i+1] == 'M')
						{
							break;
						}
					}
					if(pdata[i] >= '0' && pdata[i] <= '9')
					{
						linenum++;
					}else if(pdata[i] == '#' || (pdata[i] == '/' && pdata[i+1] == '/'))
					{
						while(pdata[i] != '\r' && pdata[i] != '\n')
							i++;
					}
				}
			}
			float steptime = 60.0f/bpm*measure*4.0f/max(linenum, 1);
			float speed = scroll*bpm/240.0f;
			const char *pnode = pdata;
			for(int i=0; i<nline; i++)
			{
				int brbegin = branch < 0 ? 0 : branch;
				int brend = branch < 0 ? 3 : branch+1;
				if((pdata[i] >= '0' && pdata[i] <= '9') || (linenum <= 0 && pdata[i] == ','))
				{
					bool bsecline = fabs(fmod(sectiontime, 240.0f/bpm)) < 0.01f;
					sectiontime += steptime;
					int noteparam = bsecline ? 1 : 0;
					int notetype = linenum > 0 ? pdata[i] - '0' : NONE;
					if(notebegin[brbegin] < 0 || notetype != m_sequence[brbegin][notebegin[brbegin]].type)
					{
						if(notetype == BALLOON)
						{
							Balloon tballoon = {iballoon < (int)balloon.size() ? balloon[iballoon++] : 5, -1.0f};
							m_balloon.push_back(tballoon);
							noteparam = m_balloon.size() - 1;
							if(brend == 3)
								otherScore += scoregogo ? (tballoon.count * 360 + 6000) : (tballoon.count * 300 + 5000);
						}else if(notetype == POTATO)
						{
							Potato tpotato = {iballoon < (int)balloon.size() ? balloon[iballoon++] : 5, -1.0f, -1.0f};
							m_potato.push_back(tpotato);
							noteparam = m_potato.size() - 1;
							if(brend == 3)
								otherScore += scoregogo ? (tpotato.count * 360 + 6000) : (tpotato.count * 300 + 5000);
						}
					}
					for(int ib=brbegin; ib<brend; ib++)
					{
						if(bsecline && (notetype == NONE || notetype == YELLOW || notetype == YELLOW_BIG || notetype == BALLOON || notetype == END || notetype == POTATO))
						{
							// insert empty section line
							Note tnote = {NONE, notetime[ib], speed, (int)m_sequence[ib].size(), 1};
							float begintime = tnote.time - 1.2f/tnote.speed;
							for(int in = m_sequence[ib].size()-1; in>=0 && m_sequence[ib][in].time > begintime && m_sequence[ib][in].type != BRANCH_BEGIN; in--)
								m_sequence[ib][in].showTo = (int)m_sequence[ib].size();
							m_sequence[ib].push_back(tnote);
						}
						if(notetype == NONE)
						{
							notetime[ib] += steptime;
							continue;
						}
						bool bend = false;
						if(notetype == END)
						{
							if(notebegin[ib] >= 0)
								bend = true;
						}else
						{
							if(notebegin[ib] >= 0)
							{
								if(notetype != m_sequence[ib][notebegin[ib]].type)
								{
									bend = true;
								}else
								{
									if(notetype == POTATO)
										m_potato[m_sequence[ib][notebegin[ib]].iparam].scoretime = notetime[ib];
									notetime[ib] += steptime;
									continue;
								}
							}
						}
						if(bend)
						{
							Note &beginNote = m_sequence[ib][notebegin[ib]];
							switch(beginNote.type)
							{
							case YELLOW:
								beginNote.fparam = notetime[ib];
								if(ib == 2)
									otherScore += (scoregogo ? 360 : 300) * (int)((beginNote.fparam - beginNote.time) * 12);
								break;
							case YELLOW_BIG:
								beginNote.fparam = notetime[ib];
								if(ib == 2)
									otherScore += (scoregogo ? 430 : 360) * (int)((beginNote.fparam - beginNote.time) * 12);
								break;
							case BALLOON:
								m_balloon[beginNote.iparam].endtime = notetime[ib];
								break;
							case POTATO:
								m_potato[beginNote.iparam].endtime = notetime[ib];
								if(m_potato[beginNote.iparam].scoretime < 0.0f)
									m_potato[beginNote.iparam].scoretime = beginNote.time*0.4f + notetime[ib]*0.6f;
								break;
							}
							notebegin[ib] = -1;
						}
						if(notetype != END)
						{
							if(ib == measureBranch && (notetype == RED || notetype == BLUE || notetype == RED_BIG || notetype <= BLUE_BIG))
							{
								m_fullCombo++;
								scoreMeasure[scoregogo ? 1 : 0][min(m_fullCombo/10, 10)] += (notetype == RED_BIG || notetype == BLUE_BIG ? 2 : 1);
							}
							if(bend || notetype == YELLOW || notetype == YELLOW_BIG  || notetype == BALLOON || notetype == POTATO)
								notebegin[ib] = m_sequence[ib].size();
							Note tnote = {notetype, notetime[ib], speed, (int)m_sequence[ib].size(), noteparam};
							float begintime = tnote.time - 1.2f/tnote.speed;
							for(int in = m_sequence[ib].size()-1; in>=0 && m_sequence[ib][in].time > begintime && m_sequence[ib][in].type != BRANCH_BEGIN; in--)
								m_sequence[ib][in].showTo = (int)m_sequence[ib].size();
							m_sequence[ib].push_back(tnote);
							if(begintime < m_beginTime)
								m_beginTime = begintime;
						}
						notetime[ib] += steptime;
					}
				}
				if(pdata[i] == ',')
				{
					breakLine();
				}else if(pdata[i] == '/' && pdata[i+1] == '/')
				{
					break;
				}
			}
		}
		if(pdata == NULL)
			break;
		pdata += nline;
	}
	delete[] data;
	data = NULL;

	if(m_scoreinit <= 0)
	{
		int targetScore = 0;
		switch(m_course)
		{
		case 0:
			targetScore = 280000 + 20000 * m_level;
			break;
		case 1:
			targetScore = 350000 + 50000 * m_level;
			break;
		case 2:
			targetScore = 500000 + 50000 * m_level;
			break;
		case 3:
			targetScore = m_level == 10 ? 1200000 : (650000 + 50000 * m_level);
			break;
		}
		float kdiff = 0.0f;
		for(int i=0; i<11; i++)
		{
			kdiff += (4 + i) * scoreMeasure[0][i];
			kdiff += 1.2f*((4 + i)  * scoreMeasure[1][i]);
		}
		m_scorediff = (int)((targetScore - otherScore) / kdiff) / 10 * 10;
		m_scoreinit = 4*m_scorediff;
		while(true)
		{
			int tscore = 0;
			for(int i=0; i<11; i++)
			{
				int base = m_scoreinit + i * m_scorediff;
				tscore += scoreMeasure[0][i] * base;
				tscore += scoreMeasure[1][i] * ((base + base/5) / 10 * 10);
			}
			if(tscore + otherScore > targetScore)
				break;
			m_scoreinit += max(10, (targetScore - tscore - otherScore)/m_fullCombo/12*10);
			if(m_scoreinit > 4*m_scorediff + 80)
			{
				m_scorediff += 10;
				m_scoreinit = m_scorediff * 3 + 10;
			}
		}
	}

	GameBGM::setVolume(songvol * 0.01);
	for(int i=0; i<(int)(sizeof(m_hitSound)/sizeof(m_hitSound[0])); i++)
	{
		if(m_hitSound[i] != NULL)
			m_hitSound[i]->setVolume(sevol * 0.01f);
	}
	m_time = FLT_MAX;
	return true;
}

void TaikoGame::releaseGame()
{
	GameBGM::pause();

	m_currentNote = NULL;
	m_currentGogo = NULL;
	m_currentCondation = NULL;
	for(int i=0; i<3; i++)
		m_sequence[i].clear();
	m_fullCombo = 0;
	m_branch.clear();
	m_potato.clear();
	m_passNote.clear();

	m_time = FLT_MAX;
}

void TaikoGame::begin(TaikoSkin *skin, bool autoPlay)
{
	m_autoPlay = autoPlay;
	m_autoHitTime = m_beginTime - 1.0f;
	m_autoLeft = true;
	m_time = m_beginTime;
	m_playIndex = 0;
	m_playBranch = 0;
	m_nextBranch = 0;
	m_nextBranchIndex = m_branch.size() > 0 ? m_branch[0].index[0] : 0;
	m_currentNote = NULL;
	m_currentGogo = NULL;
	m_currentCondation = NULL;
	m_score = 0;
	m_soul = 0;
	m_perfect = 0;
	m_good = 0;
	m_bad = 0;
	m_combo = 0;
	m_maxCombo = 0;
	m_gogo = false;
	m_balloonCount = -1;
	m_branchCombo = 0;
	m_branchHit = 0;
	m_branchHitBase = 0;
	m_branchScore = 0;
	m_branchLock = false;
	m_hitRed[0] = m_hitRed[1] = false;
	m_hitRedBig = false;
	m_hitBlue[0] = m_hitBlue[1] = false;
	m_hitBlueBig = false;
	m_lastMiss = false;

	skin->begin(m_course, m_branch.size() > 0 ? 0 : -1);
	if(m_time > -0.001f)
		GameBGM::play(false);
}

void TaikoGame::update(float time, TaikoSkin *skin)
{
	skin->clearNote();
	bool playing = isPlaying();
	if(playing)
	{
		if(!GameBGM::isReady())
			return;
		if(m_time < 0.001f && m_time + time > -0.001f && !GameBGM::isPlaying())
			GameBGM::play(false);
	}

	//input
	while(m_hitLock.test_and_set());
	bool hitRed[2] = {m_hitRed[0], m_hitRed[1]};
	bool hitRedBig = m_hitRedBig;
	bool hitBlue[2] = {m_hitBlue[0], m_hitBlue[1]};
	bool hitBlueBig = m_hitBlueBig;
	m_hitRed[0] = m_hitRed[1] = false;
	m_hitRedBig = false;
	m_hitBlue[0] = m_hitBlue[1] = false;
	m_hitBlueBig = false;
	m_hitLock.clear();

	//autoplay
	if(m_autoPlay)
	{
		if(m_currentNote != NULL)
		{
			float step = 1/12.0f;
			switch(m_currentNote->type)
			{
			case YELLOW:
				break;
			case YELLOW_BIG:
				hitRedBig = true;
				break;
			case BALLOON:
				step = min(step, (m_balloon[m_currentNote->iparam].endtime-m_autoHitTime - 0.03f) / m_balloonCount);
				break;
			case POTATO:
				step = min(step, (m_potato[m_currentNote->iparam].endtime-m_autoHitTime - 0.03f) / m_balloonCount);
				break;
			}
			if(m_time - m_autoHitTime > step)
			{
				hitRed[m_autoLeft ? 0 : 1] = true;
				m_autoLeft = !m_autoLeft;
				if(m_hitSound[0] != NULL)
					m_hitSound[0]->play(1);
				if(step > 0.0f)
				{
					if(m_autoHitTime + 2*step < m_time)
					{
						m_autoHitTime = m_time;
					}else
					{
						m_autoHitTime += step;
					}
				}
			}
		}else if(m_playIndex < (int)m_sequence[m_playBranch].size())
		{
			Note &tnote = m_sequence[m_playBranch][m_playIndex];
			if(fabs(m_time - tnote.time) < 0.02f)
			{
				hitRedBig = hitBlueBig = tnote.type == RED_BIG || tnote.type == BLUE_BIG;
				switch(tnote.type)
				{
				case RED:
				case RED_BIG:
					hitRed[m_autoLeft ? 0 : 1] = true;
					m_autoLeft = !m_autoLeft;
					hitRedBig = true;
					if(m_hitSound[0] != NULL)
						m_hitSound[0]->play(1);
					break;
				case BLUE:
				case BLUE_BIG:
					hitBlue[m_autoLeft ? 0 : 1] = true;
					m_autoLeft = !m_autoLeft;
					hitBlueBig = true;
					if(m_hitSound[1] != NULL)
						m_hitSound[1]->play(1);
					break;
				}
			}
		}
	}

	//hit judge
	if(m_lastMiss)
	{
		skin->setHitJudge(2);
		m_lastMiss = false;
	}
	bool redAny = hitRed[0] || hitRed[1];
	bool blueAny = hitBlue[0] || hitBlue[1];
	if(m_currentNote != NULL)
	{
		switch(m_currentNote->type)
		{
		case YELLOW:
		case YELLOW_BIG:
			if(m_time > m_currentNote->fparam)
			{
				m_currentNote = NULL;
				m_balloonCount = -1;
			}else if(redAny || blueAny)
			{
				int nscore = m_currentNote->type == YELLOW ? (m_gogo ? 300 : 360) : (m_gogo ? 360 : 430);
				m_score += nscore;
				m_branchScore += nscore;
				m_branchCombo++;
				m_balloonCount++;
				if(m_currentCondation != NULL && m_currentCondation->type == 'r')
				{
					m_nextBranch = m_branchCombo >= m_currentCondation->limit[1] ? 2 : (m_branchCombo >= m_currentCondation->limit[0] ? 1 : 0);
					m_nextBranchIndex = m_currentCondation->index[m_nextBranch];
					if(m_nextBranch != m_playBranch)
						skin->setBranch(m_nextBranch);
				}
			}
			break;
		case BALLOON:
			if(m_time > m_balloon[m_currentNote->iparam].endtime)
			{
				m_balloonCount = -1;
				m_currentNote = NULL;
			}else if(redAny && m_balloonCount > 0)
			{
				m_balloonCount--;
				if(m_balloonCount > 0)
				{
					int nscore = m_gogo ? 300 : 360;
					m_score += nscore;
					m_branchScore += nscore;
				}else
				{
					int nscore = m_gogo ? 5000 : 6000;
					m_score += nscore;
					m_branchScore += nscore;
					for(auto i = m_passNote.begin(); i != m_passNote.end(); ++i)
					{
						if(*i == m_currentNote)
						{
							m_passNote.erase(i);
							break;
						}
					}
					m_balloonCount = -1;
					m_currentNote = NULL;
				}
			}
			break;
		case POTATO:
			if(m_time > m_potato[m_currentNote->iparam].endtime)
			{
				m_balloonCount = -1;
				m_currentNote = NULL;
			}else if(redAny && m_balloonCount > 0)
			{
				m_balloonCount--;
				if(m_balloonCount > 0)
				{
					int nscore = m_gogo ? 300 : 360;
					m_score += nscore;
					m_branchScore += nscore;
				}else
				{
					int nscore = m_time <= m_potato[m_currentNote->iparam].scoretime ? (m_gogo ? 5000 : 6000 ) : (m_gogo ? 1000 : 1200);
					m_score += nscore;
					m_branchScore += nscore;
					for(auto i = m_passNote.begin(); i != m_passNote.end(); ++i)
					{
						if(*i == m_currentNote)
						{
							m_passNote.erase(i);
							break;
						}
					}
					m_balloonCount = -1;
					m_currentNote = NULL;
				}
			}
			break;
		}
	}else if((redAny || blueAny) && m_playIndex < (int)m_sequence[m_playBranch].size())
	{
		const float keytime[3] = {0.05f*0.5f, 0.15f*0.5f, 0.217f*0.5f};
		Note &tnote = m_sequence[m_playBranch][m_playIndex];
		switch(tnote.type)
		{
		case RED:
		case BLUE:
		case RED_BIG:
		case BLUE_BIG:
			{
				float dtime = fabs(m_time - tnote.time);
				if(dtime <= keytime[2])
				{
					int judge = dtime <= keytime[0] ? 0 : (dtime <= keytime[1] ?  1 : 2);
					if(tnote.type == RED || tnote.type == RED_BIG)
					{
						if(!redAny)
						{
							judge = 2;
						}else if(blueAny)
						{
							judge = 1;
						}
					}else
					{
						if(!blueAny)
						{
							judge = 2;
						}else if(redAny)
						{
							judge = 1;
						}
					}
					if(judge < 2)
					{
						m_combo++;
						float score = (float)(m_scoreinit + min(m_combo/10, 10)*m_scorediff);
						if((tnote.type == RED_BIG && hitRedBig) || (tnote.type == BLUE_BIG && hitBlueBig))
							score *= 2.0f;
						if(judge > 0)
							score *= 0.5f;
						bool ingogo = (m_currentGogo == NULL || tnote.time < m_currentGogo->time) ? m_gogo : (m_currentGogo->type == GOGOSTART);
						if(ingogo)
							score *= 1.2f;
						int nscore = (int)score / 10 * 10;
						m_score += nscore;
						if(judge == 0)
						{
							m_soul += 2;
							m_perfect++;
							m_branchHit += 2;
						}else
						{
							m_soul += 1;
							m_good++;
							m_branchHit += 1;
						}
						if(m_combo > m_maxCombo)
							m_maxCombo = m_combo;
						m_branchScore += nscore;
					}else
					{
						m_combo = 0;
						m_soul -= 4;
						if(m_soul < 0)
							m_soul = 0;
						m_bad++;
					}
					m_branchHitBase += 2;
					skin->setHitJudge(judge);
					m_playIndex++;
				}
			}
			break;
		}
	}
	
	m_time += time;
	//update playindex
	while(m_playIndex < (int)m_sequence[m_playBranch].size())
	{
		Note &tnote = m_sequence[m_playBranch][m_playIndex];
		bool bcontinue = true;
		switch(tnote.type)
		{
		case NONE:
			if(m_time <= tnote.time + 0.2f/tnote.speed)
				m_passNote.push_front(&tnote);
			break;
		case RED:
		case BLUE:
		case RED_BIG:
		case BLUE_BIG:
			bcontinue = m_time > tnote.time+0.15f*0.5f;
			if(!bcontinue && m_time > tnote.time)
			{
				if(m_playIndex > 0 && m_sequence[m_playBranch][m_playIndex].type == END && m_sequence[m_playBranch][m_playIndex].time >= tnote.time-0.001f)
				{
					bcontinue = true;
				}else
				{
					float endtime = 2.0f*m_time-tnote.time;
					for(int i=m_playIndex+1; !bcontinue && i < (int)m_sequence[m_playBranch].size() && m_sequence[m_playBranch][i].time <= endtime; i++)
					{
						switch(m_sequence[m_playBranch][i].type)
						{
						case RED:
						case BLUE:
						case RED_BIG:
						case BLUE_BIG:
							bcontinue = true;
							break;
						}
					}
				}
			}
			if(bcontinue)
			{
				m_branchHitBase += 2;
				if(m_time <= tnote.time + 0.2f/tnote.speed)
					m_passNote.push_front(&tnote);
				m_lastMiss = true;
				m_combo = 0;
				m_soul -= 4;
				if(m_soul < 0)
					m_soul = 0;
				m_bad++;
			}
			break;
		case YELLOW:
		case YELLOW_BIG:
			bcontinue = m_time > tnote.time-0.05f*0.5f;
			if(bcontinue && m_time <= tnote.fparam + 0.2f/tnote.speed)
			{
				m_passNote.push_front(&tnote);
				if(m_time < tnote.fparam)
				{
					m_currentNote = &tnote;
					m_balloonCount = 0;
				}
			}
			break;
		case BALLOON:
			bcontinue = m_time > tnote.time-0.05f*0.5f;
			if(bcontinue && m_time <= m_balloon[tnote.iparam].endtime + 0.2f/tnote.speed)
			{
				m_passNote.push_front(&tnote);
				if(m_time <= m_balloon[tnote.iparam].endtime)
				{
					m_balloonCount = m_balloon[tnote.iparam].count;
					m_currentNote = &tnote;
				}
			}
			break;
		case POTATO:
			bcontinue = m_time > tnote.time-0.05f*0.5f;
			if(bcontinue && m_time <= m_potato[tnote.iparam].endtime + 0.2f/tnote.speed)
			{
				m_passNote.push_front(&tnote);
				if(m_time <= m_potato[tnote.iparam].endtime)
				{
					m_balloonCount = m_potato[tnote.iparam].count;
					m_currentNote = &tnote;
				}
			}
			break;
		case GOGOSTART:
		case GOGOEND:
			if(m_time >= tnote.time)
			{
				m_gogo = tnote.type == GOGOSTART;
				m_currentGogo = NULL;
			}
			else
			{
				bcontinue = m_currentGogo == NULL || tnote.time < m_currentGogo->time + 0.001f;
				if(bcontinue)
					m_currentGogo = &tnote;
			}
			break;
		case SECTION:
			m_branchCombo = 0;
			m_branchHit = 0;
			m_branchHitBase = 0;
			m_branchScore = 0;
			break;
		case BRANCH_CONDATION:
			if(!m_branchLock)
			{
				BranchCondation &bc = m_branch[tnote.iparam];
				int branchkey = 0;
				switch(bc.type)
				{
				case 'r':
					branchkey = m_branchCombo;
					m_currentCondation = &bc;
					break;
				case 'p':
					branchkey = m_branchHitBase > 0 ? m_branchHit*100/m_branchHitBase : 0;
					break;
				case 's':
					branchkey = m_branchScore;
					break;
				}
				m_nextBranch = branchkey >= bc.limit[1] ? 2 : (branchkey >= bc.limit[0] ? 1 : 0);
				m_nextBranchIndex = bc.index[m_nextBranch];
				if(m_nextBranch != m_playBranch)
					skin->setBranch(m_nextBranch);
			}
			else
			{
				m_nextBranchIndex = m_branch[tnote.iparam].index[m_nextBranch];
			}
			break;
		case BRANCH_BEGIN:
			bcontinue = m_currentNote == NULL;
			if(bcontinue)
			{
				m_playBranch = m_nextBranch;
				m_playIndex = m_nextBranchIndex;
				m_currentCondation = NULL;
			}
			break;
		//case BRANCH_END:
		//	if(!m_branchLock)
		//		m_playbranch = 0;
		//	break;
		case LEVEL_HOLD:
			m_branchLock = true;
			break;
		}
		if(!bcontinue)
			break;
		m_playIndex++;
	}
	if(m_currentGogo != NULL && m_time > m_currentGogo->time)
	{
		m_gogo = m_currentGogo->type == GOGOSTART;
		m_currentGogo = NULL;
	}
	
	//update display
	skin->setHit(hitRed[0], hitRed[1], hitBlue[0], hitBlue[1]);
	if(m_fullCombo > 0)
		skin->setSoul((m_course >= 3 && m_level >= 9) ? m_soul*50*4/(m_fullCombo*2*3) : m_soul*50*10/(m_fullCombo*2*7));
	skin->setCombo(m_combo);
	skin->setGogo(m_gogo);
	skin->setNoteCount(m_balloonCount, m_currentNote != NULL ? m_currentNote->type : NONE);

	int showBranch[2] = {m_playBranch, m_nextBranch};
	int showIndex[2] = {m_playIndex, m_nextBranchIndex + 1};
	int ishowbegin = (m_nextBranch == m_playBranch && m_nextBranchIndex < m_playIndex) ? 0 : 1;
	for(int i=ishowbegin; i>=0; i--)
	{
		int showEnd = -1;
		if(showIndex[i] < (int)m_sequence[showBranch[i]].size())
		{
			showEnd = m_sequence[showBranch[i]][showIndex[i]].showTo;
			int type = m_sequence[showBranch[i]][showIndex[i]].type;
			if(showIndex[i]+1 < (int)m_sequence[showBranch[i]].size() && (type == YELLOW || type == YELLOW_BIG || type == BALLOON || type == POTATO))
				showEnd = m_sequence[showBranch[i]][showIndex[i]+1].showTo;
		}
		for(int j=showEnd; j>=showIndex[i]; j--)
		{
			Note &tnote = m_sequence[showBranch[i]][j];
			float fpos = (tnote.time-m_time)*tnote.speed;
			if(fpos > 1.2f)
				continue;
			switch(tnote.type)
			{
			case NONE:
				if(tnote.iparam == 1)
					skin->addLine(fpos);
				break;
			case RED:
			case BLUE:
			case RED_BIG:
			case BLUE_BIG:
				if(tnote.iparam == 1)
					skin->addLine(fpos);
				skin->addNote(fpos, tnote.type);
				break;
			case YELLOW:
			case YELLOW_BIG:
				skin->addBar(fpos, (tnote.fparam-m_time)*tnote.speed, tnote.type == YELLOW_BIG);
				break;
			case BALLOON:
			case POTATO:
				if(m_time < tnote.time)
				{
					skin->addNote(fpos, tnote.type);
				}else
				{
					skin->addNote(0.0f, tnote.type);
				}
				break;
			}
		}
	}

	for(auto i = m_passNote.begin(); i != m_passNote.end();)
	{
		Note &tnote = **i;
		switch(tnote.type)
		{
		case NONE:
		case RED:
		case BLUE:
		case RED_BIG:
		case BLUE_BIG:
			if(m_time > tnote.time + 0.2f/tnote.speed)
			{
				m_passNote.erase(i++);
			}else
			{
				++i;
				skin->addNote((tnote.time-m_time)*tnote.speed, tnote.type);
				if(tnote.iparam == 1)
					skin->addLine((tnote.time-m_time)*tnote.speed);
			}
			break;
		case YELLOW:
		case YELLOW_BIG:
			if(m_time > tnote.fparam + 0.2f/tnote.speed)
			{
				m_passNote.erase(i++);
			}else
			{
				++i;
				skin->addBar((tnote.time-m_time)*tnote.speed, (tnote.fparam-m_time)*tnote.speed, tnote.type == YELLOW_BIG);
			}
			break;
		case BALLOON:
			if(m_time > m_balloon[tnote.iparam].endtime + 0.2f/tnote.speed)
			{
				m_passNote.erase(i++);
			}else
			{
				++i;
				skin->addNote((min(m_time, m_balloon[tnote.iparam].endtime)-m_time)*tnote.speed, tnote.type);
			}
			break;
		case POTATO:
			if(m_time > m_potato[tnote.iparam].endtime + 0.2f/tnote.speed)
			{
				m_passNote.erase(i++);
			}else
			{
				++i;
				skin->addNote((min(m_time, m_potato[tnote.iparam].endtime)-m_time)*tnote.speed, tnote.type);
			}
			break;
		}
	}
}
void TaikoGame::input(int type, bool left, TaikoSkin *skin)
{
	if(m_time > FLT_MAX/2 || m_autoPlay)
		return;
	while(m_hitLock.test_and_set());
	m_hitRed[left ? 0 : 1] |= type == RED || type == RED_BIG;
	m_hitBlue[left ? 0 : 1] |= type == BLUE || type == BLUE_BIG;
	m_hitRedBig |= type == RED_BIG;
	m_hitBlueBig |= type == BLUE_BIG;
	m_hitLock.clear();
	if(type == RED || type == RED_BIG)
	{
		if(m_hitSound[0] != NULL)
			m_hitSound[0]->play(1);
	}
	if(type == BLUE || type == BLUE_BIG)
	{
		if(m_hitSound[1] != NULL)
			m_hitSound[1]->play(1);
	}
}
bool TaikoGame::isPlaying()
{
	return m_time < FLT_MAX/2 && (m_playIndex < (int)m_sequence[m_playBranch].size() || m_currentNote != NULL || !m_passNote.empty());
}
bool TaikoGame::getScore(bool &trueScore, bool &success, bool &fullCombo, int &score, int &combo, int &good, int &normal, int &bad)
{
	if(m_time > FLT_MAX/2 || isPlaying())
		return false;
	trueScore = !m_autoPlay;
	int soul = (m_course >= 3 && m_level >= 9) ? m_soul*50*4/(m_fullCombo*2*3) : m_soul*50*10/(m_fullCombo*2*7);
	int targetsoul = m_course >= 3 ? 40 : 30;
	success = soul >= targetsoul;
	fullCombo = m_bad == 0;
	score = m_score;
	combo = m_maxCombo;
	good = m_perfect;
	normal = m_good;
	bad = m_bad;
	return true;
}
