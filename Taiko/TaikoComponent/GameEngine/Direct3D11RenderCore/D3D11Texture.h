#ifndef D3D11TEXTURE_H
#define D3D11TEXTURE_H

#include <RenderCore/RenderTexture.h>
#include <d3d11.h>
#include <assert.h>

class D3D11Texture : public RenderTexture
{
protected:
	friend class RenderTexture;
	bool m_rt;
	bool m_cube;
	int m_width;
	int m_height;
	int m_format;
	ID3D11Texture2D *m_tex;
	ID3D11ShaderResourceView *m_view;
public:
	D3D11Texture();
	~D3D11Texture();

	bool init(ID3D11Device *pdev, ID3D11DeviceContext *pcontext, int width, int height, int format, const void *data, bool mipmap, bool cube, bool renderTarget);

	bool isRenderTarget() const
	{
		return m_rt;
	}
	ID3D11ShaderResourceView *getSRV() const
	{
		return m_view;
	}
};

class D3D11RenderTarget : public D3D11Texture
{
private:
	ID3D11Texture2D *m_depthBuffer;
	ID3D11DepthStencilView *m_depthView;
	union
	{
		ID3D11RenderTargetView *m_rtView;
		ID3D11RenderTargetView **m_rtCubeView;
	};
public:
	D3D11RenderTarget();
	~D3D11RenderTarget();

	bool init(ID3D11Device *pdev, ID3D11DeviceContext *pcontext, int width, int height, int format, int depthbit, int stencilbit, bool cube);
	
	ID3D11RenderTargetView *getRTView(int surface) const
	{
		assert((surface==TS_2D && !m_cube) || (surface!=TS_2D && m_cube));
		return m_cube ? m_rtCubeView[surface -TS_CUBE_X_POS] : m_rtView;
	}
	ID3D11DepthStencilView *getDSView() const
	{
		return m_depthView;
	}
};

#endif
