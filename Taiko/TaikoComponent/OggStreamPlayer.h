#ifndef OGGSTREAMPLAYER_H
#define OGGSTREAMPLAYER_H

#include "../../GameSoundXAudio2/GameSoundManagerXAudio2.h"
#include <vorbis/vorbisfile.h>
#include <atomic>
#include <condition_variable>
#include <thread>
#include <list>

class OggFileReader;
class OggStreamPlayer : public IXAudio2VoiceCallback
{
private:
	std::atomic<bool> m_ready;
	bool m_play;
	bool m_loop;
	bool m_error;
	double m_timeBegin;
	float m_volume;
	OggVorbis_File m_oggFile;
	IXAudio2SourceVoice *m_voice;
	std::thread m_streamThread;
	std::mutex m_streamMutex;
	std::condition_variable m_streamCond;
	std::mutex m_cmdMutex;
	enum ECMD
	{
		SEEK,
		RELEASE,
		LOAD,
		QUIT,
	};
	struct StreamCmd
	{
		int cmd;
		double time;
		union
		{
			OggFileReader *loadFile;
			IXAudio2SourceVoice *releaseVoice;
		};
	};
	std::list<StreamCmd> m_cmdList;
	
	void CALLBACK OnStreamEnd();
	void CALLBACK OnVoiceProcessingPassEnd() {}
	void CALLBACK OnVoiceProcessingPassStart(UINT32 SamplesRequired) {}
	void CALLBACK OnBufferEnd(void * pBufferContext);
	void CALLBACK OnBufferStart(void * pBufferContext) {}
	void CALLBACK OnLoopEnd(void * pBufferContext) {}
	void CALLBACK OnVoiceError(void * pBufferContext, HRESULT Error) {}

	static size_t oggRead(void *ptr, size_t size, size_t nmemb, void *datasource);
	static int oggClose(void *datasource);
	static int oggSeek(void *datasource, ogg_int64_t offset, int whence);
	static long oggTell(void *datasource);
	static void oggStreamThread(OggStreamPlayer *player);
public:
	enum LoadResult
	{
		Ok,
		FileNotExist,
		NotOgg,
	};
	OggStreamPlayer();
	~OggStreamPlayer();
	void shutdown();
	LoadResult load(const wchar_t *path, double time);
	void release();
	void play(bool loop);
	void pause();
	void seek(double time);
	void setVolume(double volume);
	double getCurrentTime();
	bool isReady()
	{
		return m_ready;
	}
	bool isPlaying()
	{
		return m_play;
	}
	bool isError()
	{
		return m_error;
	}
};

#endif
