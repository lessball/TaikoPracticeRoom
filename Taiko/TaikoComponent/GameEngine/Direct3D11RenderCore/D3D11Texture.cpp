#include "D3D11Texture.h"
#include <LogPrint.h>
#include <assert.h>

static const DXGI_FORMAT D3DTextureFormat[] =
{
	DXGI_FORMAT_R8G8B8A8_UNORM, //PF_R8G8B8
	DXGI_FORMAT_R8G8B8A8_UNORM, //PF_R8G8B8A8
	DXGI_FORMAT_UNKNOWN, //PF_R5G6B5
	DXGI_FORMAT_UNKNOWN, //PF_R4G4B4A4
	DXGI_FORMAT_UNKNOWN, //PF_R5G5B5A1,
	DXGI_FORMAT_R8_UNORM, //PF_L8,
	DXGI_FORMAT_R16_UNORM, //PF_L16,
};
static const int D3DPixelSize[] =
{
	4, //PF_R8G8B8
	4, //PF_R8G8B8A8
	0, //PF_R5G6B5
	0, //PF_R4G4B4A4
	0, //PF_R5G5B5A1,
	1, //PF_L8,
	2, //PF_L16,
};

static void Convert24To32(void *tar, const void *src, int size)
{
	for(int i=0; i<size; i++)
	{
		char *pTar = i*4 + (char *)tar;
		char *pSrc = i*3 + (char *)src;
		memcpy(pTar, pSrc, 3);
		pTar[3] = -1;
	}
}

D3D11Texture::D3D11Texture()
{
	m_rt = false;
	m_cube = false;
	m_width = 0;
	m_height = 0;
	m_format = 0;
	m_tex = NULL;
	m_view = NULL;
}
D3D11Texture::~D3D11Texture()
{
	if(m_view != NULL)
	{
		m_view->Release();
		m_view = NULL;
	}
	if(m_tex != NULL)
	{
		m_tex->Release();
		m_tex = NULL;
	}
}

bool D3D11Texture::init(ID3D11Device *pdev, ID3D11DeviceContext *pcontext, int width, int height, int format, const void *data, bool mipmap, bool cube, bool renderTarget)
{
	assert(data != NULL || (!mipmap && !cube) || renderTarget);
	assert(width == height || !cube);
	m_cube = cube;
	m_width = width;
	m_height = height;
	m_format = format;

	HRESULT hr;
	D3D11_TEXTURE2D_DESC desc;
	memset(&desc, 0, sizeof(desc));
	desc.Width = width;
	desc.Height = height;
	desc.MipLevels = mipmap ? 0 : 1;
	desc.ArraySize = cube ? 6 : 1;
	desc.Format = D3DTextureFormat[format];
	if(desc.Format == DXGI_FORMAT_UNKNOWN)
		return false;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Usage = (data != NULL || renderTarget) ? D3D11_USAGE_DEFAULT : D3D11_USAGE_DYNAMIC;
	desc.BindFlags = (renderTarget || mipmap) ? (D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET) : D3D11_BIND_SHADER_RESOURCE;
	desc.CPUAccessFlags =  (data != NULL || renderTarget) ? 0 : D3D11_CPU_ACCESS_WRITE;
	desc.MiscFlags = (mipmap ? D3D11_RESOURCE_MISC_GENERATE_MIPS : 0) | (cube ? D3D11_RESOURCE_MISC_TEXTURECUBE : 0);
	hr = pdev->CreateTexture2D(&desc, NULL, &m_tex);
	if(FAILED(hr))
		return false;
	if(data != NULL)
	{
		int srcRowPitch = width * D3DPixelSize[format];
		if(cube)
		{
			int nMip = 1;
			if(mipmap)
			{
				for(int i=width; i>1; i=i>>1)
					nMip++;
			}
			if(format != PF_R8G8B8)
			{
				for(int i=0; i<6; i++)
				{
					int nSubRes = D3D11CalcSubresource(0, i, nMip);
#if !WINAPI_FAMILY_PARTITION( WINAPI_PARTITION_PHONE )
					pcontext->UpdateSubresource(m_tex, nSubRes, NULL, ((void**)data)[i], srcRowPitch, 0);
#else
					if(!mipmap)
					{
						pcontext->UpdateSubresource(m_tex, nSubRes, NULL, ((void**)data)[i], srcRowPitch, 0);
					}
					else
					{
						D3D11_TEXTURE2D_DESC desc;
						memset(&desc, 0, sizeof(desc));
						desc.Width = width;
						desc.Height = height;
						desc.MipLevels = 1;
						desc.ArraySize = 1;
						desc.Format = D3DTextureFormat[format];
						desc.SampleDesc.Count = 1;
						desc.SampleDesc.Quality = 0;
						desc.Usage = D3D11_USAGE_DEFAULT;
						desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
						desc.CPUAccessFlags =  0;
						desc.MiscFlags = 0;
						D3D11_SUBRESOURCE_DATA initData;
						initData.pSysMem = ((void**)data)[i];
						initData.SysMemPitch = srcRowPitch;
						initData.SysMemSlicePitch = 0;
						ID3D11Texture2D *tempTex = NULL;
						hr = pdev->CreateTexture2D(&desc, &initData, &tempTex);
						if(FAILED(hr))
							return false;
						pcontext->CopySubresourceRegion(m_tex, nSubRes, 0, 0, 0, tempTex, 0, NULL);
						tempTex->Release();
					}
#endif
				}
			}else
			{
				char *pData = new char[width*height*4];
				for(int i=0; i<6; i++)
				{
					Convert24To32(pData, ((void**)data)[i], width*height);
					int nSubRes = D3D11CalcSubresource(0, i, nMip);
					pcontext->UpdateSubresource(m_tex, nSubRes, NULL, pData, srcRowPitch, 0);
				}
				delete[] pData;
			}
		}else
		{
			if(format != PF_R8G8B8)
			{
				pcontext->UpdateSubresource(m_tex, 0, NULL, data, srcRowPitch, 0);
			}else
			{
				char *pData = new char[width*height*4];
				Convert24To32(pData, data, width*height);
				pcontext->UpdateSubresource(m_tex, 0, NULL, pData, srcRowPitch, 0);
				delete[] pData;
			}
		}
	}
	D3D11_SHADER_RESOURCE_VIEW_DESC viewDesc;
	viewDesc.Format = desc.Format;
	if(cube)
	{
		viewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
		viewDesc.TextureCube.MostDetailedMip = 0;
		viewDesc.TextureCube.MipLevels = -1;
	}
	else
	{
		viewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		viewDesc.Texture2D.MostDetailedMip = 0;
		viewDesc.Texture2D.MipLevels = -1;
	}
	if(FAILED(pdev->CreateShaderResourceView(m_tex, &viewDesc, &m_view)))
		return false;
	if(mipmap)
		pcontext->GenerateMips(m_view);
	return true;
}

D3D11RenderTarget::D3D11RenderTarget()
{
	m_rt = true;
	m_depthBuffer = NULL;
	m_depthView = NULL;
	m_cube = false;
	m_rtView = NULL;
}
D3D11RenderTarget::~D3D11RenderTarget()
{
	if(m_depthBuffer != NULL)
	{
		m_depthBuffer->Release();
		m_depthBuffer = NULL;
	}
	if(m_depthView != NULL)
	{
		m_depthView->Release();
		m_depthView = NULL;
	}
	if(m_cube)
	{
		if(m_rtCubeView != NULL)
		{
			for(int i=0; i<6; i++)
				m_rtCubeView[i]->Release();
			delete[] m_rtCubeView;
			m_rtCubeView = NULL;
		}
	}else
	{
		if(m_rtView != NULL)
		{
			m_rtView->Release();
			m_rtView = NULL;
		}
	}
}
bool D3D11RenderTarget::init(ID3D11Device *pdev, ID3D11DeviceContext *pcontext, int width, int height, int format, int depthbit, int stencilbit, bool cube)
{
	if(depthbit > 0 || stencilbit > 0)
	{
		D3D11_TEXTURE2D_DESC desc;
		desc.Width = width;
		desc.Height = height;
		desc.MipLevels = 1;
		desc.ArraySize = 1;
		desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;
		desc.Usage = D3D11_USAGE_DEFAULT;
		desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
		desc.CPUAccessFlags = 0;
		desc.MiscFlags = 0;
		if(SUCCEEDED(pdev->CreateTexture2D(&desc, NULL, &m_depthBuffer)))
		{
			if(FAILED(pdev->CreateDepthStencilView(m_depthBuffer, NULL, &m_depthView)))
			{
				m_depthBuffer->Release();
				m_depthBuffer = NULL;
			}
		}
		if(m_depthBuffer == NULL)
		{
			LOG_PRINT("ERROR: Failed to initialise rendertarget depthbuffer\n");
			return false;
		}
	}
	if(!D3D11Texture::init(pdev, pcontext, width, height, format, NULL, false, cube, true))
		return false;
	if(cube)
	{
		m_rtCubeView = new ID3D11RenderTargetView*[6];
		D3D11_RENDER_TARGET_VIEW_DESC rtViewDesc;
		rtViewDesc.Format = D3DTextureFormat[format];
		rtViewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
		rtViewDesc.Texture2DArray.MipSlice = 0;
		rtViewDesc.Texture2DArray.ArraySize = 1;
		for(int i=0; i<6; i++)
		{
			rtViewDesc.Texture2DArray.FirstArraySlice = i;
			if(FAILED(pdev->CreateRenderTargetView(m_tex, &rtViewDesc, m_rtCubeView+i)))
			{
				for(int j=0; j<i; j++)
					m_rtCubeView[j]->Release();
				delete[] m_rtCubeView;
				m_rtCubeView = NULL;
				return false;
			}
		}
	}else
	{
		return SUCCEEDED(pdev->CreateRenderTargetView(m_tex, NULL, &m_rtView));
	}
	return true;
}

void RenderTexture::updateDynamicRect(int surface, int x, int y, int width, int height, int format, void *data)
{
	D3D11Texture *pthis = static_cast<D3D11Texture*>(this);
	assert((surface==TS_2D && !pthis->m_cube) || (surface!=TS_2D && pthis->m_cube));
	assert(format = pthis->m_format);
	
	ID3D11Device *pdev = NULL;
	pthis->m_tex->GetDevice(&pdev);
	ID3D11DeviceContext *pContext = NULL;
	pdev->GetImmediateContext(&pContext);
	pdev->Release();
	char *pData = NULL;
	if(format == PF_R8G8B8)
	{
		pData = new char[width*height*4];
		Convert24To32(pData, data, width*height);
		data = pData;
	}
	D3D11_BOX box;
	box.left = x;
	box.top = y;
	box.right = x+width;
	box.bottom = y+height;
	box.front = box.back = 0;
	D3D11_MAP mapType = (x==0 && y==0 && width==pthis->m_width && height==pthis->m_height) ? D3D11_MAP_WRITE_DISCARD : D3D11_MAP_WRITE;
	D3D11_MAPPED_SUBRESOURCE mapRes;
	if(SUCCEEDED(pContext->Map(pthis->m_tex, surface-TS_CUBE_X_POS, mapType, 0, &mapRes)))
	{
		int lineStride = width*D3DPixelSize[format];
		for(int i=0; i<height; i++)
			memcpy(i*mapRes.RowPitch + (char*)mapRes.pData, (char*)data + i*lineStride, lineStride);
		pContext->Unmap(pthis->m_tex, 0);
	}
	pContext->Release();
}
void RenderTexture::copyFromMainBuffer(bool mipmap)
{
	D3D11Texture *pthis = static_cast<D3D11Texture*>(this);
	assert(!pthis->m_cube);

	ID3D11Device *pdev = NULL;
	pthis->m_tex->GetDevice(&pdev);
	ID3D11DeviceContext *pContext = NULL;
	pdev->GetImmediateContext(&pContext);
	pdev->Release();
	ID3D11RenderTargetView *rtView;
	pContext->OMGetRenderTargets(1, &rtView, NULL);
	ID3D11Resource *rtRes;
	rtView->GetResource(&rtRes);
	rtView->Release();
	pContext->ResolveSubresource(pthis->m_tex, 0, rtRes, 0, D3DTextureFormat[pthis->m_format]);
	rtRes->Release();
	if(mipmap)
		pContext->GenerateMips(pthis->m_view);
	pContext->Release();
}
void RenderTexture::updateRenderTargetMipmap()
{
	D3D11Texture *pthis = static_cast<D3D11Texture*>(this);
	assert(pthis->isRenderTarget());

	ID3D11Device *pdev = NULL;
	pthis->m_tex->GetDevice(&pdev);
	ID3D11DeviceContext *pContext = NULL;
	pdev->GetImmediateContext(&pContext);
	pdev->Release();
	pContext->GenerateMips(pthis->m_view);
	pContext->Release();
}

int RenderTexture::getWidth() const
{
	return static_cast<const D3D11Texture*>(this)->m_width;
}
int RenderTexture::getHeight() const
{
	return static_cast<const D3D11Texture*>(this)->m_height;
}
int RenderTexture::getPixelFormat() const
{
	return static_cast<const D3D11Texture*>(this)->m_format;
}
bool RenderTexture::isCubeMap() const
{
	return static_cast<const D3D11Texture*>(this)->m_cube;
}
