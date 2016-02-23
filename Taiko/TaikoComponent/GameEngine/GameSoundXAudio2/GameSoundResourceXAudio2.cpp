#include "GameSoundResourceXAudio2.h"
#include <GameSound/GameSoundLoader.h>
#include <assert.h>

GameSoundResourceXAudio2::GameSoundResourceXAudio2(const char *name)
	: m_name(name)
{
	m_data = NULL;
	m_dataSize = 0;
}

GameSoundResourceXAudio2::~GameSoundResourceXAudio2()
{
	delete[] m_data;
	m_data = NULL;
}

bool GameSoundResourceXAudio2::load(const char *name)
{
	GameSoundLoader loader(name);
	if(loader.getData() == NULL)
		return false;
	if((loader.getNumChannel() != 1 && loader.getNumChannel() != 2) || (loader.getSampleBits() != 8 && loader.getSampleBits() != 16))
		return false;
	assert(m_data == NULL);
	m_format.wFormatTag = WAVE_FORMAT_PCM;
	m_format.nChannels = loader.getNumChannel();
	m_format.nSamplesPerSec = loader.getFreq();
	m_format.nAvgBytesPerSec = loader.getSampleBits() * loader.getNumChannel() * loader.getFreq() / 8;
	m_format.nBlockAlign = loader.getSampleBits() * loader.getNumChannel() / 8;
	m_format.wBitsPerSample = loader.getSampleBits();
	m_format.cbSize = 0;
	m_data = new char[loader.getDataSize()];
	memcpy(m_data, loader.getData(), loader.getDataSize());
	m_dataSize = loader.getDataSize();
	return true;
}