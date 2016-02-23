#ifndef GAMESOUNDXAUDIO2_H
#define GAMESOUNDXAUDIO2_H

#include "GameSoundManagerXAudio2.h"

class GameSoundXAudio2 : public GameSound
{
private:
	GameSoundResourceXAudio2 *m_res;
	IXAudio2SourceVoice *m_voice;
	int m_state;
public:
	GameSoundXAudio2(IXAudio2 *pXAudio2, GameSoundResourceXAudio2 *res);
	~GameSoundXAudio2();

	void play(int loopCount);
	void pause();
	void stop();
	void setVolume(float volume);
	int getState();
	const char *getName()
	{
		return m_res != NULL ? m_res->getName() : NULL;
	}
};

#endif
