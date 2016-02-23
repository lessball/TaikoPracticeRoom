#ifndef D3D11RENDERCORE_H
#define D3D11RENDERCORE_H

#include <RenderCore/RenderCore.h>
#include "D3D11RenderState.h"
#include <d3d11.h>
#include <string>
#include <boost/container/flat_map.hpp>
#include <unordered_map>

struct InputElement;

class D3D11RenderCore : public RenderCore
{
public:
	enum TechniqueBranchMask
	{
		TB_ALPHATEST = 1,
		TB_CLIPPLANE = 2,
		TB_NUMBER = 4,
	};
	struct Shader
	{
		int numCB;
		char **pShadowBuffer;
		bool *pDirty;
		ID3D11Buffer **pD3DCB;
		void release();
	};
	struct VertexShader : public Shader
	{
		ID3D11VertexShader *d3dshader;
		char *byteCode;
		int byteCodeSize;
		void release();
	};
	struct PixelShader : public Shader
	{
		ID3D11PixelShader *d3dshader;
		void init(ID3D11Device *pdev, char *byteCode, int byteCodeSize);
		void release();
	};
	struct ShaderParam
	{
		std::string name;
		unsigned char type;
		unsigned char column; // or texture index
		unsigned char row;
		unsigned char count;
		//int byteSize; // or texture index
		bool *pDirtyV;
		void *pDataV;
		bool *pDirtyP;
		void *pDataP;
		bool operator < (const ShaderParam &other) const
		{
			return name < other.name;
		}
	};
	typedef boost::container::flat_map<std::string, ShaderParam> ParamMap;
	struct ShaderSet
	{
		VertexShader *vs;
		PixelShader *ps;
		ParamMap mapParam;
	};
	struct Technique
	{
		std::string name;
		ShaderSet *shader[TB_NUMBER];
		int id;
	};

private:
	friend class RenderCore;

	ID3D11Device *m_device;
	ID3D11DeviceContext *m_context;
#if !WINAPI_FAMILY_PARTITION( WINAPI_PARTITION_PHONE )
	IDXGISwapChain *m_swapChain;
	int m_swapInterval;
#else
	ID3D11Texture2D *m_rtTexture;
	ID3D11Texture2D *m_aaTexture;
	D3D11_TEXTURE2D_DESC m_aaDesc;
	D3D11_TEXTURE2D_DESC m_dsDesc;
	float m_scissorScale[2];
#endif
	ID3D11RenderTargetView *m_rtView;
	ID3D11Texture2D *m_dsTexture;
	ID3D11DepthStencilView *m_dsView;
	ID3D11RenderTargetView *m_currentRTView;
	ID3D11DepthStencilView *m_currentDSView;

	typedef boost::container::flat_map<std::string, Technique> TechniqueMap;
	TechniqueMap m_mapTechnique;

	Technique *m_currentTechnique;
	int m_techniqueBranch;
	RenderAtom *m_stateAtom;
	bool m_techniqueDirty;
	bool m_stateDirty;

	typedef std::unordered_map<int, D3D11BlendState*> BlendStateMap;
	BlendStateMap m_mapBlendState;
	D3D11BlendState *m_currentBlendState;

	typedef std::unordered_map<long long, D3D11DepthStencilState*> DepthStencilStateMap;
	DepthStencilStateMap m_mapDepthStencilState;
	D3D11DepthStencilState *m_currentDepthStencilState;
	int m_currentStencilRef;

	ID3D11RasterizerState *m_rasterizerState[6];
	int m_currentRasterizerState;

	typedef std::unordered_map<std::string, ID3D11InputLayout*> InputLayoutMap; // technique; branch; vertex layout;
	InputLayoutMap m_mapInputLayout;

	typedef std::unordered_map<int, ID3D11SamplerState*> SamplerStateMap;
	SamplerStateMap m_mapSamplerState;
	ID3D11ShaderResourceView *m_textureView[8];
	ID3D11SamplerState *m_samplerState[8];
	int m_numTexture;

	float m_clipPlane[16]; // max 4 clipPlane
	float m_alphaTestRange[2];
public:
	D3D11RenderCore();
	~D3D11RenderCore();

	bool initDevice(const boost::any &window, int colorbit, int depthbit, int stencilbit, int multisample, int swapInterval);
	void releaseDevice();

	bool loadShader(const rapidxml::xml_node<> *desc, const std::string &basepath);
	void releaseShaderResource();

	ID3D11Device *getDevice()
	{
		return m_device;
	}
	ID3D11DeviceContext *getDeviceContext()
	{
		return m_context;
	}

	ID3D11InputLayout *getInputLayout(int tech, int branch, int numElement, const InputElement *pElement);

	void setSampler(int index, RenderSampler *sampler);
	void commitShaderParam();

	void setTechnique(Technique *tech);
	Technique *getCurrentTechnique()
	{
		return m_currentTechnique;
	}
	
#if !WINAPI_FAMILY_PARTITION( WINAPI_PARTITION_PHONE )
	bool resetBackBuffer(D3D11_TEXTURE2D_DESC *depthStencilDesc);
#else
	void updateContext(ID3D11DeviceContext *context, ID3D11RenderTargetView *view);
	void setScissorScale(float scaleX, float scaleY)
	{
		m_scissorScale[0] = scaleX;
		m_scissorScale[1] = scaleY;
	}
#endif
	
};

#endif
