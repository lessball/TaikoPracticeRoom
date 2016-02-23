#include "GameSoundXAudio2.h"

GameSoundXAudio2::GameSoundXAudio2(IXAudio2 *pXAudio2, GameSoundResourceXAudio2 *res)
{
	m_res = res;
	m_voice = NULL;
	if(m_res != NULL && m_res->isLoaded())
		pXAudio2->CreateSourceVoice(&m_voice, m_res->getFormat());
	m_state = STOP;
}
GameSoundXAudio2::~GameSoundXAudio2()
{
	if(m_voice != NULL)
	{
		m_voice->DestroyVoice();
		m_voice = NULL;
	}
}

void GameSoundXAudio2::play(int loopCount)
{
	if(m_res == NULL || !m_res->isLoaded())
		return;
	if(m_voice == NULL)
	{
		GameSoundManagerXAudio2::getSingleton()->getIXAudio2()->CreateSourceVoice(&m_voice, m_res->getFormat());
		if(m_voice == NULL)
			return;
	}
	if(loopCount >= 0)
	{
		XAUDIO2_BUFFER buffer;
		buffer.Flags = XAUDIO2_END_OF_STREAM;
		buffer.AudioBytes = m_res->getDataSize();
		buffer.pAudioData = (const BYTE*)m_res->getData();
		buffer.PlayBegin = 0;
		buffer.PlayLength = 0;
		buffer.LoopBegin = 0;
		buffer.LoopLength = 0;
		buffer.LoopCount = loopCount != 0 ? loopCount-1 : XAUDIO2_LOOP_INFINITE;
		buffer.pContext = NULL;
		m_voice->Stop();
		m_voice->FlushSourceBuffers();
		m_voice->SubmitSourceBuffer(&buffer);
	}
	m_voice->Start();
	m_state = PLAY;
}
void GameSoundXAudio2::pause()
{
	if(m_voice != NULL)
	{
		m_voice->Stop();
		m_state = PAUSE;
	}
}
void GameSoundXAudio2::stop()
{
	if(m_voice != NULL)
	{
		m_voice->Stop();
		m_voice->FlushSourceBuffers();
		m_state = STOP;
	}
}
void GameSoundXAudio2::setVolume(float volume)
{
	if(m_voice != NULL)
		m_voice->SetVolume(volume);
}
int GameSoundXAudio2::getState()
{
	if(m_res == NULL || !m_res->isLoaded() || m_voice == NULL)
		return NOT_LOADED;
	return m_state;
}

void GameSound::play(int loopCount)
{
	static_cast<GameSoundXAudio2*>(this)->play(loopCount);
}
void GameSound::pause()
{
	static_cast<GameSoundXAudio2*>(this)->pause();
}
void GameSound::stop()
{
	static_cast<GameSoundXAudio2*>(this)->stop();
}
void GameSound::setVolume(float volume)
{
	static_cast<GameSoundXAudio2*>(this)->setVolume(volume);
}
int GameSound::getState()
{
	return static_cast<GameSoundXAudio2*>(this)->getState();
}
const char *GameSound::getName()
{
	return static_cast<GameSoundXAudio2*>(this)->getName();
}
