#ifndef RENDERSTATE_H
#define RENDERSTATE_H

/**
 * 渲染状态管理
 */

/** 透明混合系数定义 */
enum BlendFactor
{
	BLEND_ZERO,
	BLEND_ONE,
	BLEND_SRC_ALPHA,
	BLEND_INV_SRC_ALPHA,
	BLEND_DST_ALPHA,
	BLEND_INV_DST_ALPHA,
	BLEND_SRC_COLOR,
	BLEND_INV_SRC_COLOR,
	BLEND_DST_COLOR,
	BLEND_INV_DST_COLOR,
	BLEND_CONST,
	BLEND_INV_CONST,
	//GL_SRC_ALPHA_SATURATE?
};
/** 透明混合运算类型定义 */
enum BlendOp
{
	BLEND_ADD,
	BLEND_SUB,
	BLEND_RSUB,
	//BLEND_MIN,
	//BLEND_MAX,
};
/** 比较运算类型，用于depth test和stencil test */
enum CompareOp
{
	COMP_NEVER,
	COMP_ALWAYS,
	COMP_LESS,
	COMP_GREATER,
	COMP_LESS_EQUAL,
	COMP_GREATER_EQUAL,
	COMP_EQUAL,
	COMP_NOT_EQUAL,
};
/** cull模式 */
enum CullMode
{
	CULL_FRONT,
	CULL_BACK,
	CULL_NONE,
};
/** 模板操作类型 */
enum StencilOp
{
	STENCIL_KEEP,
	STENCIL_ZERO,
	STENCIL_REPLACE,
	STENCIL_INCR_CLAMP,
	STENCIL_DECR_CLAMP,
	STENCIL_INCR,
	STENCIL_DECR,
	STENCIL_INVERT,
};

struct BlendStateDesc
{
	bool        blendEnable;
	BlendFactor colorSrc;
	BlendFactor colorDst;
	BlendOp     colorOp;
	BlendFactor alphaSrc;
	BlendFactor alphaDst;
	BlendOp     alphaOp;
	bool		colorWriteEnable[4];
};
class RenderBlendState
{
public:
	void getDesc(BlendStateDesc *desc) const;
};

struct DepthStencilStateDesc
{
	bool depthTestEnable;
	bool depthWriteEnable;
	CompareOp depthCompare;

	bool stencilEnable;
	char stencilReadMask;
	char stencilWriteMask;
	StencilOp stencilFailOp[2];
	StencilOp depthFailOp[2];
	StencilOp passOp[2];
	CompareOp stencilCompare[2];
};
class RenderDepthStencilState
{
public:
	void getDesc(DepthStencilStateDesc *desc) const;
};

#endif
