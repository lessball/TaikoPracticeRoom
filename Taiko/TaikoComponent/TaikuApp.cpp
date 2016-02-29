#include "TaikoApp.h"
#include <FileResource/XmlFileReader.h>
#include <FileResource/FileResource.h>
#include "GameBGM.h"
#include <assert.h>

void TaikoApp::fixInputPosition(float &x, float &y)
{
	switch(m_direction)
	{
	case 1:
		{
			float tx = x;
			x = y;
			y = m_width - tx;
		}
		break;
	case 3:
		{
			float tx = x;
			x = m_height - y;
			y = tx;
		}
		break;
	}
}

TaikoApp::TaikoApp()
	:m_rc(NULL), m_callback(NULL)
{
}
TaikoApp::~TaikoApp()
{
	assert(m_rc == NULL);
}
bool TaikoApp::init(RenderCore *rc, int width, int height, int direction)
{
	m_rc = rc;
	m_runTime = 0.0;
	{
		XmlFileReader xfile("render/core.xml");
		if(xfile.getRootNode() == NULL || !m_rc->init(xfile.getRootNode(), "render/"))
			return false;
	}
	m_imageRender.init(m_rc, true, true);
	m_width = width;
	m_height = height;
	setDirection(direction);
	rc->setMainViewSize(width, height);
	rc->setViewPort(0, 0, width, height);
	BlendStateDesc blendDesc;
	blendDesc.blendEnable = true;
	blendDesc.colorSrc = BLEND_SRC_ALPHA;
	blendDesc.colorDst = BLEND_INV_SRC_ALPHA;
	blendDesc.colorOp  = BLEND_ADD;
	blendDesc.alphaSrc = BLEND_SRC_ALPHA;
	blendDesc.alphaDst = BLEND_INV_SRC_ALPHA;
	blendDesc.alphaOp  = BLEND_ADD;
	for(int i=0; i<4; i++)
		blendDesc.colorWriteEnable[i] = true;
	RenderBlendState *blendState = m_rc->createBlendState(&blendDesc);
	m_rc->setBlendState(blendState);
	m_rc->destroyBlendState(blendState);
	blendState = NULL;
	DepthStencilStateDesc dsDesc;
	memset(&dsDesc, 0, sizeof(dsDesc));
	dsDesc.depthTestEnable = false;
	dsDesc.depthWriteEnable = false;
	RenderDepthStencilState *dsState = m_rc->createDepthStencilState(&dsDesc);
	m_rc->setDepthStencilState(dsState);
	m_rc->destroyDepthStencilState(dsState);
	m_rc->setCullMode(CULL_NONE);
	
	m_game.init();
	m_skin.init();
	m_imageRender.getRootNode()->addChild(m_skin.getImageRoot());

	m_runTime = -1.0f;
	return true;
}
void TaikoApp::release()
{
	GameBGM::release();
	m_game.release();
	m_imageRender.getRootNode()->clearChild();
	m_skin.release();
	if(m_rc != NULL)
	{
		m_imageRender.release(m_rc);
		m_rc->release();
		m_rc = NULL;
	}
}
bool TaikoApp::mainLoop(double runTime)
{
	if(m_runTime >= 0.0)
	{
		m_game.update((float)(runTime - m_runTime), &m_skin);
		Animation2DManager::getSingleton()->update((int)(runTime*1000) - (int)(m_runTime*1000));
		m_runTime = runTime;
		bool trueScore = false;
		bool success = false;
		bool fullCombo = false;
		int score = 0;
		int combo = 0;
		int good = 0;
		int normal = 0;
		int bad = 0;
		if(m_callback != NULL && !GameBGM::isPlaying() && m_game.getScore(trueScore, success, fullCombo, score, combo, good, normal, bad))
			m_callback->submitResult(trueScore, success, fullCombo, score, combo, good, normal, bad);
	}else
	{
		m_runTime = runTime;
	}
	m_imageRender.getRootNode()->updateVisibleBranchMatrix(NULL, false);
	return true;
}
bool TaikoApp::render()
{
	m_rc->clear(true, 0.0f, 0.698039216f, 0.921568627f, 1.0f, false, 1.0f, false, 0);
	m_rc->begin();
	m_imageRender.render(m_rc);
	m_rc->end();
	return m_game.isPlaying() || GameBGM::isPlaying();
}
void TaikoApp::setCallback(SubmitResultCallback *callback)
{
	m_callback = callback;
}
void TaikoApp::setDirection(int direction)
{
	m_direction = direction % 4;
	switch(m_direction)
	{
	case 0:
		m_imageRender.setViewPortSize(m_width, m_height);
		break;
	case 1:
		{
			float rootMatrix[] = {0.0f,-2.0f/m_width,1.0f, -2.0f/m_height,0.0f,1.0f};
			m_imageRender.getRootNode()->setLocalMatrix(rootMatrix);
		}
		break;
	case 2:
		{
			float rootMatrix[] = {-2.0f/m_width,0.0f,-1.0f, 0.0f,2.0f/m_height,1.0f};
			m_imageRender.getRootNode()->setLocalMatrix(rootMatrix);
		}
		break;
	case 3:
		{
			float rootMatrix[] = {0.0f,2.0f/m_width,-1.0f, 2.0f/m_height,0.0f,-1.0f};
			m_imageRender.getRootNode()->setLocalMatrix(rootMatrix);
		}
		break;
	}
}
void TaikoApp::beginGame(const char *path, int index, bool autoPlay)
{
	m_game.releaseGame();
	if(m_game.loadtja(path, index))
		m_game.begin(&m_skin, autoPlay);
}

void TaikoApp::stopGame()
{
	m_game.releaseGame();
}

void TaikoApp::onPress(float x, float y)
{
	fixInputPosition(x, y);
	bool left = false;
	int type = m_skin.getHitType(x, y, &left);
	input(type, left);
}

void TaikoApp::setDrumScale(float scale)
{
	m_skin.setDrumScale(scale);
}
