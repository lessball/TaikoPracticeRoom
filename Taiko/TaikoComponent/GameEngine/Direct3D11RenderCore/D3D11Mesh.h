#ifndef D3D11MESH_H
#define D3D11MESH_H

#include <RenderCore/RenderMesh.h>
#include <d3d11.h>
#include <string>

class D3D11Mesh : public RenderMesh
{
private:
	friend class RenderMesh;
	D3D11_PRIMITIVE_TOPOLOGY m_topology;
	int m_vertexDataStride;
	int m_numVertexElement;
	VertexElement *m_vertexElement;
	ID3D11Buffer *m_vertexBuffer;
	ID3D11Buffer *m_indexBuffer;
	DXGI_FORMAT m_indexFormat;
	int m_numVertex;
	int m_numIndex;
public:
	D3D11Mesh(ID3D11Device *pdev, const VertexElement *vertexLayout, int numVertexElement, const void *vertex, int numVertex, int indexByteSize, const void *index, int numIndex, int meshType);
	~D3D11Mesh();

	D3D11_PRIMITIVE_TOPOLOGY getTopology() const
	{
		return m_topology;
	}
	int getNumVertexElement() const
	{
		return m_numVertexElement;
	}
	const VertexElement *getVertexElement() const
	{
		return m_vertexElement;
	}
	int getVertexStride() const
	{
		return m_vertexDataStride;
	}
	ID3D11Buffer *getVertexBuffer() const
	{
		return m_vertexBuffer;
	}
	int getNumVertex() const
	{
		return m_numVertex;
	}
	ID3D11Buffer *getIndexBuffer() const
	{
		return m_indexBuffer;
	}
	int getNumIndex() const
	{
		return m_numIndex;
	}
	DXGI_FORMAT getIndexFormat() const
	{
		return m_indexFormat;
	}
};

#endif
