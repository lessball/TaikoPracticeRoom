#include "GameBGM.h"

#include <string>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <DXGIFormat.h>
#include <wrl.h>
#include <mfmediaengine.h>
#include <mfapi.h>

#include "OggStreamPlayer.h"

using namespace std;
using namespace Microsoft::WRL;
using namespace Windows::Foundation;

namespace GameBGM
{

class MediaEngineNotify;
class BGMPlayer
{
private:
	bool m_ogg;
	bool m_error;
	bool m_play;
	double m_volume;
	wstring m_path;
	OggStreamPlayer m_oggPlayer;
    ComPtr<IMFMediaEngine> m_spMediaEngine;
public:
	BGMPlayer()
		: m_ogg(false), m_error(false), m_play(false), m_volume(1.0)
	{}
	void release()
	{
		m_ogg = false;
		m_play = false;
		m_path.clear();
		m_oggPlayer.shutdown();
		if(m_spMediaEngine)
		{
			m_spMediaEngine->Shutdown();
			MFShutdown();
			m_spMediaEngine.Reset();
		}
	}

	bool initMF();
	void onMFCanPlay()
	{
		if(m_spMediaEngine)
		{
			if(m_play)
				m_spMediaEngine->Play();
		}
	}
	void onMFEnded()
	{
		m_play = false;
	}
	void onMFError()
	{ 
        if(m_spMediaEngine)
        {
            ComPtr<IMFMediaError> error;
            m_spMediaEngine->GetError(&error);
			if(error)
			{
	            USHORT errorCode = error->GetErrorCode();
				LOG_PRINT("mf error : %d\n", errorCode);
			}
        }
	}
	void setMFVolume()
	{
		//if(m_spMediaEngine)
		//{
		//	if(m_volume <= 0.0)
		//	{
		//		m_spMediaEngine->SetVolume(0.0);
		//	}
		//	else if(m_volume >= 1.0)
		//	{
		//		m_spMediaEngine->SetVolume(1.0);
		//	}
		//	else
		//	{
		//		const double e = 2.7182818;
		//		const double c = 9.21045;
		//		double f = log(m_volume*(pow(2.7182818, c)-1) + 1) / c;
		//		m_spMediaEngine->SetVolume(f);
		//	}
		//}
	}

	bool load(const wchar_t *path, double time)
	{
		assert(path != NULL);
		m_play = false;
		m_error = false;
		if(m_path == path)
		{
			pause();
			seek(time);
			return true;
		}
		m_path = path;
		if(m_path.empty())
		{
			m_error = true;
			return false;
		}
		if(m_ogg)
		{
			m_oggPlayer.release();
		}else if(m_spMediaEngine)
		{
			m_spMediaEngine->Pause();
		}
		switch(m_oggPlayer.load(path, time))
		{
		case OggStreamPlayer::Ok:
			m_ogg = true;
			return true;
		case OggStreamPlayer::FileNotExist:
			m_error = true;
			return false;
		case OggStreamPlayer::NotOgg:
			break;
		}

		m_ogg = false;
		if(!m_spMediaEngine && !initMF())
		{
			m_error = true;
			return false;
		}
		if(m_spMediaEngine)
		{
			setMFVolume();
			WCHAR *twpath = new WCHAR[m_path.size()+1];
			wcscpy_s(twpath, m_path.size()+1, m_path.c_str());
			m_spMediaEngine->SetSource(twpath);
			m_spMediaEngine->SetCurrentTime(time);
			delete[] twpath;
			return true;
		}
		return false;
	}
	void play(bool loop)
	{
		if(m_ogg)
		{
			m_oggPlayer.play(loop);
		}
		else if(m_spMediaEngine)
		{
			m_spMediaEngine->SetLoop(loop ? TRUE : FALSE);
			if(!m_play && m_spMediaEngine->GetReadyState() == MF_MEDIA_ENGINE_READY_HAVE_ENOUGH_DATA)
				m_spMediaEngine->Play();
		}
		m_play = true;
	}
	void pause()
	{
		m_play = false;
		if(m_ogg)
		{
			m_oggPlayer.pause();
		}
		else if(m_spMediaEngine)
		{
			m_spMediaEngine->Pause();
		}
	}
	void seek(double time)
	{
		if(m_ogg)
		{
			m_oggPlayer.seek(time);
		}else if(m_spMediaEngine)
		{
			m_spMediaEngine->SetCurrentTime(time);
		}
	}
	void setVolume(double volume)
	{
		m_volume = volume;
		m_oggPlayer.setVolume(volume);
		setMFVolume();
	}
	bool isReady()
	{
		if(m_ogg)
		{
			return m_oggPlayer.isReady();
		}else if(m_spMediaEngine)
		{
			return m_spMediaEngine->GetReadyState() == MF_MEDIA_ENGINE_READY_HAVE_ENOUGH_DATA;
		}else
		{
			return false;
		}
	}
	bool isPlaying()
	{
		if(m_ogg)
		{
			return m_oggPlayer.isPlaying();
		}else if(m_spMediaEngine)
		{
			return m_play;
		}else
		{
			return false;
		}
	}
	bool isError()
	{
		return m_ogg ? m_oggPlayer.isError() : m_error;
	}
	double getCurrentTime()
	{
		if(m_ogg)
		{
			return m_oggPlayer.getCurrentTime();
		}else if(m_spMediaEngine)
		{
			return m_spMediaEngine->GetCurrentTime();
		}
		else
		{
			return 0.0;
		}
	}
};
static BGMPlayer g_bgmPlayer;

class MediaEngineNotify : public IMFMediaEngineNotify
{
private:
    long m_cRef;
public:
    MediaEngineNotify()
        : m_cRef(1)
    {}
    STDMETHODIMP QueryInterface(REFIID riid, void** ppv)
    {
        if(__uuidof(IMFMediaEngineNotify) == riid)
        {
            *ppv = static_cast<IMFMediaEngineNotify*>(this);
        }
        else
        {
            *ppv = nullptr;
            return E_NOINTERFACE;
        }
        AddRef();
        return S_OK;
    }      
    STDMETHODIMP_(ULONG) AddRef()
    {
        return InterlockedIncrement(&m_cRef);
    }
    STDMETHODIMP_(ULONG) Release()
    {
        LONG cRef = InterlockedDecrement(&m_cRef);
        if (cRef == 0)
        {
            delete this;
        }
        return cRef;
    }
    // EventNotify is called when the Media Engine sends an event.
    STDMETHODIMP EventNotify(DWORD meEvent, DWORD_PTR param1, DWORD param2)
    {
		switch(meEvent)
		{
		case MF_MEDIA_ENGINE_EVENT_NOTIFYSTABLESTATE:
            SetEvent(reinterpret_cast<HANDLE>(param1));
			break;
		case MF_MEDIA_ENGINE_EVENT_CANPLAY:
		case MF_MEDIA_ENGINE_EVENT_SEEKED:
			g_bgmPlayer.onMFCanPlay();
			break;
		case MF_MEDIA_ENGINE_EVENT_ENDED:
			g_bgmPlayer.onMFEnded();
			break;
		case MF_MEDIA_ENGINE_EVENT_ERROR:
			g_bgmPlayer.onMFError();
			break;
		}
        return S_OK;
    }
};

bool BGMPlayer::initMF()
{
	ComPtr<IMFMediaEngineClassFactory> spFactory;
	ComPtr<IMFAttributes> spAttributes;
	ComPtr<MediaEngineNotify> spNotify = new MediaEngineNotify();
	if(FAILED(MFStartup(MF_VERSION)))
		return false;
	if(FAILED(MFCreateAttributes(&spAttributes, 1))
		|| FAILED(spAttributes->SetUnknown(MF_MEDIA_ENGINE_CALLBACK, (IUnknown*) spNotify.Get()))
		|| FAILED(spAttributes->SetUINT32(MF_MEDIA_ENGINE_VIDEO_OUTPUT_FORMAT, DXGI_FORMAT_B8G8R8A8_UNORM))
		|| FAILED(CoCreateInstance(CLSID_MFMediaEngineClassFactory, nullptr, CLSCTX_ALL, IID_PPV_ARGS(&spFactory)))
		|| FAILED(spFactory->CreateInstance(0, spAttributes.Get(), &m_spMediaEngine)))
	{
		MFShutdown();
		return false;
	}
	return true;
}

void release()
{
	g_bgmPlayer.release();
}
bool load(const wchar_t *path, double time)
{
	return g_bgmPlayer.load(path, time);
}
void play(bool loop)
{
	g_bgmPlayer.play(loop);
}
void pause()
{
	g_bgmPlayer.pause();
}
void seek(double time)
{
	g_bgmPlayer.seek(time);
}
void setVolume(double volume)
{
	g_bgmPlayer.setVolume(volume);
}
bool isReady()
{
	return g_bgmPlayer.isReady();
}
bool isPlaying()
{
	return g_bgmPlayer.isPlaying();
}
bool isError()
{
	return g_bgmPlayer.isError();
}
double getCurrentTime()
{
	return g_bgmPlayer.getCurrentTime();
}


} //namespace GameBGM
