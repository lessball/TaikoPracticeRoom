#ifndef TAIKUAPP_H
#define	TAIKUAPP_H

#include <RenderCore/RenderCore.h>
#include <Game2DImage/Game2DImageRender.h>
#include "TaikoGame.h"
#include "TaikoSkin.h"

class SubmitResultCallback
{
public:
	virtual void submitResult(bool trueScore, bool success, bool fullCombo, int score, int combo, int good, int normal, int bad) = 0;
};

class TaikoApp
{
private:
	int m_direction;
	int m_width;
	int m_height;
	TaikoGame m_game;
	TaikoSkin m_skin;
	RenderCore *m_rc;
	Game2DImageRender m_imageRender;
	double m_runTime;
	SubmitResultCallback *m_callback;

	void fixInputPosition(float &x, float &y);
public:
	TaikoApp();
	~TaikoApp();
	//cw direction*pi/4
	bool init(RenderCore *rc, int width, int height, int direction);
	void release();
	bool mainLoop(double runTime);
	bool render();

	void setCallback(SubmitResultCallback *callback);
	void setDirection(int direction);
	void beginGame(const char *path, int index, bool autoPlay);
	void stopGame();
	void input(int type, bool left)
	{
		m_game.input(type, left, &m_skin);
	}
	void onPress(float x, float y);

	void setDrumScale(float scale);
};

#endif
