#include "OggStreamPlayer.h"
#include <FileResource/FileResource.h>
#include <FileResource/MemoryFileReader.h>

#include <codecvt>
#include <string>
#include <memory>
#include <assert.h>

using namespace std;

class OggFileReader : public MemoryFileReader
{
private:
	unique_ptr<char> m_ptr;
public:
	OggFileReader(char *data, int size)
		: m_ptr(data), MemoryFileReader(data, size)
	{}
};

void CALLBACK OggStreamPlayer::OnStreamEnd()
{
	unique_lock<mutex> lock(m_streamMutex);
	if(m_voice != NULL)
	{
		m_play = m_loop;
		m_timeBegin = 0.0;
		if(m_loop)
		{
			lock.unlock();
			m_streamCond.notify_all();
		}else
		{
			m_voice->Stop();
		}
	}
}
void CALLBACK OggStreamPlayer::OnBufferEnd(void * pBufferContext)
{
	m_streamCond.notify_all();
}

size_t OggStreamPlayer::oggRead(void *ptr, size_t size, size_t nmemb, void *datasource)
{
	OggFileReader *file = (OggFileReader*)datasource;
	int readed = file->read(ptr, size*nmemb);
	if(readed % size != 0)
	{
		file->seek(-(readed % (int)size), SEEK_CUR);
	}
	return readed / size;
}
int OggStreamPlayer::oggClose(void *datasource)
{
	OggFileReader *file = (OggFileReader*)datasource;
	delete file;
	file = NULL;
	return 0;
}
int OggStreamPlayer::oggSeek(void *datasource, ogg_int64_t offset, int whence)
{
	OggFileReader *file = (OggFileReader*)datasource;
	return file->seek((int)offset, whence);
}
long OggStreamPlayer::oggTell(void *datasource)
{
	OggFileReader *file = (OggFileReader*)datasource;
	return file->tell();
}

void OggStreamPlayer::oggStreamThread(OggStreamPlayer *player)
{
	unique_lock<mutex> lock(player->m_streamMutex);
	const int bufferSize = 32768;
	const int bufferCount = 4;
	BYTE *dataBuffer[bufferCount];
	for(int i=0; i<bufferCount; i++)
		dataBuffer[i] = new BYTE[bufferSize];
	int ibuffer =  0;
	XAUDIO2_BUFFER buffer;
	buffer.Flags = 0;
	buffer.AudioBytes = 0;
	buffer.pAudioData = dataBuffer[ibuffer];
	buffer.PlayBegin = 0;
	buffer.PlayLength = 0;
	buffer.LoopBegin = 0;
	buffer.LoopLength = 0;
	buffer.LoopCount = 0;
	buffer.pContext = NULL;
	int bitStream = 0;
	ogg_int64_t pcmTotal = 0;
	while(true)
	{
		while(player->m_cmdList.size() > 0)
		{
			auto cmd = player->m_cmdList.begin();
			switch(cmd->cmd)
			{
			case SEEK:
				{
					double time = min(max(cmd->time, 0.0), ov_time_total(&(player->m_oggFile), -1));
					ov_time_seek(&(player->m_oggFile), time);
					player->m_voice->Stop();
					player->m_voice->FlushSourceBuffers();
					buffer.Flags = 0;
					buffer.AudioBytes = 0;
					if(player->m_play)
						player->m_voice->Start();
					vorbis_info *oggInfo = ov_info(&(player->m_oggFile), -1);
					XAUDIO2_VOICE_STATE state;
					player->m_voice->GetState(&state);
					player->m_timeBegin = time - state.SamplesPlayed / (double)(oggInfo->rate);
				}
				break;
			case RELEASE:
				if(cmd->releaseVoice != NULL)
				{
					lock.unlock();
					cmd->releaseVoice->DestroyVoice();
					lock.lock();
				}
				ov_clear(&(player->m_oggFile));
				player->m_timeBegin = cmd->time;
				buffer.Flags = 0;
				buffer.AudioBytes = 0;
				pcmTotal = 0;
				break;
			case LOAD:
				{
					assert(cmd->loadFile != NULL && player->m_voice == NULL);
					ov_callbacks oggCallback = {oggRead, oggSeek, oggClose, oggTell};
					if(ov_open_callbacks((void*)cmd->loadFile, &(player->m_oggFile), NULL, 0, oggCallback) == 0)
					{
						vorbis_info *oggInfo = ov_info(&(player->m_oggFile), -1);
						WAVEFORMATEX format;
						format.wFormatTag = WAVE_FORMAT_PCM;
						format.nChannels = oggInfo->channels;
						format.nSamplesPerSec = oggInfo->rate;
						format.nAvgBytesPerSec = 2 * oggInfo->channels * oggInfo->rate;
						format.nBlockAlign = 2 * oggInfo->channels;
						format.wBitsPerSample = 16;
						format.cbSize = 0;
						GameSoundManagerXAudio2::getSingleton()->getIXAudio2()->CreateSourceVoice(&(player->m_voice), &format, 0, XAUDIO2_DEFAULT_FREQ_RATIO, player);
						if(player->m_voice != NULL)
						{
							player->m_voice->SetVolume(player->m_volume);
							player->m_timeBegin = cmd->time;
							ov_time_seek(&player->m_oggFile, cmd->time);
							buffer.Flags = 0;
							buffer.AudioBytes = 0;
							pcmTotal = ov_pcm_total(&(player->m_oggFile), -1);
							if(player->m_play)
								player->m_voice->Start();
						}else
						{
							ov_clear(&(player->m_oggFile));
							player->m_error = true;
						}
					}else
					{
						delete cmd->loadFile;
						player->m_error = true;
					}
				}
				break;
			case QUIT:
				if(player->m_voice != NULL)
				{
					IXAudio2SourceVoice *voice = player->m_voice;
					player->m_voice = NULL;
					lock.unlock();
					voice->DestroyVoice();
					lock.lock();
				}
				ov_clear(&(player->m_oggFile));
				for(int i=0; i<bufferCount; i++)
				{
					delete[] dataBuffer[i];
					dataBuffer[i] = NULL;
				}
				player->m_cmdList.clear();
				lock.unlock();
				return;
			}
			player->m_cmdList.pop_front();
		}
		bool bufferFull = true;
		if(player->m_voice != NULL)
		{
			if(buffer.AudioBytes < bufferSize - 4096 && buffer.Flags != XAUDIO2_END_OF_STREAM)
			{
				lock.unlock();
				long nread = ov_read(&(player->m_oggFile), buffer.AudioBytes+(char*)dataBuffer[ibuffer], bufferSize - buffer.AudioBytes, 0, 2, 1, &bitStream);
				if(ov_pcm_tell(&(player->m_oggFile)) >= pcmTotal)
				{
					buffer.Flags = XAUDIO2_END_OF_STREAM;
					if(player->m_loop)
						ov_pcm_seek(&(player->m_oggFile), 0);
				}else if(nread <= 0)
				{
					buffer.Flags = XAUDIO2_END_OF_STREAM;
				}
				if(nread >= 0)
					buffer.AudioBytes += nread;
				bufferFull = false;
				lock.lock();
			}else
			{
				XAUDIO2_VOICE_STATE state;
				player->m_voice->GetState(&state, XAUDIO2_VOICE_NOSAMPLESPLAYED);
				if(state.BuffersQueued < bufferCount-1)
				{
					ibuffer = (ibuffer+1) % bufferCount;
					player->m_voice->SubmitSourceBuffer(&buffer);
					buffer.Flags = 0;
					buffer.AudioBytes = 0;
					buffer.pAudioData = dataBuffer[ibuffer];
					bufferFull = false;
					player->m_ready = true;
				}
			}
		}
		if(bufferFull && player->m_cmdList.size() <= 0)
			player->m_streamCond.wait(lock);
	}
}

OggStreamPlayer::OggStreamPlayer()
	:m_ready(false), m_play(false), m_loop(false), m_error(false), m_timeBegin(0.0), m_volume(1.0f), m_voice(NULL)
{
}

OggStreamPlayer::~OggStreamPlayer()
{
	assert(m_voice == NULL);
}

void OggStreamPlayer::shutdown()
{
	unique_lock<mutex> lock(m_streamMutex);
	if(m_streamThread.get_id() != thread::id())
	{
		StreamCmd cmd = {QUIT, 0.0, NULL};
		m_cmdList.push_back(cmd);
		lock.unlock();
		m_streamCond.notify_all();
		m_streamThread.join();
	}
}

OggStreamPlayer::LoadResult OggStreamPlayer::load(const wchar_t *path, double time)
{
	unique_lock<mutex> lock(m_streamMutex);
	m_ready = false;
	m_play = false;
	m_error = false;
	m_timeBegin = time;
	std::wstring_convert<std::codecvt_utf8<wchar_t>> cv;
	FileResource *file = FileResource::open(cv.to_bytes(path).c_str());
	char *data = file != NULL ? file->readAll() : NULL;
	if(data == NULL)
	{
		m_error = true;
		return FileNotExist;
	}
	OggFileReader *reader = new OggFileReader(data, file->size());
	FileResource::close(file);
	file = NULL;
	DWORD head;
	if (!reader->readData(&head) || head != 0x5367674f)
	{
		delete reader;
		reader = NULL;
		m_error = true;
		return NotOgg;
	}
	reader->seek(0, SEEK_SET);
	if(m_voice != NULL)
	{
		StreamCmd cmd = {RELEASE, 0.0, NULL};
		cmd.releaseVoice = m_voice;
		m_voice = NULL;
		m_cmdList.push_back(cmd);
	}
	StreamCmd cmd = {LOAD, time, reader};
	m_cmdList.push_back(cmd);
	if(m_streamThread.get_id() == thread::id())
		m_streamThread = thread(oggStreamThread, this);
	lock.unlock();
	m_streamCond.notify_all();
	return Ok;
}

void OggStreamPlayer::release()
{
	unique_lock<mutex> lock(m_streamMutex);
	if(m_voice != NULL)
	{
		m_ready = false;
		m_play = false;
		StreamCmd cmd = {RELEASE, 0.0, NULL};
		cmd.releaseVoice = m_voice;
		m_voice = NULL;
		m_cmdList.push_back(cmd);
		lock.unlock();
		m_streamCond.notify_all();
	}
}

void OggStreamPlayer::play(bool loop)
{
	unique_lock<mutex> lock(m_streamMutex);
	m_play = true;
	m_loop = loop;
	if(m_voice != NULL)
		m_voice->Start();
}

void OggStreamPlayer::pause()
{
	unique_lock<mutex> lock(m_streamMutex);
	m_play = false;
	if(m_voice != NULL)
		m_voice->Stop();
}

void OggStreamPlayer::seek(double time)
{
	unique_lock<mutex> lock(m_streamMutex);
	m_ready = false;
	if(m_voice != NULL)
	{
		StreamCmd cmd = {SEEK, time, NULL};
		m_cmdList.push_back(cmd);
		lock.unlock();
		m_streamCond.notify_all();
	}
}
void OggStreamPlayer::setVolume(double volume)
{
	unique_lock<mutex> lock(m_streamMutex);
	m_volume = (float)volume;
	if(m_voice != NULL)
		m_voice->SetVolume(m_volume);
}

double OggStreamPlayer::getCurrentTime()
{
	unique_lock<mutex> lock(m_streamMutex);
	if(m_voice != NULL)
	{
		if(m_ready)
		{
			vorbis_info *oggInfo = ov_info(&m_oggFile, -1);
			if(oggInfo != NULL && oggInfo->rate > 0)
			{
				XAUDIO2_VOICE_STATE state;
				m_voice->GetState(&state);
				return m_timeBegin + state.SamplesPlayed / (double)(oggInfo->rate);
			}
		}
		else
		{
			return m_timeBegin;
		}
	}
	return 0.0;
}
