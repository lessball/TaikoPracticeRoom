#include "TaikoComponent.h"
#include "Direct3DContentProvider.h"

#include "FilePluginWP8/FilePluginWP8.h"
#include "Direct3D11RenderCore/D3D11RenderCore.h"
#include "GameSoundXAudio2/GameSoundManagerXAudio2.h"
#include "GameBGM.h"

#include <codecvt>
#include <string>

using namespace Windows::Foundation;
using namespace Windows::UI::Core;
using namespace Microsoft::WRL;
using namespace Windows::Phone::Graphics::Interop;
using namespace Windows::Phone::Input::Interop;

using namespace std;
using namespace Windows::Storage;

namespace PhoneDirect3DXamlAppComponent
{

Direct3DBackground::Direct3DBackground() :
	m_timer(ref new BasicTimer())
	, m_device(NULL), m_context(NULL), m_renderTargetView(NULL), m_inGame(false), m_gameCallback(nullptr), m_tjaindex(0), m_tjaloaded(false)
{
	m_taikoAppCallback.m_bg = this;
	m_app.setCallback(&m_taikoAppCallback);
}

IDrawingSurfaceBackgroundContentProvider^ Direct3DBackground::CreateContentProvider()
{
	ComPtr<Direct3DContentProvider> provider = Make<Direct3DContentProvider>(this);
	return reinterpret_cast<IDrawingSurfaceBackgroundContentProvider^>(provider.Get());
}

// IDrawingSurfaceManipulationHandler
void Direct3DBackground::SetManipulationHost(DrawingSurfaceManipulationHost^ manipulationHost)
{
	manipulationHost->PointerPressed +=
		ref new TypedEventHandler<DrawingSurfaceManipulationHost^, PointerEventArgs^>(this, &Direct3DBackground::OnPointerPressed);

	manipulationHost->PointerMoved +=
		ref new TypedEventHandler<DrawingSurfaceManipulationHost^, PointerEventArgs^>(this, &Direct3DBackground::OnPointerMoved);

	manipulationHost->PointerReleased +=
		ref new TypedEventHandler<DrawingSurfaceManipulationHost^, PointerEventArgs^>(this, &Direct3DBackground::OnPointerReleased);
}

void Direct3DBackground::InitCulture(Platform::String ^name)
{
	if(m_renderCore != NULL)
		return;
	if(name->Length() > 0)
		FileResourceManager::getSingleton()->addPlugin(new FilePluginWP8(name->Data()));
	FileResourceManager::getSingleton()->addPlugin(new FilePluginWP8(L"res\\"));
	FileResourceManager::getSingleton()->addPlugin(new FilePluginWP8(L""));
	auto localPath = ApplicationData::Current->LocalFolder->Path + L"\\";
	FileResourceManager::getSingleton()->addPlugin(new FilePluginWP8(localPath->Data()));
}

void Direct3DBackground::SetGameCallBack(IGameCallback ^cb)
{
	m_gameCallback = cb;
}

bool Direct3DBackground::playDemo(Platform::String ^path, double startTime, double volume)
{
	if (!GameBGM::load(path->Data(), startTime))
	{
		return false;
	}
	GameBGM::setVolume(volume);
	GameBGM::play(true);
	return true;
}

bool Direct3DBackground::beginGame(Platform::String ^tjapath, int index, Platform::String ^wavepath, Platform::String ^dbpath, bool autoPlay)
{
	unique_lock<mutex> lock(m_gameMutex);
	m_timer->Reset();
	m_tjapath.clear();
	m_tjapath = tjapath->Data();
	if (!GameBGM::load(wavepath->Data(), 0.0))
	{
		return false;
	}
	m_dbpath = dbpath->Data();
	m_tjaindex = index;
	m_tjaloaded = false;
	m_autoPlay = autoPlay;
	m_inGame = true;
	RequestAdditionalFrame();
	return true;
}

void Direct3DBackground::stopGame()
{
	unique_lock<mutex> lock(m_gameMutex);
	m_app.stopGame();
	m_inGame = false;
}

void Direct3DBackground::submitResult(bool trueScore, bool success, bool fullCombo, int score, int combo, int good, int normal, int bad)
{
	m_app.stopGame();
	m_inGame = false;
	if(m_gameCallback != nullptr && !m_tjapath.empty())
		m_gameCallback->submitResult(ref new Platform::String(m_dbpath.c_str()), trueScore, success, fullCombo, score, combo, good, normal, bad);
}

void Direct3DBackground::clearMemoryFileData()
{
	if (m_filePlugin != NULL)
	{
		m_filePlugin->clearAllFile();
	}
}

void Direct3DBackground::setMemoryFileData(Platform::String ^path, const Platform::Array<byte> ^data)
{
	if (m_filePlugin == NULL)
	{
		m_filePlugin = new FilePluginCXArray();
		FileResourceManager::getSingleton()->addPlugin(m_filePlugin);
	}
	m_filePlugin->addFile(path, data);
}

// Event Handlers
void Direct3DBackground::OnPointerPressed(DrawingSurfaceManipulationHost^ sender, PointerEventArgs^ args)
{
	// Insert your code here.
	if(m_renderCore != NULL && m_inGame)
	{
		//unique_lock<mutex> lock(m_inputMutex);
		//Input tinput = {INPUT_PRESS, (int)(args->CurrentPoint->Position.X * RenderResolution.Width / WindowBounds.Width), (int)(args->CurrentPoint->Position.Y * RenderResolution.Height / WindowBounds.Height)};
		//Input tinput = {INPUT_PRESS, (int)(args->CurrentPoint->Position.Y * RenderResolution.Height / WindowBounds.Height), (int)((WindowBounds.Width - args->CurrentPoint->Position.X) * RenderResolution.Width / WindowBounds.Width)};
		//m_inputList.push_back(tinput);
		m_app.onPress(args->CurrentPoint->Position.X * RenderResolution.Width / WindowBounds.Width, args->CurrentPoint->Position.Y * RenderResolution.Height / WindowBounds.Height);
	}
}

void Direct3DBackground::OnPointerMoved(DrawingSurfaceManipulationHost^ sender, PointerEventArgs^ args)
{
	// Insert your code here.
	//if(m_renderCore != NULL)
	//{
	//	unique_lock<mutex> lock(m_inputMutex);
	//	Input tinput = {INPUT_MOVE, (int)(args->CurrentPoint->Position.X * RenderResolution.Width / WindowBounds.Width), (int)(args->CurrentPoint->Position.Y * RenderResolution.Height / WindowBounds.Height)};
	//	m_inputList.push_back(tinput);
	//}
}

void Direct3DBackground::OnPointerReleased(DrawingSurfaceManipulationHost^ sender, PointerEventArgs^ args)
{
	// Insert your code here.
	//if(m_renderCore != NULL)
	//{
	//	unique_lock<mutex> lock(m_inputMutex);
	//	Input tinput = {INPUT_RELEASE, (int)(args->CurrentPoint->Position.X * RenderResolution.Width / WindowBounds.Width), (int)(args->CurrentPoint->Position.Y * RenderResolution.Height / WindowBounds.Height)};
	//	m_inputList.push_back(tinput);
	//}
}

// Interface With Direct3DContentProvider
HRESULT Direct3DBackground::Connect(_In_ IDrawingSurfaceRuntimeHostNative* host, _In_ ID3D11Device1* device)
{
	unique_lock<mutex> lock(m_gameMutex);
	m_timer->Reset();
	//GameSoundManagerXAudio2::getSingleton()->getIXAudio2()->StartEngine();
	GameSoundManager::getSingleton()->initSoundManager(); //TODO
	return S_OK;
}

void Direct3DBackground::Disconnect()
{
	unique_lock<mutex> lock(m_gameMutex);
	m_app.release();
	if(m_renderCore != NULL)
	{
		RenderCore::destroyRenderCore(m_renderCore);
		m_renderCore = NULL;
	}
//	m_app.saveState();
	m_device = NULL;
	m_context = NULL;
	m_renderTargetView = NULL;
	//GameBGM::stop();
	//GameSoundManagerXAudio2::getSingleton()->getIXAudio2()->StopEngine();
	GameSoundManager::getSingleton()->releaseSoundManager();//TODO
	//unique_lock<mutex> lock(m_inputMutex);
	//m_inputList.clear();

	if (m_filePlugin != NULL)
	{
		m_filePlugin->clearAllFile();
	}
}

HRESULT Direct3DBackground::PrepareResources(_In_ const LARGE_INTEGER* presentTargetTime, _Inout_ DrawingSurfaceSizeF* desiredRenderTargetSize)
{
	unique_lock<mutex> lock(m_gameMutex);
	if(!m_inGame)
		return S_OK;
	m_timer->Update();

	desiredRenderTargetSize->width = RenderResolution.Width;
	desiredRenderTargetSize->height = RenderResolution.Height;

	if(m_renderCore != NULL)
	{
		//m_inputMutex.lock();
		//for(auto i = m_inputList.begin(); i != m_inputList.end();)
		//{
		//	switch(i->type)
		//	{
		//	case INPUT_PRESS:
		//		m_app.onPress(i->x, i->y);
		//		break;
		//	}
		//	++i;
		//	m_inputList.pop_front();
		//}
		//m_inputMutex.unlock();
		if(!m_app.mainLoop(m_timer->Total))
		{
			m_app.release();
			RenderCore::destroyRenderCore(m_renderCore);
			m_renderCore = NULL;
			FileResourceManager::getSingleton()->release();
			GameBGM::release();
			GameSoundManager::getSingleton()->releaseSoundManager();
			return S_FALSE;
		}
	}

	return S_OK;
}

HRESULT Direct3DBackground::Draw(_In_ ID3D11Device1* device, _In_ ID3D11DeviceContext1* context, _In_ ID3D11RenderTargetView* renderTargetView)
{
	unique_lock<mutex> lock(m_gameMutex);
	if(!m_inGame)
		return S_OK;
	if(device != m_device)
	{
		if(m_renderCore != NULL)
		{
			m_app.release();
			RenderCore::destroyRenderCore(m_renderCore);
		}
		m_device = device;
		m_renderCore = RenderCore::createRenderCore(boost::any(m_device), 24, 0, 0, 1, 1);
		if(m_renderCore == NULL)
			return S_FALSE;
		(static_cast<D3D11RenderCore*>(m_renderCore))->updateContext(context, renderTargetView);
		(static_cast<D3D11RenderCore*>(m_renderCore))->setScissorScale(WindowBounds.Width / RenderResolution.Width, WindowBounds.Height / RenderResolution.Height);
		TextureManager::getSingleton()->init(m_renderCore);
		int nDirection = 1;
		if(m_gameCallback != nullptr && m_gameCallback->getDirection() == 3)
			nDirection = 3;
		if(!m_app.init(m_renderCore, (int)(0.5f+RenderResolution.Width), (int)(0.5f+RenderResolution.Height), nDirection))
		{
			m_app.release();
			RenderCore::destroyRenderCore(m_renderCore);
			m_renderCore = NULL;
			FileResourceManager::getSingleton()->release();
			GameBGM::release();
			GameSoundManager::getSingleton()->releaseSoundManager();
			return S_FALSE;
		}
	}else if(context != m_context || renderTargetView != m_renderTargetView)
	{
		m_context = context;
		m_renderTargetView = renderTargetView;
		(static_cast<D3D11RenderCore*>(m_renderCore))->updateContext(context, renderTargetView);
	}
	if(!m_tjaloaded && !m_tjapath.empty())
	{
		std::wstring_convert<std::codecvt_utf8<wchar_t>> cv;
		string utjapath = cv.to_bytes(m_tjapath);
		int nDirection = 1;
		if(m_gameCallback != nullptr && m_gameCallback->getDirection() == 3)
			nDirection = 3;
		m_app.setDirection(nDirection);
		m_app.beginGame(utjapath.c_str(), m_tjaindex, m_autoPlay);
		m_tjaloaded = true;
	}
	m_app.render();

	RequestAdditionalFrame();

	return S_OK;
}

}