#ifndef GAMEBGM_H
#define GAMEBGM_H

namespace GameBGM
{
	void release();
	bool load(const wchar_t *path, double time);
	void play(bool loop);
	void pause();
	void seek(double time);
	void setVolume(double volume);
	bool isReady();
	bool isPlaying();
	bool isError();
	double getCurrentTime();
};

#endif
