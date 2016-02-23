#ifndef GAMESOUNDMANAGER_H
#define GAMESOUNDMANAGER_H

class GameSound
{
public:
	/** 声音的状态 */
	enum State
	{
		NOT_LOADED,
		PLAY,
		PAUSE,
		STOP,
	};
	/**
	 * 播放
	 * loopCount 循环次数。0表示始终循环。-1表示不关心次数（继续暂停中的播放）
	 */
	void play(int loopCount);
	void pause();
	void stop();
	void setVolume(float volume);
	/**
	 * 返回状态
	 * return State枚举值
	 */
	int getState();
	/**
	 * 返回资源名称
	 */
	const char *getName();
};
class GameSoundManager
{
public:
	static GameSoundManager *getSingleton();
	bool initSoundManager();
	void releaseSoundManager();
	GameSound *get(const char *name, bool delayLoad);
	void release(GameSound *sound, bool delayDestroy);
	float doIncrementLoad(bool stepLoad);
	void releaseAll();
};

#endif
