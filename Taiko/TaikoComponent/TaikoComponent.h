#pragma once

#include "TaikoApp.h"
#include "BasicTimer.h"

#include "FilePluginWP8\FilePluginCXArray.h"

#include <DrawingSurfaceNative.h>
#include <list>
#include <mutex>

namespace PhoneDirect3DXamlAppComponent
{
	
public interface class IGameCallback
{
	int getDirection();
    void submitResult(Platform::String ^dbpath, bool trueScore, bool success, bool fullCombo, int score, int combo, int good, int normal, int bad);
};

public delegate void RequestAdditionalFrameHandler();

[Windows::Foundation::Metadata::WebHostHidden]
public ref class Direct3DBackground sealed : public Windows::Phone::Input::Interop::IDrawingSurfaceManipulationHandler
{
public:
	Direct3DBackground();

	Windows::Phone::Graphics::Interop::IDrawingSurfaceBackgroundContentProvider^ CreateContentProvider();

	// IDrawingSurfaceManipulationHandler
	virtual void SetManipulationHost(Windows::Phone::Input::Interop::DrawingSurfaceManipulationHost^ manipulationHost);

	event RequestAdditionalFrameHandler^ RequestAdditionalFrame;

	property Windows::Foundation::Size WindowBounds;
	property Windows::Foundation::Size NativeResolution;
	property Windows::Foundation::Size RenderResolution;

	void InitCulture(Platform::String ^name);
	void SetGameCallBack(IGameCallback ^cb);
	
	bool playDemo(Platform::String ^path, double startTime, double volume);
	bool beginGame(Platform::String ^tjapath, int index, Platform::String ^wavepath, Platform::String ^dbpath, bool autoPlay);
	void stopGame();
    void submitResult(bool trueScore, bool success, bool fullCombo, int score, int combo, int good, int normal, int bad);

	void clearMemoryFileData();
	void setMemoryFileData(Platform::String ^path, const Platform::Array<byte> ^data);

	void beginRender();
	void setDrumScale(float scale);

protected:
	// Event Handlers
	void OnPointerPressed(Windows::Phone::Input::Interop::DrawingSurfaceManipulationHost^ sender, Windows::UI::Core::PointerEventArgs^ args);
	void OnPointerReleased(Windows::Phone::Input::Interop::DrawingSurfaceManipulationHost^ sender, Windows::UI::Core::PointerEventArgs^ args);
	void OnPointerMoved(Windows::Phone::Input::Interop::DrawingSurfaceManipulationHost^ sender, Windows::UI::Core::PointerEventArgs^ args);

internal:
	HRESULT Connect(_In_ IDrawingSurfaceRuntimeHostNative* host, _In_ ID3D11Device1* device);
	void Disconnect();

	HRESULT PrepareResources(_In_ const LARGE_INTEGER* presentTargetTime, _Inout_ DrawingSurfaceSizeF* desiredRenderTargetSize);
	HRESULT Draw(_In_ ID3D11Device1* device, _In_ ID3D11DeviceContext1* context, _In_ ID3D11RenderTargetView* renderTargetView);

private:
	ID3D11Device *m_device;
	ID3D11DeviceContext1 *m_context;
	ID3D11RenderTargetView *m_renderTargetView;
	RenderCore *m_renderCore;
	BasicTimer^ m_timer;
	TaikoApp m_app;
	bool m_inGame;
	std::mutex m_gameMutex;
	IGameCallback ^m_gameCallback;
	struct TaikoAppCallback : public SubmitResultCallback
	{
	public:
		Direct3DBackground ^m_bg;
		void submitResult(bool trueScore, bool success, bool fullCombo, int score, int combo, int good, int normal, int bad)
		{
			if(m_bg != nullptr)
				m_bg->submitResult(trueScore, success, fullCombo, score, combo, good, normal, bad);
		}
	};
	TaikoAppCallback m_taikoAppCallback;

	//enum InputType
	//{
	//	INPUT_PRESS,
	//	INPUT_MOVE,
	//	INPUT_RELEASE,
	//	INPUT_RETURN,
	//};
	//struct Input
	//{
	//	InputType type;
	//	int x;
	//	int y;
	//};
	//std::list<Input> m_inputList;
	//std::mutex m_inputMutex;
	std::wstring m_tjapath;
	int m_tjaindex;
	bool m_tjaloaded;
	bool m_autoPlay;
	std::wstring m_dbpath;
	FilePluginCXArray *m_filePlugin;
	float m_drumScale;
};

}