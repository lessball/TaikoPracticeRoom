#ifndef GAMESOUNDMANAGER_H
#define GAMESOUNDMANAGER_H

class GameSound
{
public:
	/** ������״̬ */
	enum State
	{
		NOT_LOADED,
		PLAY,
		PAUSE,
		STOP,
	};
	/**
	 * ����
	 * loopCount ѭ��������0��ʾʼ��ѭ����-1��ʾ�����Ĵ�����������ͣ�еĲ��ţ�
	 */
	void play(int loopCount);
	void pause();
	void stop();
	void setVolume(float volume);
	/**
	 * ����״̬
	 * return Stateö��ֵ
	 */
	int getState();
	/**
	 * ������Դ����
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
