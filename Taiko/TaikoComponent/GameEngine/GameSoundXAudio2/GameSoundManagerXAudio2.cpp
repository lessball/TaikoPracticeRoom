#include "GameSoundManagerXAudio2.h"
#include "GameSoundXAudio2.h"
#include <assert.h>

GameSoundManagerXAudio2 GameSoundManagerXAudio2::s_singleton;

GameSoundResourceXAudio2 *GameSoundManagerXAudio2::SoundFactory::create(const char *name, bool delayLoad)
{
	GameSoundResourceXAudio2 *res = new GameSoundResourceXAudio2(name);
	if(!delayLoad && !res->load(name))
	{
		delete res;
		return NULL;
	}
	return res;
}

bool GameSoundManagerXAudio2::initSoundManager()
{
	assert(m_pXAudio2 == NULL);
	if(FAILED(XAudio2Create(&m_pXAudio2)))
		return false;
	if(FAILED(m_pXAudio2->CreateMasteringVoice(&m_pMasterVoice)))
	{
		m_pXAudio2->Release();
		m_pXAudio2 = NULL;
		return false;
	}
	return true;
}
void GameSoundManagerXAudio2::releaseSoundManager()
{
	if(m_pMasterVoice != NULL)
	{
		m_pMasterVoice->DestroyVoice();
		m_pMasterVoice = NULL;
	}
	if(m_pXAudio2 != NULL)
	{
		m_pXAudio2->Release();
		m_pXAudio2 = NULL;
	}
}
GameSoundXAudio2 *GameSoundManagerXAudio2::get(const char *name, bool delayLoad)
{
	if(m_pXAudio2 == NULL)
		return NULL;
	GameSoundResourceXAudio2 *res = m_resourceManager.get(name, delayLoad);
	return res != NULL ? new GameSoundXAudio2(m_pXAudio2, res) : NULL;
}
void GameSoundManagerXAudio2::release(GameSoundXAudio2 *sound, bool delayDestroy)
{
	assert(sound != NULL);
	m_resourceManager.release(sound->getName(), delayDestroy);
	delete sound;
}

GameSoundManager *GameSoundManager::getSingleton()
{
	return GameSoundManagerXAudio2::getSingleton();
}
bool GameSoundManager::initSoundManager()
{
	return static_cast<GameSoundManagerXAudio2*>(this)->initSoundManager();
}
void GameSoundManager::releaseSoundManager()
{
	static_cast<GameSoundManagerXAudio2*>(this)->releaseSoundManager();
}
GameSound *GameSoundManager::get(const char *name, bool delayLoad)
{
	return static_cast<GameSoundManagerXAudio2*>(this)->get(name, delayLoad);
}
void GameSoundManager::release(GameSound *sound, bool delayDestroy)
{
	static_cast<GameSoundManagerXAudio2*>(this)->release(static_cast<GameSoundXAudio2*>(sound), delayDestroy);
}
float GameSoundManager::doIncrementLoad(bool stepLoad)
{
	return static_cast<GameSoundManagerXAudio2*>(this)->doIncrementLoad(stepLoad);
}
void GameSoundManager::releaseAll()
{
	static_cast<GameSoundManagerXAudio2*>(this)->releaseAll();
}
