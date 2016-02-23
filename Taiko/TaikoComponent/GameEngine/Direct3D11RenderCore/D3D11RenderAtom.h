#ifndef D3D11RENDERATOM_H
#define D3D11RENDERATOM_H

#include <RenderCore/RenderAtom.h>
#include "D3D11RenderCore.h"
#include "D3D11Mesh.h"
#include <d3d11.h>
#include <string>
#include <boost/container/flat_map.hpp>

class D3D11RenderCore;

enum InputElementType
{
	IET_FLOAT1,
	IET_FLOAT2,
	IET_FLOAT3,
	IET_FLOAT4,
	IET_BYTE4,
	IET_WORD2,
	IET_BYTE4_UNORM,
	IET_WORD2_UNORM,
	IET_BYTE4_SNORM,
	IET_WORD2_SNORM,
};
struct InputElement
{
	unsigned char usage;
	unsigned char index;
	unsigned char type;
	unsigned char slot;
};

class D3D11RenderAtom : public RenderAtom
{
private:
	friend class RenderAtom;

	int m_numInputElement;
	InputElement *m_inputElement;

	struct MeshLink
	{
		ID3D11InputLayout *inputLayout;
		int numVertexBuffer;
		ID3D11Buffer **pVertexBuffer;
		UINT *pVertexStride;
		D3D11Mesh *mainMesh;
	};
	struct ParamLink
	{
		int paramIndex; //-1:mesh
		union
		{
			D3D11RenderCore::ShaderParam *targetParam;
			MeshLink *mesh;
		};
	};
	struct TechniqueLink
	{
		ParamLink *paramLink[D3D11RenderCore::TB_NUMBER];
		int numParamLink[D3D11RenderCore::TB_NUMBER];
		TechniqueLink();
		void release();
	};
	typedef boost::container::flat_map<D3D11RenderCore::Technique*, TechniqueLink> LinkMap;
	union
	{
		TechniqueLink *m_uniqueLink;
		LinkMap *m_multiLink;
	};
	RenderParam *m_param;
	int m_numParam;
	D3D11RenderCore::Technique *m_technique;

	TechniqueLink *linkParam(D3D11RenderCore *rc, int branch, D3D11RenderCore::Technique *tech);

public:
	D3D11RenderAtom(D3D11RenderCore *rc, D3D11RenderCore::Technique *tech, const RenderParam *param, int numParam);
	~D3D11RenderAtom();
	void perform(D3D11RenderCore *core, int branch);
};

#endif
