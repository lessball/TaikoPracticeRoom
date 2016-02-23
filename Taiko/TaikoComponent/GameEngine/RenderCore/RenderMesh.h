#ifndef RENDERMESH_H
#define RENDERMESH_H

#include <assert.h>

/**
 * 定义渲染用的mesh
 */

/** 定义顶点数据的使用类型 */
enum VertexElementUsage
{
	VE_POSITION,
	VE_TEXCOORD,
	VE_COLOR,
	VE_BONEINDEX,
	VE_BONEWEIGHT,
	VE_NORMAL,
	VE_TANGENT,
	VE_BINORMAL,
};
/** 定义顶点数据的数据类型 */
enum VertexElementDataType
{
	VET_FLOAT1,
	VET_FLOAT2,
	VET_FLOAT3,
	VET_FLOAT4,
	VET_BYTE4,
	VET_WORD2,
};
/** 声明一个顶点数据。通过VertexElement的数组来描述顶点的完整数据结构 */
struct VertexElement
{
	unsigned char usage; //使用类型。VertexElementUsage枚举值
	unsigned char index; //索引
	unsigned char type; //数据类型。VertexElementDataType枚举值
};
/** 定义mesh类型 */
enum MeshType
{
	MT_TRIANGLE_LIST,
	MT_TRIANGLE_STRIP,
	MT_LINE_LIST,
	MT_LINE_STRIP,
};
/**
 * 调用RenderCore中的相关方法创建或销毁RenderMesh对象
 */
class RenderMesh
{
public:
    /**
     * 更新动态顶点缓冲的数据
     * start 开始位置索引
     * count 更新的顶点数量
     * newNumVertex 新的顶点总数。负数表示不变
     * data 更新的顶点数据
     */
	void updateVertex(int start, int count, int newNumVertex, const void *data);
    /**
     * 更新动态索引缓冲的数据
     * start 开始位置索引
     * count 更新的索引数量
     * newNumIndex 新的索引总数。负数表示不变
     * data 更新的索引数据
     */
	void updateIndex(int start, int count, int newNumIndex, const void *data);
};

#endif
