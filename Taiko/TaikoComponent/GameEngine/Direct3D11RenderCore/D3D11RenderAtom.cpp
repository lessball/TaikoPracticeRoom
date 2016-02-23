#include "D3D11RenderAtom.h"
#include <LogPrint.h>
#include <algorithm>
#include <vector>
using namespace std;

static const int TB_NUMBER = D3D11RenderCore::TB_NUMBER;

D3D11RenderAtom::TechniqueLink::TechniqueLink()
{
	for(int i=0; i<D3D11RenderCore::TB_NUMBER; i++)
	{
		paramLink[i] = NULL;
		numParamLink[i] = 0;
	}
}
void D3D11RenderAtom::TechniqueLink::release()
{
	for(int i=0; i<TB_NUMBER; i++)
	{
		if(paramLink[i] == NULL)
			continue;
		for(int j=i+1; j<TB_NUMBER; j++)
		{
			if(paramLink[j] == paramLink[i])
				paramLink[j] = NULL;
		}
		if(numParamLink[i] > 0 && paramLink[i][0].paramIndex == -1 && paramLink[i][0].mesh != NULL)
		{
			delete[] paramLink[i][0].mesh->pVertexBuffer;
			delete[] paramLink[i][0].mesh->pVertexStride;
			delete paramLink[i][0].mesh;
		}
		delete[] paramLink[i];
		paramLink[i] = NULL;
	}
}

D3D11RenderAtom::TechniqueLink *D3D11RenderAtom::linkParam(D3D11RenderCore *rc, int branch, D3D11RenderCore::Technique *tech)
{
	TechniqueLink *techLink = NULL;
	if(m_technique != NULL)
	{
		assert(tech == NULL);
		techLink = m_uniqueLink;
		tech = m_technique;
	}else
	{
		LinkMap::iterator ifind = m_multiLink->find(tech);
		if(ifind != m_multiLink->end())
		{
			techLink = &(ifind->second);
		}
		{
			techLink = &((*m_multiLink)[tech]);
		}
	}
	if(techLink->paramLink[branch] != NULL)
		return techLink;
	for(int i=0; i<TB_NUMBER; i++)
	{
		if(techLink->paramLink[i] != NULL && tech->shader[i] == tech->shader[branch])
		{
			techLink->paramLink[branch] = techLink->paramLink[i];
			techLink->numParamLink[branch] = techLink->numParamLink[i];
			return techLink;
		}
	}

	const unsigned char inputElementType[3][VET_WORD2+1] =
	{
		{IET_FLOAT1, IET_FLOAT2, IET_FLOAT3, IET_FLOAT4, IET_BYTE4, IET_WORD2},
		{IET_FLOAT1, IET_FLOAT2, IET_FLOAT3, IET_FLOAT4, IET_BYTE4_UNORM, IET_WORD2_UNORM},
		{IET_FLOAT1, IET_FLOAT2, IET_FLOAT3, IET_FLOAT4, IET_BYTE4_SNORM, IET_WORD2_SNORM},
	};
	const int inputElementTypeIndex[VE_BINORMAL+1] = {0, 0, 1, 0, 1, 2, 2, 2};

	D3D11Mesh *pMainMesh = NULL;
	vector<D3D11Mesh*> vVertexMesh;
	vector<ParamLink> vParamLink;
	bool bUpdateLayout = m_inputElement == NULL;
	vector<InputElement> vInputElement;
	bool inputElementUse[VE_BINORMAL+1][8];
	if(bUpdateLayout)
	{
		for(int i=0; i<VE_BINORMAL+1; i++)
		{
			for(int j=0; j<8; j++)
				inputElementUse[i][j] = false;
		}
	}
	D3D11RenderCore::ShaderSet *shaderSet = tech->shader[branch];
	for(int i=0; i<m_numParam; i++)
	{
		if(m_param[i].type == PT_MESH)
		{
			D3D11Mesh *pMesh = static_cast<D3D11Mesh*>(m_param[i].mesh);
			if(pMesh->getVertexBuffer() != NULL)
			{
				vVertexMesh.push_back(pMesh);
				if(bUpdateLayout)
				{
					unsigned char slot = (unsigned char)vVertexMesh.size() - 1;
					for(int i=0; i<pMesh->getNumVertexElement(); i++)
					{
						const VertexElement &vele = pMesh->getVertexElement()[i];
						InputElement iele =
						{
							vele.usage,
							vele.index,
							inputElementType[inputElementTypeIndex[vele.usage]][vele.type],
							slot,
						};
						while(inputElementUse[iele.usage][iele.index] && iele.index < 8)
							iele.index++;
						assert(iele.index < 8);
						inputElementUse[iele.usage][iele.index] = true;
						vInputElement.push_back(iele);
					}
				}
			}
			if(pMainMesh == NULL || (pMainMesh->getIndexBuffer() == NULL && pMesh->getIndexBuffer() != NULL))
				pMainMesh = pMesh;
		}else
		{
			D3D11RenderCore::ParamMap::iterator ifind = shaderSet->mapParam.find(m_param[i].name);
			if(ifind != shaderSet->mapParam.end())
			{
				if(ifind->second.type == m_param[i].type
					&& (m_param[i].type == PT_SAMPLER || ifind->second.column == m_param[i].column)
					&& ifind->second.row == m_param[i].row)
					//&& ifind->second.count == m_param[i].count)
				{
					ParamLink tLink = {i, &(ifind->second)};
					vParamLink.push_back(tLink);
				}else
				{
					LOG_PRINT("dismatch param %s\n", m_param[i].name);
				}
			}
		}
	}
	if(vInputElement.size() > 0)
	{
		assert(m_inputElement == NULL);
		m_numInputElement = vInputElement.size();
		m_inputElement = new InputElement[m_numInputElement];
		for(int i=0; i<m_numInputElement; i++)
			m_inputElement[i] = vInputElement[i];
	}
	int paramBegin = pMainMesh != NULL ? 1 : 0;
	int numParamLink = paramBegin + (int)vParamLink.size();
	ParamLink *pParamLink = new ParamLink[numParamLink];
	if(pMainMesh != NULL)
	{
		pParamLink[0].paramIndex = -1;
		MeshLink *pMeshLink = pParamLink[0].mesh = new MeshLink;
		pMeshLink->inputLayout = NULL;
		pMeshLink->numVertexBuffer = (int)vVertexMesh.size();
		pMeshLink->pVertexBuffer = NULL;
		pMeshLink->pVertexStride = NULL;
		pMeshLink->mainMesh = pMainMesh;
		if(pMeshLink->numVertexBuffer > 0)
		{
			pMeshLink->pVertexBuffer = new ID3D11Buffer*[pMeshLink->numVertexBuffer];
			pMeshLink->pVertexStride = new UINT[pMeshLink->numVertexBuffer];
			for(int i=0; i<pMeshLink->numVertexBuffer; i++)
			{
				pMeshLink->pVertexBuffer[i] = vVertexMesh[i]->getVertexBuffer();
				pMeshLink->pVertexStride[i] = (UINT)(vVertexMesh[i]->getVertexStride());
			}
			assert(m_inputElement != NULL);
			pMeshLink->inputLayout = rc->getInputLayout(tech->id, branch, m_numInputElement, m_inputElement);
		}
	}
	for(int i=0; i<(int)vParamLink.size(); i++)
		pParamLink[i+paramBegin] = vParamLink[i];
	techLink->numParamLink[branch] = numParamLink;
	techLink->paramLink[branch] = pParamLink;
	return techLink;
}

D3D11RenderAtom::D3D11RenderAtom(D3D11RenderCore *rc, D3D11RenderCore::Technique *tech, const RenderParam *param, int numParam)
{
	m_numInputElement = 0;
	m_inputElement = NULL;
	m_technique = tech;
	m_param = new RenderParam[numParam];
	memcpy(m_param, param, sizeof(RenderParam) * numParam);
	for(int i=0; i<numParam; i++)
	{
		if(param[i].name != NULL)
		{
			char *str = new char[strlen(param[i].name) + 1];
			strcpy(str, param[i].name);
			m_param[i].name = str;
		}else
		{
			m_param[i].name = NULL;
		}
	}
	m_numParam = numParam;
	if(tech != NULL)
	{
		m_uniqueLink = new TechniqueLink();
		linkParam(rc, 0, NULL);
	}else
	{
		m_multiLink = new LinkMap();
	}
}
D3D11RenderAtom::~D3D11RenderAtom()
{
	delete[] m_inputElement;
	m_numInputElement = 0;
	for(int i=0; i<m_numParam; i++)
		delete[] m_param[i].name;
	delete[] m_param;
	m_param = NULL;
	m_numParam = 0;
	if(m_technique != NULL)
	{
		m_uniqueLink->release();
		delete m_uniqueLink;
	}else
	{
		for(LinkMap::iterator i = m_multiLink->begin(); i != m_multiLink->end(); ++i)
			i->second.release();
		delete m_multiLink;
	}
}

void D3D11RenderAtom::perform(D3D11RenderCore *core, int branch)
{
	TechniqueLink *pTechLink = NULL;
	if(m_technique != NULL)
	{
		core->setTechnique(m_technique);
		pTechLink = linkParam(core, branch, NULL);
	}else
	{
		pTechLink = linkParam(core, branch, core->getCurrentTechnique());
	}
	int numParam = pTechLink->numParamLink[branch];
	ParamLink *pLink = pTechLink->paramLink[branch];
	if(numParam <= 0)
		return;
	for(int i = pLink[0].paramIndex >= 0 ? 0 : 1; i < numParam; i++)
	{
		D3D11RenderCore::ShaderParam *pTarget = pLink[i].targetParam;
		if(pTarget->type != PT_SAMPLER)
		{
			int nrow = pTarget->row * pTarget->count;
			int columnSize = 4 * pTarget->column;
			if(pTarget->pDataV != NULL)
			{
				for(int irow=0; irow<nrow; irow++)
					memcpy(16*irow + (char*)(pTarget->pDataV), pTarget->column*irow + m_param[pLink[i].paramIndex].pfloat, columnSize);
				//memcpy(pTarget->pDataV, m_param[pLink[i].paramIndex].pfloat, pTarget->byteSize);
			}
			if(pTarget->pDirtyV != NULL)
				*(pTarget->pDirtyV) = true;
			if(pTarget->pDataP != NULL)
			{
				for(int irow=0; irow<nrow; irow++)
					memcpy(16*irow + (char*)(pTarget->pDataP), pTarget->column*irow + m_param[pLink[i].paramIndex].pfloat, columnSize);
				//memcpy(pTarget->pDataP, m_param[pLink[i].paramIndex].pfloat, pTarget->byteSize);
			}
			if(pTarget->pDirtyP != NULL)
				*(pTarget->pDirtyP) = true;
		}else
		{
			core->setSampler(pTarget->column, m_param[pLink[i].paramIndex].sampler);
		}
	}
	if(pLink[0].paramIndex == -1)
	{
		core->commitShaderParam();
		ID3D11DeviceContext *pContext = core->getDeviceContext();
		pContext->IASetInputLayout(pLink[0].mesh->inputLayout);
		UINT offsets[16];
		memset(offsets, 0, sizeof(offsets));
		pContext->IASetVertexBuffers(0, pLink[0].mesh->numVertexBuffer, pLink[0].mesh->pVertexBuffer, pLink[0].mesh->pVertexStride, offsets);
		pContext->IASetPrimitiveTopology(pLink[0].mesh->mainMesh->getTopology());
		if(pLink[0].mesh->mainMesh->getIndexBuffer() != NULL)
		{
			pContext->IASetIndexBuffer(pLink[0].mesh->mainMesh->getIndexBuffer(), pLink[0].mesh->mainMesh->getIndexFormat(), 0);
			pContext->DrawIndexed(pLink[0].mesh->mainMesh->getNumIndex(), 0, 0);
		}else
		{
			pContext->Draw(pLink[0].mesh->mainMesh->getNumVertex(), 0);
		}
	}
}
int RenderAtom::getSortGroup() const
{
	const D3D11RenderAtom *pthis = static_cast<const D3D11RenderAtom*>(this);
	return pthis->m_technique != NULL ? pthis->m_technique->id : -1;
}
const char *RenderAtom::getTechniqueName() const
{
	const D3D11RenderAtom *pthis = static_cast<const D3D11RenderAtom*>(this);
	return pthis->m_technique != NULL ? pthis->m_technique->name.c_str() : NULL;
}
int RenderAtom::getTechniqueID() const
{
	const D3D11RenderAtom *pthis = static_cast<const D3D11RenderAtom*>(this);
	return pthis->m_technique != NULL ? pthis->m_technique->id : -1;
}
