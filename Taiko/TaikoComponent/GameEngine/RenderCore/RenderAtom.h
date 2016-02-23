#ifndef RENDERATOM_H
#define RENDERATOM_H

#include <RenderCore/RenderMesh.h>
#include <RenderCore/RenderTexture.h>

/**
 * 将technique、mesh、参数设置等封装为渲染操作
 */

/** 定义参数类型 */
enum ParamType
{
	PT_FLOAT,
	PT_INT,
	PT_MESH,
	PT_SAMPLER,
};
/**
 * 表示一个渲染参数
 * 数据指针的生命周期需要在外部维护
 */
struct RenderParam
{
	const char *name;
	union
	{
		float *pfloat;
		int *pint;
		RenderMesh *mesh;
		RenderSampler *sampler;
	};
	unsigned char type;
	unsigned char column; //size 1D
	unsigned char row; //size 2D
	unsigned char count;
};
/**
 * class RenderAtom
 * 表示一个渲染操作，对应一个technique和/或一组参数集合。
 * technique表示渲染技术，在可编程管线中对应一组shader
 * 如果没有指定technique，则可以作为通用参数，在使用时动态与当前technique链接
 * 参数中如果有mesh则进行绘制。可以包含多个mesh，将作为一个模型的不同分量合并绘制，当顶点数据位置重复时,后续的顶点数据索引自动增加
 * technique和其它渲染状态(alpha test,clip plane)不改变时，传递的参数会一直保留，可以将设置technique和传递参数的操作分散到多个RenderAtom中
 * 调用RenderCore中的相关方法创建或销毁RenderAtom对象
 */
class RenderAtom
{
public:
	/** 返回用于渲染排序分组参考的id */
	int getSortGroup() const;
	/** 返回technique名称 */
	const char *getTechniqueName() const;
	/** 返回technique的id */
	int getTechniqueID() const;
};

#endif
