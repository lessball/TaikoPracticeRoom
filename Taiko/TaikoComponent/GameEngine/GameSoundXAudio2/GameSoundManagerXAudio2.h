#ifndef GAMESOUNDMANAGERXAUDIO2_H
#define GAMESOUNDMANAGERXAUDIO2_H

#include <GameSound/GameSoundManager.h>
#include <ResourceManagerT.h>
#include <xaudio2.h>
#include "GameSoundResourceXAudio2.h"

class GameSoundXAudio2;
class GameSoundManagerXAudio2 : public GameSoundManager
{
private:
	class SoundFactory
	{
	public:
		GameSoundResourceXAudio2 *create(const char *name, bool delayLoad);
		void destroy(GameSoundResourceXAudio2 *res)
		{
			delete res;
		}
		bool isLoaded(GameSoundResourceXAudio2 *res)
		{
			return res->isLoaded();
		}
		void load(GameSoundResourceXAudio2 *res, const char *name)
		{
			res->load(name);
		}
	};
	ResourceManagerT<GameSoundResourceXAudio2, SoundFactory> m_resourceManager;

	IXAudio2 *m_pXAudio2;
	IXAudio2MasteringVoice *m_pMasterVoice;

	static GameSoundManagerXAudio2 s_singleton;
public:
	static GameSoundManagerXAudio2 *getSingleton()
	{
		return &s_singleton;
	}
	bool initSoundManager();
	void releaseSoundManager();

	GameSoundXAudio2 *get(const char *name, bool delayLoad);
	void release(GameSoundXAudio2 *sound, bool delayDestroy);
	float doIncrementLoad(bool stepLoad)
	{
		return m_resourceManager.doIncrementLoad(stepLoad);
	}
	void releaseAll()
	{
		m_resourceManager.releaseAll();
	}

	IXAudio2 *getIXAudio2()
	{
		return m_pXAudio2;
	}
};

#endif
