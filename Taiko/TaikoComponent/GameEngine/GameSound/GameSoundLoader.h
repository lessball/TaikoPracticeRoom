#ifndef GAMESOUNDLOADER_H
#define GAMESOUNDLOADER_H

class GameSoundLoader
{
private:
	char *m_fileData;
	int m_numChannel;
	int m_bits;
	int m_freq;
	bool m_deletaData;
	char *m_data;
	int m_dataSize;
	void loadWav(void *data, int size);
	void loadOgg(void *data, int size);
public:
	GameSoundLoader(const char *path);
	~GameSoundLoader();

	int getNumChannel() const
	{
		return m_numChannel;
	}
	int getSampleBits() const
	{
		return m_bits;
	}
	int getFreq() const
	{
		return m_freq;
	}
	void *getData() const
	{
		return m_data;
	}
	int getDataSize() const
	{
		return m_dataSize;
	}
};

#endif
