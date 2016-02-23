#ifndef D3D11RENDERSTATE_H
#define D3D11RENDERSTATE_H

#include <RenderCore/RenderState.h>
#include <d3d11.h>

class D3D11BlendState : public RenderBlendState
{
private:
	BlendStateDesc m_desc;
	ID3D11BlendState *m_state;
	D3D11BlendState();
public:
	D3D11BlendState(const BlendStateDesc &desc, ID3D11Device *pdev);
	~D3D11BlendState()
	{
		if(m_state != NULL)
			m_state->Release();
	}
	
	ID3D11BlendState *getD3DBlendState() const
	{
		return m_state;
	}
	void getDesc(BlendStateDesc *desc) const
	{
		*desc = m_desc;
	}

	static int getKey(const BlendStateDesc &desc);
};

class D3D11DepthStencilState : public RenderDepthStencilState
{
private:
	DepthStencilStateDesc m_desc;
	ID3D11DepthStencilState *m_state;
	D3D11DepthStencilState();
public:
	D3D11DepthStencilState(const DepthStencilStateDesc &desc, ID3D11Device *pdev);
	~D3D11DepthStencilState()
	{
		if(m_state != NULL)
			m_state->Release();
	}

	ID3D11DepthStencilState *getD3DDepthStencilState() const
	{
		return m_state;
	}
	void getDesc(DepthStencilStateDesc *desc) const
	{
		*desc = m_desc;
	}

	static long long getKey(const DepthStencilStateDesc &desc);
};

#endif
