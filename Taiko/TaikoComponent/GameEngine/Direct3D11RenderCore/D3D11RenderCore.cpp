#include "D3D11RenderCore.h"
#include "D3D11RenderAtom.h"
#include "D3D11Texture.h"

#include <FileResource/FileResource.h>
#include <FileResource/MemoryFileReader.h>
#include <LogPrint.h>

#include <DirectXMath.h>

#include <map>
#include <set>
#include <assert.h>

#if !WINAPI_FAMILY_PARTITION( WINAPI_PARTITION_PHONE ) && !defined(NDEBUG)
#include <Initguid.h>
#include <DXGIDebug.h>
#endif

using namespace std;
using namespace rapidxml;

struct LoaderShaderParam
{
	unsigned char type;
	unsigned char column;
	unsigned char row;
	unsigned char count;
	int iCB;
	int offset;
};

template<typename TShader>
struct LoaderShaderBranch
{
	TShader *shader;
	map<string, LoaderShaderParam> mapParam;
};

template<typename TShader>
TShader *createShader(ID3D11Device *pdev, void *byteCode, int byteCodeSize);
template<>
D3D11RenderCore::VertexShader *createShader<D3D11RenderCore::VertexShader>(ID3D11Device *pdev, void *byteCode, int byteCodeSize)
{
	D3D11RenderCore::VertexShader *vs = new D3D11RenderCore::VertexShader;
	if(FAILED(pdev->CreateVertexShader(byteCode, byteCodeSize, NULL, &vs->d3dshader)))
	{
		delete vs;
		return NULL;
	}
	vs->byteCode = new char[byteCodeSize];
	memcpy(vs->byteCode, byteCode, byteCodeSize);
	vs->byteCodeSize = byteCodeSize;
	return vs;
}
template<>
D3D11RenderCore::PixelShader *createShader<D3D11RenderCore::PixelShader>(ID3D11Device *pdev, void *byteCode, int byteCodeSize)
{
	D3D11RenderCore::PixelShader *ps = new D3D11RenderCore::PixelShader;
	if(FAILED(pdev->CreatePixelShader(byteCode, byteCodeSize, NULL, &ps->d3dshader)))
	{
		delete ps;
		return NULL;
	}
	return ps;
}

template<typename TShader>
struct LoaderShader
{
	LoaderShaderBranch<TShader> shader[D3D11RenderCore::TB_NUMBER];
};

template<typename TShader>
static LoaderShader<TShader> *createShader(ID3D11Device *pdev, const char *name, map<string, LoaderShader<TShader>*> &shadermap, const std::string &basepath)
{
	string sname(name);
	map<string, LoaderShader<TShader>*>::iterator ifind = shadermap.find(sname);
	if(ifind != shadermap.end())
		return ifind->second;
	FileResource *fileRes = FileResource::open((basepath+sname).c_str());
	if(fileRes == NULL)
		return NULL;
	char *fileData = fileRes->readAll();
	int fileSize = fileRes->size();
	FileResource::close(fileRes);
	fileRes = NULL;
	
	MemoryFileReader file(fileData, fileSize);
	int magic;
	if(!file.readData(&magic) || magic != ('P' | ('R'<<8) | ('S'<<16) | ('D'<<24)))
	{
		delete[] fileData;
		return NULL;
	}
	int flag;
	if(!file.readData(&flag))
	{
		delete[] fileData;
		return NULL;
	}
	bool bAlphaTest = (flag & 1) != 0;
	bool bClipPlane = (flag & 2) != 0;
	LoaderShader<TShader> *tShader = new LoaderShader<TShader>();
	for(int i=0; i<D3D11RenderCore::TB_NUMBER; i++)
	{
		tShader->shader[i].shader = NULL;
	}
	bool bError = false;
	for(int i=0; i<D3D11RenderCore::TB_NUMBER; i++)
	{
		if((i & 1) != 0 && !bAlphaTest)
		{
			tShader->shader[i].shader = tShader->shader[i-1].shader;
			continue;
		}else if((i & 2) != 0 && !bClipPlane)
		{
			tShader->shader[i].shader = tShader->shader[i-2].shader;
			continue;
		}
		int shaderDataSize;
		void *shaderData;
		if(!file.readData(&shaderDataSize) || (shaderData = file.readData(shaderDataSize)) == NULL)
		{
			bError = true;
			break;
		}
		TShader *pRCShader = createShader<TShader>(pdev, shaderData, shaderDataSize);
		if(pRCShader == NULL || !file.readData(&pRCShader->numCB))
		{
			bError = true;
			break;
		}
		tShader->shader[i].shader = pRCShader;
		pRCShader->pShadowBuffer = NULL;
		pRCShader->pDirty = NULL;
		pRCShader->pD3DCB = NULL;
		pRCShader->pShadowBuffer = new char*[pRCShader->numCB];
		pRCShader->pD3DCB = new ID3D11Buffer*[pRCShader->numCB];
		pRCShader->pDirty = new bool[pRCShader->numCB];
		for(int icb=0; icb<pRCShader->numCB; icb++)
		{
			pRCShader->pShadowBuffer[icb] = NULL;
			pRCShader->pD3DCB[icb] = NULL;
		}
		for(int icb=0; icb<pRCShader->numCB; icb++)
		{
			int CBSize;
			if(!file.readData(&CBSize))
			{
				bError = true;
				break;
			}
			pRCShader->pShadowBuffer[icb] = CBSize > 0 ? new char[CBSize] : NULL;
			D3D11_BUFFER_DESC bufferDesc = {CBSize, D3D11_USAGE_DYNAMIC, D3D11_BIND_CONSTANT_BUFFER, D3D11_CPU_ACCESS_WRITE, 0, 0};
			if(FAILED(pdev->CreateBuffer(&bufferDesc, NULL, pRCShader->pD3DCB+icb)))
			{
				bError = true;
				break;
			}
		}
		if(bError)
			break;
		int numParam;
		if(!file.readData(&numParam))
		{
			bError = true;
			break;
		}
		for(int ipar=0; ipar<numParam; ipar++)
		{
			char *paramName = file.readStringAligned();
			if(paramName == NULL)
			{
				bError = true;
				break;
			}
			if(!file.readData(&tShader->shader[i].mapParam[paramName]))
			{
				delete[] paramName;
				bError = true;
				break;
			}
			delete[] paramName;
		}
		if(bError)
			break;
	}
	delete[] fileData;
	if(bError)
	{
		for(int i=0; i<D3D11RenderCore::TB_NUMBER; i++)
		{
			if((i & 1) != 0 && !bAlphaTest)
			{
				continue;
			}else if((i & 2) != 0 && !bClipPlane)
			{
				continue;
			}
			TShader *pRCShader = tShader->shader[i].shader;
			if(pRCShader != NULL)
			{
				pRCShader->release();
				delete pRCShader;
			}
		}
		delete tShader;
		return NULL;
	}
	shadermap[name] = tShader;
	return tShader;
}

void D3D11RenderCore::Shader::release()
{
	for(int icb=0; icb<numCB; icb++)
	{
		if(pShadowBuffer != NULL && pShadowBuffer[icb] != NULL)
		{
			delete[] pShadowBuffer[icb];
			pShadowBuffer[icb] = NULL;
		}
		if(pD3DCB != NULL && pD3DCB[icb] != NULL)
		{
			pD3DCB[icb]->Release();
			pD3DCB[icb] = NULL;
		}
	}
	delete[] pShadowBuffer;
	pShadowBuffer = NULL;
	delete[] pD3DCB;
	pD3DCB = NULL;
	delete[] pDirty;
	pDirty = NULL;
}
void D3D11RenderCore::VertexShader::release()
{
	Shader::release();
	delete[] byteCode;
	byteCode = NULL;
	byteCodeSize = 0;
	if(d3dshader != NULL)
	{
		d3dshader->Release();
		d3dshader = NULL;
	}
}
void D3D11RenderCore::PixelShader::release()
{
	Shader::release();
	if(d3dshader != NULL)
	{
		d3dshader->Release();
		d3dshader = NULL;
	}
}

D3D11RenderCore::D3D11RenderCore()
{
	m_device = NULL;
	m_context = NULL;
#if !WINAPI_FAMILY_PARTITION( WINAPI_PARTITION_PHONE )
	m_swapChain = NULL;
#else
	m_rtTexture = NULL;
	m_aaTexture = NULL;
	m_scissorScale[0] = m_scissorScale[1] = 1.0f;
#endif
	m_rtView = NULL;
	m_dsTexture = NULL;
	m_dsView = NULL;
	m_currentRTView = NULL;
	m_currentDSView = NULL;
	m_currentTechnique = NULL;
	m_techniqueBranch = 0;
	m_stateAtom = NULL;
	m_techniqueDirty = false;
	m_stateDirty = false;
	m_currentBlendState = NULL;
	m_currentDepthStencilState = NULL;
	m_currentStencilRef = 0;
	for(int i=0; i<6; i++)
		m_rasterizerState[i] = NULL;
	m_currentRasterizerState = -1;
	for(int i=0; i<(int)(sizeof(m_textureView)/sizeof(m_textureView[0])); i++)
		m_textureView[i] = NULL;
	for(int i=0; i<(int)(sizeof(m_samplerState)/sizeof(m_samplerState[0])); i++)
		m_samplerState[i] = NULL;
	m_numTexture = 0;
	for(int i=0; i<(int)(sizeof(m_clipPlane)/sizeof(m_clipPlane[0])); i++)
		m_clipPlane[i] = 0.0f;
	m_alphaTestRange[0] = -1.0f;
	m_alphaTestRange[1] = 2.0f;
}

D3D11RenderCore::~D3D11RenderCore()
{
	assert(m_device == NULL);
}

bool D3D11RenderCore::initDevice(const boost::any &window, int colorbit, int depthbit, int stencilbit, int multisample, int swapInterval)
{
#ifndef NDEBUG
	const UINT deviceFlag = D3D11_CREATE_DEVICE_DEBUG;
#else
	const UINT deviceFlag = 0;
#endif

	int width = 0;
	int height = 0;

#if !WINAPI_FAMILY_PARTITION( WINAPI_PARTITION_PHONE )
	const HWND *pwindow = boost::any_cast<HWND>(&window);
	if(pwindow == NULL)
		return false;
	RECT clientRect;
	GetClientRect(*pwindow, &clientRect);
	width = clientRect.right - clientRect.left;
	height = clientRect.bottom - clientRect.top;
	DXGI_SWAP_CHAIN_DESC desc;
	desc.BufferDesc.Width = width;
	desc.BufferDesc.Height = height;
	desc.BufferDesc.RefreshRate.Numerator = 60;
	desc.BufferDesc.RefreshRate.Denominator = 0;
	desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	desc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
	desc.SampleDesc.Count = multisample;
	desc.SampleDesc.Quality =  D3D11_STANDARD_MULTISAMPLE_PATTERN;
	desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	desc.BufferCount = 1;
	desc.OutputWindow = *pwindow;
	desc.Windowed = TRUE;
	desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
	desc.Flags = 0; //DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH
	D3D_FEATURE_LEVEL featureLevel;
	if(FAILED(D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, deviceFlag, NULL, 0, D3D11_SDK_VERSION, &desc, &m_swapChain, &m_device, &featureLevel, &m_context)))
		return false;
	m_swapInterval = swapInterval;

	if(depthbit > 0 || stencilbit > 0)
	{
		D3D11_TEXTURE2D_DESC dsDesc;
		dsDesc.Width = width;
		dsDesc.Height = height;
		dsDesc.MipLevels = 1;
		dsDesc.ArraySize = 1;
		dsDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		dsDesc.SampleDesc.Count = multisample;
		dsDesc.SampleDesc.Quality = D3D11_STANDARD_MULTISAMPLE_PATTERN;
		dsDesc.Usage = D3D11_USAGE_DEFAULT;
		dsDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
		dsDesc.CPUAccessFlags = 0;
		dsDesc.MiscFlags = 0;
		if(!resetBackBuffer(&dsDesc))
			return false;
	}else if(!resetBackBuffer(NULL))
	{
		return false;
	}

	D3D11_VIEWPORT vp;
	vp.Width = (float)width;
	vp.Height = (float)height;
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	vp.TopLeftX = 0.0f;
	vp.TopLeftY = 0.0f;
	m_context->RSSetViewports(1, &vp);
#else
	ID3D11Device *const*ppDevice = boost::any_cast<ID3D11Device*>(&window);
	if(ppDevice == NULL)
		return false;
	m_device = *ppDevice;

	m_aaDesc.Width = 9;
	m_aaDesc.Height = 9;
	m_aaDesc.MipLevels = 1;
	m_aaDesc.ArraySize = 1;
	m_aaDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	m_aaDesc.SampleDesc.Count = multisample;
	m_aaDesc.SampleDesc.Quality = 0;
	m_aaDesc.Usage = D3D11_USAGE_DEFAULT;
	m_aaDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
	m_aaDesc.CPUAccessFlags =  0;
	m_aaDesc.MiscFlags = 0;
	
	m_dsDesc.Width = width;
	m_dsDesc.Height = height;
	m_dsDesc.MipLevels = 1;
	m_dsDesc.ArraySize = 1;
	m_dsDesc.Format = depthbit > 0 || stencilbit > 0 ? DXGI_FORMAT_D24_UNORM_S8_UINT : DXGI_FORMAT_UNKNOWN;
	m_dsDesc.SampleDesc.Count = multisample;
	m_dsDesc.SampleDesc.Quality = 0;
	m_dsDesc.Usage = D3D11_USAGE_DEFAULT;
	m_dsDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	m_dsDesc.CPUAccessFlags = 0;
	m_dsDesc.MiscFlags = 0;
	
	m_currentRasterizerState = 0;
#endif

	D3D11_RASTERIZER_DESC rdesc;
	rdesc.FillMode = D3D11_FILL_SOLID;
	rdesc.FrontCounterClockwise = FALSE;
	rdesc.DepthBias = 0;
	rdesc.DepthBiasClamp = 0.0f;
	rdesc.SlopeScaledDepthBias = 0.0f;
	rdesc.DepthClipEnable = TRUE;
	rdesc.MultisampleEnable = FALSE;
	rdesc.AntialiasedLineEnable = FALSE;
	const D3D11_CULL_MODE d3dCull[] = {D3D11_CULL_FRONT, D3D11_CULL_BACK, D3D11_CULL_NONE};
	for(int i=0; i<6; i++)
	{
		rdesc.CullMode = d3dCull[i%3];
		rdesc.ScissorEnable = i<3 ? FALSE : TRUE;
		m_device->CreateRasterizerState(&rdesc, m_rasterizerState+i);
	}
	m_currentRasterizerState = 0;
	if(m_context != NULL)
		m_context->RSSetState(m_rasterizerState[0]);

	RenderParam stateParam[] =
	{
		{"g_clipPlane0", {m_clipPlane}, PT_FLOAT, 4, 1, 1},
		{"g_clipPlane1", {m_clipPlane+4}, PT_FLOAT, 4, 1, 1},
		{"g_clipPlane2", {m_clipPlane+8}, PT_FLOAT, 4, 1, 1},
		{"g_clipPlane3", {m_clipPlane+16}, PT_FLOAT, 4, 1, 1},
		{"g_alphaTestRange", {m_alphaTestRange}, PT_FLOAT, 2, 1, 1},
	};
	m_stateAtom = createRenderAtom(NULL, stateParam, (int)(sizeof(stateParam)/sizeof(stateParam[0])));
	return true;
}
void D3D11RenderCore::releaseDevice()
{
	if(m_stateAtom != NULL)
	{
		destroyRenderAtom(m_stateAtom);
		m_stateAtom = NULL;
	}
	for(int i=0; i<6; i++)
	{
		if(m_rasterizerState[i] != NULL)
		{
			m_rasterizerState[i]->Release();
			m_rasterizerState[i] = NULL;
		}
	}
	if(m_dsView != NULL)
	{
		m_dsView->Release();
		m_dsView = NULL;
	}
	if(m_dsTexture != NULL)
	{
		m_dsTexture->Release();
		m_dsTexture = NULL;
	}
#if !WINAPI_FAMILY_PARTITION( WINAPI_PARTITION_PHONE )
	if(m_rtView != NULL)
	{
		m_rtView->Release();
		m_rtView = NULL;
	}
	if(m_swapChain != NULL)
	{
		m_swapChain->Release();
		m_swapChain = NULL;
	}
	if(m_context != NULL)
	{
		m_context->ClearState();
		m_context->Release();
		m_context = NULL;
	}
	if(m_device != NULL)
	{
		m_device->Release();
		m_device = NULL;
	}
#ifndef NDEBUG
	HMODULE hdxgi = GetModuleHandle("Dxgidebug.dll");
	typedef HRESULT (WINAPI *PDXGIGET)(REFIID riid, void **ppDebug);
	PDXGIGET pDXGIGet = NULL;
	if(hdxgi != NULL && (pDXGIGet = (PDXGIGET)GetProcAddress(hdxgi, "DXGIGetDebugInterface")) != NULL)
	{
		IDXGIDebug *dxgiDebug = NULL;
		pDXGIGet(__uuidof(IDXGIDebug), (void**)&dxgiDebug);
		if(dxgiDebug != NULL)
			dxgiDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_DETAIL);
	}
#endif
#else
	m_rtView = NULL;
	m_context = NULL;
	m_device = NULL;
	if(m_aaTexture != NULL)
	{
		if(m_rtView != NULL)
		{
			m_rtView->Release();
			m_rtView = NULL;
		}
		m_aaTexture->Release();
		m_aaTexture = NULL;
	}
#endif
}

bool D3D11RenderCore::loadShader(const rapidxml::xml_node<> *desc, const std::string &basepath)
{
	map<string, LoaderShader<VertexShader>*> vsmap;
	map<string, LoaderShader<PixelShader>*> psmap;
	for(rapidxml::xml_node<> *xtech = desc->first_node("technique"); xtech != NULL; xtech=xtech->next_sibling("technique"))
	{
		xml_attribute<> *xname = xtech->first_attribute("name");
		xml_attribute<> *xvs = xtech->first_attribute("vertexshader");
		xml_attribute<> *xps = xtech->first_attribute("pixelshader");
		if(xname==NULL || xvs==NULL || xps==NULL)
			continue;
		const char *name = xname->value();
		const char *svs = xvs->value();
		const char *sps = xps->value();
		LoaderShader<VertexShader> *vs = createShader(m_device, svs, vsmap, basepath);
		if(vs == NULL)
		{
			LOG_PRINT("Failed to create vertex shader :%s\n", svs);
			continue;
		}
		LoaderShader<PixelShader> *ps = createShader(m_device, sps, psmap, basepath);
		if(ps == NULL)
		{
			LOG_PRINT("Failed to create vertex shader :%s\n", svs);
			continue;
		}
		Technique tech;
		tech.name = name;
		for(int i=0; i<TB_NUMBER; i++)
			tech.shader[i] = NULL;
		for(int i=0; i<TB_NUMBER; i++)
		{
			if((i & 1) != 0 && vs->shader[i].shader == vs->shader[i-1].shader && ps->shader[i].shader == ps->shader[i-1].shader)
			{
				tech.shader[i] = tech.shader[i-1];
				continue;
			}else if((i & 2) != 0 && vs->shader[i].shader == vs->shader[i-2].shader && ps->shader[i].shader == ps->shader[i-2].shader)
			{
				tech.shader[i] = tech.shader[i-2];
				continue;
			}
			tech.shader[i] = new ShaderSet;
			tech.shader[i]->vs = vs->shader[i].shader;
			tech.shader[i]->ps = ps->shader[i].shader;
			for(map<string, LoaderShaderParam>::iterator ipar = vs->shader[i].mapParam.begin(); ipar != vs->shader[i].mapParam.end(); ++ipar)
			{
				ShaderParam &tpar = tech.shader[i]->mapParam[ipar->first];
				tpar.name = ipar->first;
				tpar.type = ipar->second.type;
				tpar.column = ipar->second.column;
				tpar.row = ipar->second.row;
				tpar.count = ipar->second.count;
				if(ipar->second.type != PT_SAMPLER)
				{
					//tpar.byteSize = 4*tpar.column*tpar.row*tpar.count;
					tpar.pDataV = vs->shader[i].shader->pShadowBuffer[ipar->second.iCB] + ipar->second.offset;
					tpar.pDirtyV = vs->shader[i].shader->pDirty + ipar->second.iCB;
				}else
				{
					//tpar.byteSize = ipar->second.iCB;
					tpar.column = ipar->second.iCB;
					tpar.pDataV = NULL;
					tpar.pDirtyV = NULL;
				}
				tpar.pDataP = NULL;
				tpar.pDirtyP = NULL;
			}
			for(map<string, LoaderShaderParam>::iterator ipar = ps->shader[i].mapParam.begin(); ipar != ps->shader[i].mapParam.end();  ++ipar)
			{
				ShaderParam &tpar = tech.shader[i]->mapParam[ipar->first];
				if(tpar.name.empty())
				{
					tpar.name = ipar->first;
					tpar.type = ipar->second.type;
					tpar.column = ipar->second.column;
					tpar.row = ipar->second.row;
					tpar.count = ipar->second.count;
					tpar.pDataV = NULL;
					tpar.pDirtyV = NULL;
				}else if(tpar.type != ipar->second.type || tpar.column != ipar->second.column || tpar.row != ipar->second.row || tpar.count != ipar->second.count)
				{
					continue;
				}
				if(ipar->second.type != PT_SAMPLER)
				{
					//tpar.byteSize = 4*tpar.column*tpar.row*tpar.count;
					tpar.pDataP = ps->shader[i].shader->pShadowBuffer[ipar->second.iCB] + ipar->second.offset;
					tpar.pDirtyP = ps->shader[i].shader->pDirty + ipar->second.iCB;
				}else
				{
					//tpar.byteSize = ipar->second.iCB;
					tpar.column = ipar->second.iCB;
					tpar.pDataP = NULL;
					tpar.pDirtyP = NULL;
				}
			}
		}
		tech.id = (int)m_mapTechnique.size();
		m_mapTechnique[name] = tech;
	}
	int techId = 0;
	for(TechniqueMap::iterator i = m_mapTechnique.begin(); i != m_mapTechnique.end(); ++i)
	{
		i->second.id = techId++;
	}
	for(map<string, LoaderShader<VertexShader>*>::iterator iv = vsmap.begin(); iv != vsmap.end(); ++iv)
		delete iv->second;
	for(map<string, LoaderShader<PixelShader>*>::iterator ip = psmap.begin(); ip != psmap.end(); ++ip)
		delete ip->second;
	return true;
}

void D3D11RenderCore::releaseShaderResource()
{
	set<VertexShader*> vsset;
	set<PixelShader*> psset;
	for(TechniqueMap::iterator i = m_mapTechnique.begin(); i != m_mapTechnique.end(); ++i)
	{
		for(int ibranch=0; ibranch<TB_NUMBER; ibranch++)
		{
			ShaderSet *pShaderSet = i->second.shader[ibranch];
			if(pShaderSet == NULL)
				continue;
			for(int j=ibranch+1; j<TB_NUMBER; j++)
			{
				if(i->second.shader[j] == pShaderSet)
					i->second.shader[j] = NULL;
			}
			if(vsset.find(pShaderSet->vs) == vsset.end())
			{
				vsset.insert(pShaderSet->vs);
				pShaderSet->vs->release();
				delete pShaderSet->vs;
				pShaderSet->vs = NULL;
			}
			if(psset.find(pShaderSet->ps) == psset.end())
			{
				psset.insert(pShaderSet->ps);
				pShaderSet->ps->release();
				delete pShaderSet->ps;
				pShaderSet->ps = NULL;
			}
			pShaderSet->mapParam.clear();
			delete pShaderSet;
		}
	}
	m_mapTechnique.clear();

	for(BlendStateMap::iterator i = m_mapBlendState.begin(); i != m_mapBlendState.end(); ++i)
	{
		if(i->second != NULL)
			delete i->second;
	}
	m_mapBlendState.clear();
	for(DepthStencilStateMap::iterator i = m_mapDepthStencilState.begin(); i != m_mapDepthStencilState.end(); ++i)
	{
		if(i->second != NULL)
			delete i->second;
	}
	m_mapDepthStencilState.clear();

	for(InputLayoutMap::iterator i = m_mapInputLayout.begin(); i != m_mapInputLayout.end(); ++i)
	{
		if(i->second != NULL)
		{
			i->second->Release();
			i->second = NULL;
		}
	}
	m_mapInputLayout.clear();

	for(SamplerStateMap::iterator i = m_mapSamplerState.begin(); i != m_mapSamplerState.end(); ++i)
	{
		if(i->second != NULL)
		{
			i->second->Release();
			i->second = NULL;
		}
	}
	m_mapSamplerState.clear();
}

ID3D11InputLayout *D3D11RenderCore::getInputLayout(int tech, int branch, int numElement, const InputElement *pElement)
{
	assert(tech >= 0 && tech < 256 && tech < (int)m_mapTechnique.size() && branch >= 0  && branch < TB_NUMBER);
	string key;
	key += (char)(tech & 0xff);
	key += (char)(branch & 0xff);
	key.append((char*)pElement, (char*)(pElement+numElement));
	InputLayoutMap::iterator ifind = m_mapInputLayout.find(key);
	if(ifind != m_mapInputLayout.end())
		return ifind->second;
	
	const int VERTEX_USAGE_MAX = VE_BINORMAL+1;
	const char *semanticName[VERTEX_USAGE_MAX] = {"POSITION", "TEXCOORD", "COLOR", "BLENDINDICES", "BLENDWEIGHT", "NORMAL", "TANGENT", "BINORMAL"};
	const DXGI_FORMAT dxgiFormat[IET_WORD2_SNORM+1] =
	{
		DXGI_FORMAT_R32_FLOAT,
		DXGI_FORMAT_R32G32_FLOAT, 
		DXGI_FORMAT_R32G32B32_FLOAT, 
		DXGI_FORMAT_R32G32B32A32_FLOAT, 
		DXGI_FORMAT_R8G8B8A8_UINT,
		DXGI_FORMAT_R16G16_UINT,
		DXGI_FORMAT_R8G8B8A8_UNORM,
		DXGI_FORMAT_R16G16_UNORM,
		DXGI_FORMAT_R8G8B8A8_SNORM,
		DXGI_FORMAT_R16G16_SNORM,
	};

	vector<D3D11_INPUT_ELEMENT_DESC> vele;
	for(int i=0; i<numElement; i++)
	{
		D3D11_INPUT_ELEMENT_DESC eleDesc =
		{
			semanticName[pElement[i].usage],
			pElement[i].index,
			dxgiFormat[pElement[i].type],
			pElement[i].slot,
			D3D11_APPEND_ALIGNED_ELEMENT,
			D3D11_INPUT_PER_VERTEX_DATA,
			0
		};
		vele.push_back(eleDesc);
	}
	ID3D11InputLayout *d3dil = NULL;
	VertexShader *vs = (m_mapTechnique.begin() + tech)->second.shader[branch]->vs;
	m_device->CreateInputLayout(&(vele[0]), vele.size(), vs->byteCode, vs->byteCodeSize, &d3dil);
	m_mapInputLayout[key] = d3dil;
	return d3dil;
}

void D3D11RenderCore::setSampler(int index, RenderSampler *sampler)
{
	assert(index >= 0 && index < 8 && sampler != NULL);
	if(index >= m_numTexture)
		m_numTexture = index+1;
	m_textureView[index] = ((D3D11Texture*)(sampler->texture))->getSRV();
	int samplerKey = (sampler->filter) | (sampler->anisotropy<<8) | (sampler->wrapx<<8) | (sampler->wrapy<<8);
	SamplerStateMap::iterator ifind = m_mapSamplerState.find(samplerKey);
	if(ifind != m_mapSamplerState.end())
	{
		m_samplerState[index] = ifind->second;
	}else
	{
		D3D11_SAMPLER_DESC desc;
		if(sampler->anisotropy > 0)
		{
			desc.Filter = D3D11_FILTER_ANISOTROPIC;
		}else
		{
			const D3D11_FILTER d3d11Filter[] =
			{
				D3D11_FILTER_MIN_MAG_MIP_POINT,
				D3D11_FILTER_MIN_LINEAR_MAG_MIP_POINT,
				D3D11_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT,
				D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT,
				D3D11_FILTER_MIN_MAG_MIP_POINT,
				D3D11_FILTER_MIN_LINEAR_MAG_MIP_POINT,
				D3D11_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT,
				D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT,
				//mip linear
				D3D11_FILTER_MIN_MAG_POINT_MIP_LINEAR,
				D3D11_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR,
				D3D11_FILTER_MIN_POINT_MAG_MIP_LINEAR,
				D3D11_FILTER_MIN_MAG_MIP_LINEAR,
				D3D11_FILTER_MIN_MAG_POINT_MIP_LINEAR,
				D3D11_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR,
				D3D11_FILTER_MIN_POINT_MAG_MIP_LINEAR,
				D3D11_FILTER_MIN_MAG_MIP_LINEAR,
			};
			desc.Filter = d3d11Filter[sampler->filter];
		}
		D3D11_TEXTURE_ADDRESS_MODE d3d11Wrap[] =
		{
			D3D11_TEXTURE_ADDRESS_WRAP,
			D3D11_TEXTURE_ADDRESS_CLAMP,
			D3D11_TEXTURE_ADDRESS_MIRROR,
		};
		desc.AddressU = d3d11Wrap[sampler->wrapx];
		desc.AddressV = d3d11Wrap[sampler->wrapy];
		desc.AddressW = d3d11Wrap[0];
		desc.MipLODBias = 0;
		desc.MaxAnisotropy = sampler->anisotropy;
		desc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
		desc.BorderColor[0] = desc.BorderColor[1] = desc.BorderColor[2] = desc.BorderColor[3] = 0.0f;
		desc.MinLOD = 0;
		desc.MaxLOD = D3D11_FLOAT32_MAX;
		ID3D11SamplerState *pSamplerState = NULL;
		if(SUCCEEDED(m_device->CreateSamplerState(&desc, &pSamplerState)))
		{
			m_mapSamplerState[samplerKey] = pSamplerState;
			m_samplerState[index] = pSamplerState;
		}
	}
}
void D3D11RenderCore::commitShaderParam()
{
	assert(m_currentTechnique != NULL);
	Shader *shader[2] = {m_currentTechnique->shader[m_techniqueBranch]->vs, m_currentTechnique->shader[m_techniqueBranch]->ps};
	for(int i=0; i<2; i++)
	{
		for(int icb=0; icb<shader[i]->numCB; icb++)
		{
			if(shader[i]->pDirty[icb])
			{
				D3D11_MAPPED_SUBRESOURCE pRes;
				if(SUCCEEDED(m_context->Map(shader[i]->pD3DCB[icb], 0, D3D11_MAP_WRITE_DISCARD, 0, &pRes)))
				{
					memcpy(pRes.pData, shader[i]->pShadowBuffer[icb], pRes.RowPitch);
					m_context->Unmap(shader[i]->pD3DCB[icb], 0);
				}
				shader[i]->pDirty[icb] = false;
			}
		}
	}
	if(m_numTexture > 0)
	{
		m_context->PSSetShaderResources(0, m_numTexture, m_textureView);
		m_context->PSSetSamplers(0, m_numTexture, m_samplerState);
		m_numTexture = 0;
		for(int i=0; i<8; i++)
		{
			m_textureView[i] = NULL;
			m_samplerState[i] = NULL;
		}
	}
	m_context->VSSetConstantBuffers(0, shader[0]->numCB, shader[0]->pD3DCB);
	m_context->PSSetConstantBuffers(0, shader[1]->numCB, shader[1]->pD3DCB);
}

void D3D11RenderCore::setTechnique(Technique *tech)
{
	if(tech != m_currentTechnique || m_techniqueDirty)
	{
		if(tech != NULL)
		{
			m_context->VSSetShader(tech->shader[m_techniqueBranch]->vs->d3dshader, NULL, 0);
			m_context->PSSetShader(tech->shader[m_techniqueBranch]->ps->d3dshader, NULL, 0);
		}
		m_currentTechnique = tech;
		m_techniqueDirty = false;
	}
	if(m_stateDirty)
	{
		accept(m_stateAtom);
		m_stateDirty = false;
	}
}

RenderCore *RenderCore::createRenderCore(const boost::any &window, int colorbit, int depthbit, int stencilbit, int multisample, int swapInterval)
{
	D3D11RenderCore *rc = new D3D11RenderCore();
	if(!rc->initDevice(window, colorbit, depthbit, stencilbit, multisample, swapInterval))
	{
		rc->releaseDevice();
		delete rc;
		return NULL;
	}
	return rc;
}
void RenderCore::destroyRenderCore(RenderCore *core)
{
	assert(core != NULL);
	D3D11RenderCore *prc = static_cast<D3D11RenderCore*>(core);
	prc->releaseDevice();
	delete prc;
}

bool RenderCore::init(const rapidxml::xml_node<> *desc, const char *basepath)
{
	D3D11RenderCore *pthis = static_cast<D3D11RenderCore*>(this);
	return pthis->loadShader(desc, basepath);
}
void RenderCore::release()
{
	D3D11RenderCore *pthis = static_cast<D3D11RenderCore*>(this);
	pthis->releaseShaderResource();
}

void RenderCore::begin()
{
}
void RenderCore::end()
{
	D3D11RenderCore *pthis = static_cast<D3D11RenderCore*>(this);
#if !WINAPI_FAMILY_PARTITION( WINAPI_PARTITION_PHONE )
	pthis->m_swapChain->Present(pthis->m_swapInterval, 0);
#else
	if(pthis->m_aaTexture != NULL)
	{
		pthis->m_context->ResolveSubresource(pthis->m_rtTexture, 0, pthis->m_aaTexture, 0, DXGI_FORMAT_UNKNOWN); //TODO format ?
	}
#endif
}

void RenderCore::matrixPerspective(float *target, float zdx, float zdy, float zNear, float zFar, bool renderToTexture)
{
	target[0] = zdx;
	target[1] = 0.0f;
	target[2] = 0.0f;
	target[3] = 0.0f;
	target[4] = 0.0f;
	target[5] = zdy;
	target[6] = 0.0f;
	target[7] = 0.0f;
	target[8] = 0.0f;
	target[9] = 0.0f;
	target[10] = zFar/(zFar-zNear);
	target[11] = -zNear*zFar/(zFar-zNear);
	target[12] = 0.0f;
	target[13] = 0.0f;
	target[14] = 1.0f;
	target[15] = 0.0f;
}
void RenderCore::matrixOrthographic(float *target, float width, float height, float zNear,  float zFar, bool renderToTexture)
{
	target[0] = 2.0f/width;
	target[1] = 0.0f;
	target[2] = 0.0f;
	target[3] = 0.0f;
	target[4] = 0.0f;
	target[5] = 2.0f/height;
	target[6] = 0.0f;
	target[7] = 0.0f;
	target[8] = 0.0f;
	target[9] = 0.0f;
	target[10] = 1.0f/(zFar-zNear);
	target[11] = -zNear/(zFar-zNear);
	target[12] = 0.0f;
	target[13] = 0.0f;
	target[14] = 0.0f;
	target[15] = 1.0f;
}
bool RenderCore::isFilpRenderToTexture()
{
	return false;
}

void RenderCore::setMainViewSize(int w, int h)
{
#if !WINAPI_FAMILY_PARTITION( WINAPI_PARTITION_PHONE )
	D3D11RenderCore *pthis = static_cast<D3D11RenderCore*>(this);
	DXGI_SWAP_CHAIN_DESC desc;
	pthis->m_swapChain->GetDesc(&desc);
	if(desc.BufferDesc.Width == w && desc.BufferDesc.Height == h)
		return;
	pthis->m_context->OMSetRenderTargets(0, NULL, NULL);
	if(pthis->m_rtView != NULL)
	{
		pthis->m_rtView->Release();
		pthis->m_rtView = NULL;
	}
	if(pthis->m_dsView != NULL)
	{
		pthis->m_dsView->Release();
		pthis->m_dsView = NULL;
	}
	D3D11_TEXTURE2D_DESC dsDesc;
	dsDesc.Width = 0;
	if(pthis->m_dsTexture != NULL)
	{
		pthis->m_dsTexture->GetDesc(&dsDesc);
		dsDesc.Width = w;
		dsDesc.Height = h;
		pthis->m_dsTexture->Release();
		pthis->m_dsTexture = NULL;
	}
	if(SUCCEEDED(pthis->m_swapChain->ResizeBuffers(1, w, h, DXGI_FORMAT_UNKNOWN, 0)))
	{
		pthis->resetBackBuffer(dsDesc.Width > 0 ? &dsDesc : NULL);
	}else
	{
		LOG_PRINT("error: ResizeBuffers failed");
	}
#endif
}
void RenderCore::getMainViewSize(int &w, int &h)
{
	D3D11RenderCore *pthis = static_cast<D3D11RenderCore*>(this);
#if !WINAPI_FAMILY_PARTITION( WINAPI_PARTITION_PHONE )
	DXGI_SWAP_CHAIN_DESC desc;
	pthis->m_swapChain->GetDesc(&desc);
	w = desc.BufferDesc.Width;
	h = desc.BufferDesc.Height;
#else
	D3D11_TEXTURE2D_DESC desc;
	pthis->m_rtTexture->GetDesc(&desc);
	w = desc.Width;
	h = desc.Height;
#endif
}

void RenderCore::setViewPort(int left, int top, int right, int bottom)
{
	D3D11RenderCore *pthis = static_cast<D3D11RenderCore*>(this);
	D3D11_VIEWPORT vp;
	vp.Width = (float)(right-left);
	vp.Height = (float)(bottom-top);
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	vp.TopLeftX = (float)left;
	vp.TopLeftY = (float)top;
	pthis->m_context->RSSetViewports(1, &vp);
}
void RenderCore::getViewPort(int &left, int &top, int &right, int &bottom)
{
	D3D11RenderCore *pthis = static_cast<D3D11RenderCore*>(this);
	D3D11_VIEWPORT vp;
	UINT nvp = 1;
	pthis->m_context->RSGetViewports(&nvp, &vp);
	left = (int)vp.TopLeftX;
	top = (int)vp.TopLeftY;
	right = (int)(vp.TopLeftX + vp.Width);
	bottom = (int)(vp.TopLeftY + vp.Height);
}

void RenderCore::setAlphaTest(bool enable, int mode, int key)
{
	D3D11RenderCore *pthis = static_cast<D3D11RenderCore*>(this);
	if(enable != (pthis->m_techniqueBranch & D3D11RenderCore::TB_ALPHATEST) != 0)
	{
		pthis->m_techniqueBranch ^= D3D11RenderCore::TB_ALPHATEST;
		pthis->m_techniqueDirty = true;
	}
	switch(mode)
	{
	case COMP_NEVER:
		pthis->m_alphaTestRange[1] = -1.0f;
		break;
	case COMP_ALWAYS:
		pthis->m_alphaTestRange[0] = -1.0f;
		pthis->m_alphaTestRange[1] = 2.0f;
		break;
	case COMP_LESS:
		pthis->m_alphaTestRange[0] = -1.0f;
		pthis->m_alphaTestRange[1] = key / 255.0f;
		break;
	case COMP_GREATER:
		pthis->m_alphaTestRange[0] = key / 255.0f;
		pthis->m_alphaTestRange[1] = 2.0f;
		break;
	case COMP_LESS_EQUAL:
		pthis->m_alphaTestRange[0] = -1.0f;
		pthis->m_alphaTestRange[1] = (key+0.1f) / 255.0f;
		break;
	case COMP_GREATER_EQUAL:
		pthis->m_alphaTestRange[0] = (key-0.1f) / 255.0f;
		pthis->m_alphaTestRange[1] = 2.0f;
		break;
	case COMP_EQUAL:
		pthis->m_alphaTestRange[0] = (key-0.1f) / 255.0f;
		pthis->m_alphaTestRange[1] = (key+0.1f) / 255.0f;
		break;
	case COMP_NOT_EQUAL:
		LOG_PRINT("warning: unsupported alpha test op !=\n");
		break;
	}
	pthis->m_stateDirty = true;
}

void RenderCore::setClipPlane(int count, const float *planes, const float *viewProjMatrix)
{
	D3D11RenderCore *pthis = static_cast<D3D11RenderCore*>(this);
	assert(count < (int)(sizeof(pthis->m_clipPlane)/sizeof(float)/4));
	if(count > 0)
	{
		assert(count <= 4 && planes != NULL && viewProjMatrix != NULL);
		DirectX::XMMATRIX xvpmat(viewProjMatrix[0], viewProjMatrix[4], viewProjMatrix[8], viewProjMatrix[12],
			viewProjMatrix[1], viewProjMatrix[5], viewProjMatrix[9], viewProjMatrix[13],
			viewProjMatrix[2], viewProjMatrix[6], viewProjMatrix[10], viewProjMatrix[14],
			viewProjMatrix[3], viewProjMatrix[7], viewProjMatrix[11], viewProjMatrix[15]);
		DirectX::XMMATRIX xitmat = XMMatrixTranspose(XMMatrixInverse(NULL, xvpmat));
		for(int i=0; i<count; i++)
		{
			DirectX::XMVECTOR xplane = DirectX::XMPlaneTransform(DirectX::XMVectorSet(planes[i*4], planes[i*4+1], planes[i*4+2], planes[i*4+3]), xitmat);
			DirectX::XMFLOAT4 tf4;
			DirectX::XMStoreFloat4(&tf4, xplane);
			pthis->m_clipPlane[i*4] = tf4.x;
			pthis->m_clipPlane[i*4+1] = tf4.y;
			pthis->m_clipPlane[i*4+2] = tf4.z;
			pthis->m_clipPlane[i*4+3] = tf4.w;
		}
		for(int i=count; i<4; i++)
		{
			for(int j=0; j<4; j++)
				pthis->m_clipPlane[i*4+j] = 0.0f;
		}
		memcpy(pthis->m_clipPlane, planes, count*4*sizeof(float));
		pthis->m_stateDirty = true;
	}
	if((count > 0) != ((pthis->m_techniqueBranch & D3D11RenderCore::TB_CLIPPLANE) != 0))
	{
		pthis->m_techniqueBranch ^= D3D11RenderCore::TB_CLIPPLANE;
		pthis->m_stateDirty = true;
	}
}

void RenderCore::setCullMode(int mode)
{
	D3D11RenderCore *pthis = static_cast<D3D11RenderCore*>(this);
	if(pthis->m_currentRasterizerState % 3 != mode)
	{
		pthis->m_currentRasterizerState = pthis->m_currentRasterizerState / 3 * 3 + mode;
		pthis->m_context->RSSetState(pthis->m_rasterizerState[pthis->m_currentRasterizerState]);
	}
}
int RenderCore::getCullMode()
{
	D3D11RenderCore *pthis = static_cast<D3D11RenderCore*>(this);
	return pthis->m_currentRasterizerState % 3;
}
void RenderCore::setScissorTest(bool enable, int left, int top, int right, int bottom)
{
	D3D11RenderCore *pthis = static_cast<D3D11RenderCore*>(this);
	if(enable != (pthis->m_currentRasterizerState >= 3))
	{
		if(enable)
		{
			pthis->m_currentRasterizerState = (pthis->m_currentRasterizerState % 3) + 3;
			pthis->m_context->RSSetState(pthis->m_rasterizerState[pthis->m_currentRasterizerState]);
			D3D11_RECT d3drect;
			d3drect.left = left;
			d3drect.top = top;
			d3drect.right = right;
			d3drect.bottom = bottom;
#if WINAPI_FAMILY_PARTITION( WINAPI_PARTITION_PHONE )
			if(pthis->m_currentRTView == pthis->m_rtView)
			{
				d3drect.left = (int)(0.5f + pthis->m_scissorScale[0]*left);
				d3drect.top = (int)(0.5f + pthis->m_scissorScale[0]*top);
				d3drect.right = (int)(0.5f + pthis->m_scissorScale[0]*right);
				d3drect.bottom = (int)(0.5f + pthis->m_scissorScale[0]*bottom);
			}
#endif
			pthis->m_context->RSSetScissorRects(1, &d3drect);
		}else
		{
			pthis->m_currentRasterizerState = pthis->m_currentRasterizerState % 3;
			pthis->m_context->RSSetState(pthis->m_rasterizerState[pthis->m_currentRasterizerState]);
		}
	}
}
bool RenderCore::isScissorTestEnable()
{
	D3D11RenderCore *pthis = static_cast<D3D11RenderCore*>(this);
	return pthis->m_currentRasterizerState >= 3;
}

RenderBlendState *RenderCore::createBlendState(const BlendStateDesc *desc)
{
	D3D11RenderCore *pthis = static_cast<D3D11RenderCore*>(this);
	int key = D3D11BlendState::getKey(*desc);
	D3D11RenderCore::BlendStateMap::iterator ifind = pthis->m_mapBlendState.find(key);
	if(ifind != pthis->m_mapBlendState.end())
		return ifind->second;
	D3D11BlendState *state = new D3D11BlendState(*desc, pthis->m_device);
	if(state->getD3DBlendState() == NULL)
	{
		delete state;
		return NULL;
	}
	pthis->m_mapBlendState[key] = state;
	return state;
}
void RenderCore::destroyBlendState(RenderBlendState *state)
{
	return;
}
void RenderCore::setBlendState(RenderBlendState *state, const float *blendColor)
{
	D3D11RenderCore *pthis = static_cast<D3D11RenderCore*>(this);
	D3D11BlendState *pstate = static_cast<D3D11BlendState*>(state);
	if(pthis->m_currentBlendState != state || blendColor != NULL)
	{
		const float defaultColor[] = {0.0f,0.0f,0.0f,0.0f};
		pthis->m_context->OMSetBlendState(pstate->getD3DBlendState(), blendColor != NULL ? blendColor : defaultColor, 0xffffffff);
		pthis->m_currentBlendState = pstate;
	}
}
const RenderBlendState *RenderCore::getCurrentBlendState() const
{
	const D3D11RenderCore *pthis = static_cast<const D3D11RenderCore*>(this);
	return pthis->m_currentBlendState;
}

RenderDepthStencilState *RenderCore::createDepthStencilState(const DepthStencilStateDesc *desc)
{
	D3D11RenderCore *pthis = static_cast<D3D11RenderCore*>(this);
	long long key = D3D11DepthStencilState::getKey(*desc);
	D3D11RenderCore::DepthStencilStateMap::iterator ifind = pthis->m_mapDepthStencilState.find(key);
	if(ifind != pthis->m_mapDepthStencilState.end())
		return ifind->second;
	D3D11DepthStencilState *state = new D3D11DepthStencilState(*desc, pthis->m_device);
	if(state->getD3DDepthStencilState() == NULL)
	{
		delete state;
		return NULL;
	}
	pthis->m_mapDepthStencilState[key] = state;
	return state;
}
void RenderCore::destroyDepthStencilState(RenderDepthStencilState *state)
{
	return;
}
void RenderCore::setDepthStencilState(RenderDepthStencilState *state, int stencilRef)
{
	D3D11RenderCore *pthis = static_cast<D3D11RenderCore*>(this);
	D3D11DepthStencilState *pstate = static_cast<D3D11DepthStencilState*>(state);
	if(state != pthis->m_currentDepthStencilState || stencilRef != pthis->m_currentStencilRef)
	{
		pthis->m_context->OMSetDepthStencilState(pstate->getD3DDepthStencilState(), stencilRef);
		pthis->m_currentDepthStencilState = pstate;
		pthis->m_currentStencilRef = stencilRef;
	}
}
const RenderDepthStencilState *RenderCore::getCurrentDepthStencilState() const
{
	const D3D11RenderCore *pthis = static_cast<const D3D11RenderCore*>(this);
	return pthis->m_currentDepthStencilState;
}

void RenderCore::clear(bool clearColor, float red, float green, float blue, float alpha, bool clearDepth, float depth, bool clearStencil, int stencil)
{
	D3D11RenderCore *pthis = static_cast<D3D11RenderCore*>(this);
	if(clearColor)
	{
		float clearColor[] = {red, green, blue, alpha};
		pthis->m_context->ClearRenderTargetView(pthis->m_currentRTView, clearColor);
	}
	UINT clearFlag = (clearDepth ? D3D11_CLEAR_DEPTH : 0) | (clearStencil ? D3D11_CLEAR_STENCIL : 0);
	if(clearFlag != 0)
		pthis->m_context->ClearDepthStencilView(pthis->m_currentDSView, clearFlag, depth, (UINT8)stencil);
}

void RenderCore::accept(RenderAtom *atom)
{
	D3D11RenderCore *pthis = static_cast<D3D11RenderCore*>(this);
	D3D11RenderAtom *patom = static_cast<D3D11RenderAtom*>(atom);
	patom->perform(pthis, pthis->m_techniqueBranch);
}

RenderMesh *RenderCore::createMesh(const VertexElement *vertexLayout, int numVertexElement, const void *vertex, int numVertex, int indexByteSize, const void *index, int numIndex, int meshType)
{
	D3D11RenderCore *pthis = static_cast<D3D11RenderCore*>(this);
	return new D3D11Mesh(pthis->m_device, vertexLayout, numVertexElement, vertex, numVertex, indexByteSize, index, numIndex, meshType);
}
void RenderCore::destroyMesh(RenderMesh *mesh)
{
	delete static_cast<D3D11Mesh*>(mesh);
}
RenderTexture *RenderCore::createTexture(int width, int height, int format, const void *data, bool mipmap, bool renderTarget, int depthbit, int stencilbit)
{
	D3D11RenderCore *pthis = static_cast<D3D11RenderCore*>(this);
	if(renderTarget)
	{
		D3D11RenderTarget *tex = new D3D11RenderTarget();
		if(tex->init(pthis->m_device, pthis->m_context, width, height, format, depthbit, stencilbit, false))
			return tex;
		delete tex;
		return NULL;
	}else
	{
		D3D11Texture *tex = new D3D11Texture();
		if(tex->init(pthis->m_device, pthis->m_context, width, height, format, data, mipmap, false, false))
			return tex;
		delete tex;
		return NULL;
	}
}
RenderTexture *RenderCore::createCubeTexture(int size, int format, const void **data, bool mipmap, bool renderTarget, int depthbit, int stencilbit)
{
	D3D11RenderCore *pthis = static_cast<D3D11RenderCore*>(this);
	if(renderTarget)
	{
		D3D11RenderTarget *tex = new D3D11RenderTarget();
		if(tex->init(pthis->m_device, pthis->m_context, size, size, format, depthbit, stencilbit, true))
			return tex;
		delete tex;
		return NULL;
	}else
	{
		D3D11Texture *tex = new D3D11Texture();
		if(tex->init(pthis->m_device, pthis->m_context, size, size, format, data, mipmap, true, false))
			return tex;
		delete tex;
		return NULL;
	}
}
void RenderCore::destroyTexture(RenderTexture *texture)
{
	D3D11Texture *ptex = static_cast<D3D11Texture*>(texture);
	if(ptex->isRenderTarget())
	{
		delete static_cast<D3D11RenderTarget*>(ptex);
	}else
	{
		delete ptex;
	}
}
void RenderCore::bindRendeTarget(RenderTexture *rt, int surface)
{
	D3D11RenderCore *pthis = static_cast<D3D11RenderCore*>(this);
	if(rt != NULL)
	{
		assert((static_cast<D3D11Texture*>(rt))->isRenderTarget());
		D3D11RenderTarget *pRT = static_cast<D3D11RenderTarget*>(rt);
		pthis->m_currentRTView = pRT->getRTView(surface);
		pthis->m_currentDSView = pRT->getDSView();
	}else
	{
		pthis->m_currentRTView = pthis->m_rtView;
		pthis->m_currentDSView = pthis->m_dsView;
	}
	pthis->m_context->OMSetRenderTargets(1,  &(pthis->m_currentRTView), pthis->m_currentDSView);
}

RenderAtom *RenderCore::createRenderAtom(const char *technique, const RenderParam *param, int numParam)
{
	D3D11RenderCore *pthis = static_cast<D3D11RenderCore*>(this);
	if(technique != NULL)
	{
		D3D11RenderCore::TechniqueMap::iterator ifind = pthis->m_mapTechnique.find(technique);
		if(ifind == pthis->m_mapTechnique.end())
		{
			LOG_PRINT("no technique: %s\n", technique);
			return NULL;
		}
		return new D3D11RenderAtom(pthis, &(ifind->second), param, numParam);
	}else
	{
		return new D3D11RenderAtom(pthis, NULL, param, numParam);
	}
}
void RenderCore::destroyRenderAtom(RenderAtom *atom)
{
	delete static_cast<D3D11RenderAtom*>(atom);
}


#if !WINAPI_FAMILY_PARTITION( WINAPI_PARTITION_PHONE )
bool D3D11RenderCore::resetBackBuffer(D3D11_TEXTURE2D_DESC *depthStencilDesc)
{
	assert(m_rtView == NULL && m_dsTexture == NULL && m_dsView == NULL);
	ID3D11Texture2D *pBackBuffer = NULL;
	if(FAILED(m_swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&pBackBuffer)))
		return false;
	if(FAILED(m_device->CreateRenderTargetView(pBackBuffer, NULL, &m_rtView)))
		return false;
	pBackBuffer->Release();
	if(depthStencilDesc != NULL)
	{
		if(FAILED(m_device->CreateTexture2D(depthStencilDesc, NULL, &m_dsTexture))
			|| FAILED(m_device->CreateDepthStencilView(m_dsTexture, NULL, &m_dsView)))
			return false;
	}
	m_context->OMSetRenderTargets(1, &m_rtView, m_dsView);
	m_currentRTView = m_rtView;
	m_currentDSView = m_dsView;
	return true;
}

#else

void D3D11RenderCore::updateContext(ID3D11DeviceContext *context, ID3D11RenderTargetView *view)
{
	ID3D11Resource *rtResource = NULL;
	view->GetResource(&rtResource);
	if(rtResource == NULL)
		return;
	if(FAILED(rtResource->QueryInterface(__uuidof(ID3D11Texture2D), (void**)&m_rtTexture)))
		return;
	rtResource->Release();
	D3D11_TEXTURE2D_DESC rtDesc;
	m_rtTexture->GetDesc(&rtDesc);
	m_rtView = NULL;
	if(m_aaDesc.SampleDesc.Count > 1 && (m_aaDesc.Width != rtDesc.Width || m_aaDesc.Height != rtDesc.Height))
	{
		if(m_aaTexture != NULL)
		{
			m_aaTexture->Release();
			m_aaTexture = NULL;
		}
		m_aaDesc.Width = rtDesc.Width;
		m_aaDesc.Height = rtDesc.Height;
		if(FAILED(m_device->CreateTexture2D(&rtDesc, NULL, &m_aaTexture)))
			return;
		if(m_rtView != NULL)
		{
			m_rtView->Release();
			m_rtView = NULL;
		}
		if(FAILED(m_device->CreateRenderTargetView(m_aaTexture, NULL, &m_rtView)))
		{
			m_aaTexture->Release();
			m_aaTexture = NULL;
		}
	}
	m_context = context;
	if(m_rtView == NULL)
		m_rtView = view;

	if(m_dsDesc.Format != DXGI_FORMAT_UNKNOWN)
	{
		if(m_dsDesc.Width != rtDesc.Width || m_dsDesc.Height != rtDesc.Height)
		{
			if(m_dsView != NULL)
			{
				m_dsView->Release();
				m_dsView = NULL;
			}
			if(m_dsTexture != NULL)
			{
				m_dsTexture->Release();
				m_dsTexture = NULL;
			}
			m_dsDesc.Width = rtDesc.Width;
			m_dsDesc.Height = rtDesc.Height;
		}
		if(m_dsTexture == NULL && FAILED(m_device->CreateTexture2D(&m_dsDesc, NULL, &m_dsTexture)))
			return;
		if(m_dsView == NULL && FAILED(m_device->CreateDepthStencilView(m_dsTexture, NULL, &m_dsView)))
			return;
	}
	m_context->OMSetRenderTargets(1, &m_rtView, m_dsView);
	m_currentRTView = m_rtView;
	m_currentDSView = m_dsView;

	D3D11_VIEWPORT vp;
	vp.Width = (float)(rtDesc.Width);
	vp.Height = (float)(rtDesc.Height);
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	vp.TopLeftX = 0.0f;
	vp.TopLeftY = 0.0f;
	m_context->RSSetViewports(1, &vp);
	
	m_techniqueDirty = true;
	m_stateDirty = true;
	m_currentBlendState = NULL;
	m_currentDepthStencilState = NULL;
	m_context->RSSetState(m_rasterizerState[m_currentRasterizerState]);
}
#endif
