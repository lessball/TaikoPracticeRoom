#include "D3D11RenderState.h"

D3D11BlendState::D3D11BlendState(const BlendStateDesc &desc, ID3D11Device *pdev)
{
	m_desc = desc;

	const D3D11_BLEND d3dBlend[] =
	{
		D3D11_BLEND_ZERO, D3D11_BLEND_ONE,
		D3D11_BLEND_SRC_ALPHA, D3D11_BLEND_INV_SRC_ALPHA,
		D3D11_BLEND_DEST_ALPHA, D3D11_BLEND_INV_DEST_ALPHA,
		D3D11_BLEND_SRC_COLOR, D3D11_BLEND_INV_SRC_COLOR,
		D3D11_BLEND_DEST_COLOR, D3D11_BLEND_INV_DEST_COLOR,
		D3D11_BLEND_BLEND_FACTOR, D3D11_BLEND_INV_BLEND_FACTOR,
	};
	const D3D11_BLEND_OP d3dBlendOp[] = {D3D11_BLEND_OP_ADD, D3D11_BLEND_OP_SUBTRACT, D3D11_BLEND_OP_REV_SUBTRACT};
	const UINT8 d3dWriteMask[] = {D3D11_COLOR_WRITE_ENABLE_RED, D3D11_COLOR_WRITE_ENABLE_GREEN, D3D11_COLOR_WRITE_ENABLE_BLUE, D3D11_COLOR_WRITE_ENABLE_ALPHA};

	D3D11_BLEND_DESC d3ddesc;
	memset(&d3ddesc, 0, sizeof(d3ddesc));
	d3ddesc.AlphaToCoverageEnable = FALSE;
	d3ddesc.IndependentBlendEnable = FALSE;
	d3ddesc.RenderTarget[0].BlendEnable = desc.blendEnable ? TRUE : FALSE;
	d3ddesc.RenderTarget[0].SrcBlend = d3dBlend[desc.colorSrc];
	d3ddesc.RenderTarget[0].DestBlend = d3dBlend[desc.colorDst];
	d3ddesc.RenderTarget[0].BlendOp = d3dBlendOp[desc.colorOp];
	d3ddesc.RenderTarget[0].SrcBlendAlpha = d3dBlend[desc.alphaSrc];
	d3ddesc.RenderTarget[0].DestBlendAlpha = d3dBlend[desc.alphaDst];
	d3ddesc.RenderTarget[0].BlendOpAlpha = d3dBlendOp[desc.alphaOp];
	d3ddesc.RenderTarget[0].RenderTargetWriteMask = 0;
	for(int i=0; i<4; i++)
	{
		if(desc.colorWriteEnable[i])
			d3ddesc.RenderTarget[0].RenderTargetWriteMask |= d3dWriteMask[i];
	}
	m_state = NULL;
	pdev->CreateBlendState(&d3ddesc, &m_state);
}
int D3D11BlendState::getKey(const BlendStateDesc &desc)
{
	return (desc.blendEnable ? 1 : 0)
		| ((int)desc.colorSrc) << 1
		| ((int)desc.colorDst) << 5
		| ((int)desc.colorOp) << 9
		| ((int)desc.alphaSrc) << 11
		| ((int)desc.alphaDst) << 15
		| ((int)desc.alphaOp) << 19
		| (desc.colorWriteEnable[0] ? 1<<21 : 0)
		| (desc.colorWriteEnable[1] ? 1<<22 : 0)
		| (desc.colorWriteEnable[2] ? 1<<23 : 0)
		| (desc.colorWriteEnable[3] ? 1<<24 : 0);
}

void RenderBlendState::getDesc(BlendStateDesc *desc) const
{
	const D3D11BlendState *pthis = static_cast<const D3D11BlendState*>(this);
	pthis->getDesc(desc);
}

D3D11DepthStencilState::D3D11DepthStencilState(const DepthStencilStateDesc &desc, ID3D11Device *pdev)
{
	m_desc = desc;

	const D3D11_COMPARISON_FUNC d3dComp[] =
	{
		D3D11_COMPARISON_NEVER,
		D3D11_COMPARISON_ALWAYS,
		D3D11_COMPARISON_LESS,
		D3D11_COMPARISON_GREATER,
		D3D11_COMPARISON_LESS_EQUAL,
		D3D11_COMPARISON_GREATER_EQUAL,
		D3D11_COMPARISON_EQUAL,
		D3D11_COMPARISON_NOT_EQUAL,
	};
	const D3D11_STENCIL_OP d3dStencilOp[] =
	{
		D3D11_STENCIL_OP_KEEP,
		D3D11_STENCIL_OP_ZERO,
		D3D11_STENCIL_OP_REPLACE,
		D3D11_STENCIL_OP_INCR_SAT,
		D3D11_STENCIL_OP_DECR_SAT,
		D3D11_STENCIL_OP_INCR,
		D3D11_STENCIL_OP_DECR,
		D3D11_STENCIL_OP_INVERT,
	};

	D3D11_DEPTH_STENCIL_DESC d3ddesc;
	memset(&d3ddesc, 0, sizeof(d3ddesc));
	d3ddesc.DepthEnable = desc.depthTestEnable ? TRUE : FALSE;
	d3ddesc.DepthWriteMask = desc.depthWriteEnable ? D3D11_DEPTH_WRITE_MASK_ALL : D3D11_DEPTH_WRITE_MASK_ZERO;
	d3ddesc.DepthFunc = d3dComp[desc.depthCompare];
	d3ddesc.StencilEnable = desc.stencilEnable ? TRUE : FALSE;
	d3ddesc.StencilReadMask = (UINT8)desc.stencilReadMask;
	d3ddesc.StencilWriteMask = (UINT8)desc.stencilWriteMask;
	d3ddesc.FrontFace.StencilFailOp = d3dStencilOp[desc.stencilFailOp[0]];
	d3ddesc.FrontFace.StencilDepthFailOp = d3dStencilOp[desc.depthFailOp[0]];
	d3ddesc.FrontFace.StencilPassOp = d3dStencilOp[desc.passOp[0]];
	d3ddesc.FrontFace.StencilFunc = d3dComp[desc.stencilCompare[0]];
	d3ddesc.BackFace.StencilFailOp = d3dStencilOp[desc.stencilFailOp[1]];
	d3ddesc.BackFace.StencilDepthFailOp = d3dStencilOp[desc.depthFailOp[1]];
	d3ddesc.BackFace.StencilPassOp = d3dStencilOp[desc.passOp[1]];
	d3ddesc.BackFace.StencilFunc = d3dComp[desc.stencilCompare[1]];
	m_state = NULL;
	pdev->CreateDepthStencilState(&d3ddesc, &m_state);
}
long long D3D11DepthStencilState::getKey(const DepthStencilStateDesc &desc)
{
	int low = (desc.depthTestEnable ? 1 : 0)
		| (desc.depthWriteEnable ? 2 : 0)
		| ((int)desc.depthCompare) << 2
		| (desc.stencilEnable ? 1<<15 : 0)
		| ((int)desc.stencilReadMask) << 16
		| ((int)desc.stencilWriteMask) << 24;
	int high = ((int)desc.stencilFailOp[0])
		| ((int)desc.stencilFailOp[1]) << 4
		| ((int)desc.depthFailOp[0]) << 8
		| ((int)desc.depthFailOp[1]) << 12
		| ((int)desc.passOp[0]) << 16
		| ((int)desc.passOp[1]) << 20
		| ((int)desc.stencilCompare[0]) << 24
		| ((int)desc.stencilCompare[1]) << 28;
	return ((long long)high) << 32 | (long long)low;
}
void RenderDepthStencilState::getDesc(DepthStencilStateDesc *desc) const
{
	const D3D11DepthStencilState *pthis = static_cast<const D3D11DepthStencilState *>(this);
	pthis->getDesc(desc);
}
