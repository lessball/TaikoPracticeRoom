#include "D3D11Mesh.h"
#include <assert.h>

D3D11Mesh::D3D11Mesh(ID3D11Device *pdev, const VertexElement *vertexLayout, int numVertexElement, const void *vertex, int numVertex, int indexByteSize, const void *index, int numIndex, int meshType)
{
	D3D11_PRIMITIVE_TOPOLOGY D3DTopology[] =
	{
		D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST,
		D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP,
		D3D11_PRIMITIVE_TOPOLOGY_LINELIST,
		D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP,
	};
	m_topology = D3DTopology[meshType];
	m_vertexBuffer = NULL;
	m_numVertex = 0;
	m_indexBuffer = NULL;
	m_numIndex = 0;
	const int VERTEX_USAGE_MAX = VE_BINORMAL+1;
	const int VERTEX_DATA_TYPE_MAX = VET_WORD2+1;
	const int AttrSize[] = {4, 8, 12, 16, 4, 4};
	if(vertexLayout != NULL && numVertexElement > 0)
	{
		m_numVertexElement = numVertexElement;
		m_vertexElement = new VertexElement[numVertexElement];
		memcpy(m_vertexElement ,vertexLayout, sizeof(VertexElement)*numVertexElement);
		m_vertexDataStride = 0;
		for(int i=0; i<numVertexElement; i++)
		{
			assert(vertexLayout[i].usage>=0 && vertexLayout[i].usage<VERTEX_USAGE_MAX && vertexLayout[i].index>=0 && vertexLayout[i].index<8 && vertexLayout[i].type>=0 && vertexLayout[i].type<VERTEX_DATA_TYPE_MAX);
			m_vertexDataStride += AttrSize[vertexLayout[i].type];
		}
		if(numVertex > 0)
		{
			if(vertex != NULL)
			{
				D3D11_BUFFER_DESC desc = {m_vertexDataStride*numVertex, D3D11_USAGE_IMMUTABLE, D3D11_BIND_VERTEX_BUFFER, 0, 0, 0};
				D3D11_SUBRESOURCE_DATA initData = {vertex, 0, 0};
				if(FAILED(pdev->CreateBuffer(&desc, &initData, &m_vertexBuffer)))
					return;
			}else
			{
				D3D11_BUFFER_DESC desc = {m_vertexDataStride*numVertex, D3D11_USAGE_DYNAMIC, D3D11_BIND_VERTEX_BUFFER, D3D11_CPU_ACCESS_WRITE, 0, 0};
				if(FAILED(pdev->CreateBuffer(&desc, NULL, &m_vertexBuffer)))
					return;
			}
		}
	}
	m_numVertex = numVertex;
	if(numIndex > 0)
	{
		assert(indexByteSize == 2 || indexByteSize == 4);
		m_indexFormat = indexByteSize == 2 ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;
		if(index != NULL)
		{
			D3D11_BUFFER_DESC desc = {indexByteSize*numIndex, D3D11_USAGE_IMMUTABLE, D3D11_BIND_INDEX_BUFFER, 0, 0, 0};
			D3D11_SUBRESOURCE_DATA initData = {index, 0, 0};
			if(FAILED(pdev->CreateBuffer(&desc, &initData, &m_indexBuffer)))
				return;
		}else
		{
			D3D11_BUFFER_DESC desc = {indexByteSize*numIndex, D3D11_USAGE_DYNAMIC, D3D11_BIND_INDEX_BUFFER, D3D11_CPU_ACCESS_WRITE, 0, 0};
			if(FAILED(pdev->CreateBuffer(&desc, NULL, &m_indexBuffer)))
				return;
		}
	}
	m_numIndex = numIndex;
}
D3D11Mesh::~D3D11Mesh()
{
	if(m_vertexElement != NULL)
	{
		delete[] m_vertexElement;
		m_vertexElement = NULL;
	}
	if(m_vertexBuffer != NULL)
	{
		m_vertexBuffer->Release();
		m_vertexBuffer = NULL;
	}
	if(m_indexBuffer != NULL)
	{
		m_indexBuffer->Release();
		m_indexBuffer = NULL;
	}
}
void RenderMesh::updateVertex(int start, int count, int newNumVertex, const void *data)
{
	D3D11Mesh *pthis = static_cast<D3D11Mesh*>(this);
	assert(pthis->m_vertexBuffer != NULL);
	if(newNumVertex > 0)
		pthis->m_numVertex = newNumVertex;
	ID3D11Device *pdev = NULL;
	pthis->m_vertexBuffer->GetDevice(&pdev);
	ID3D11DeviceContext *pContext = NULL;
	pdev->GetImmediateContext(&pContext);
	pdev->Release();
	D3D11_MAPPED_SUBRESOURCE mapRes;
	D3D11_MAP mapType = (start==0 && count==newNumVertex) ? D3D11_MAP_WRITE_DISCARD : D3D11_MAP_WRITE;
	if(SUCCEEDED(pContext->Map(pthis->m_vertexBuffer, 0, mapType, 0, &mapRes)))
	{
		memcpy(mapRes.pData, data, count*pthis->m_vertexDataStride);
		pContext->Unmap(pthis->m_vertexBuffer, 0);
	}
	pContext->Release();
}
void RenderMesh::updateIndex(int start, int count, int newNumIndex, const void *data)
{
	D3D11Mesh *pthis = static_cast<D3D11Mesh*>(this);
	assert(pthis->m_indexBuffer != NULL);
	if(newNumIndex > 0)
		pthis->m_numIndex = newNumIndex;
	ID3D11Device *pdev = NULL;
	pthis->m_indexBuffer->GetDevice(&pdev);
	ID3D11DeviceContext *pContext = NULL;
	pdev->GetImmediateContext(&pContext);
	pdev->Release();
	D3D11_MAPPED_SUBRESOURCE mapRes;
	D3D11_MAP mapType = (start==0 && count==newNumIndex) ? D3D11_MAP_WRITE_DISCARD : D3D11_MAP_WRITE;
	if(SUCCEEDED(pContext->Map(pthis->m_indexBuffer, 0, mapType, 0, &mapRes)))
	{
		memcpy(mapRes.pData, data, count*(pthis->m_indexFormat == DXGI_FORMAT_R16_UINT ? 2 : 4));
		pContext->Unmap(pthis->m_indexBuffer, 0);
	}
	pContext->Release();
}
