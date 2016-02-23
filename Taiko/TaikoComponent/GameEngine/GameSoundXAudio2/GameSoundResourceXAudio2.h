#ifndef GAMESOUNDRESOURCEXAUDIO2_H
#define GAMESOUNDRESOURCEXAUDIO2_H

#include <xaudio2.h>
#include <string>

class GameSoundResourceXAudio2
{
private:
	WAVEFORMATEX m_format;
	char *m_data;
	int m_dataSize;
	std::string m_name;
public:
	GameSoundResourceXAudio2(const char *name);
	~GameSoundResourceXAudio2();
	const WAVEFORMATEX *getFormat() const
	{
		return &m_format;
	}
	const void *getData() const
	{
		return m_data;
	}
	int getDataSize() const
	{
		return m_dataSize;
	}

	bool load(const char *name);
	bool isLoaded()
	{
		return m_data != NULL;
	}
	const char *getName()
	{
		return m_name.c_str();
	}
};

#endif
