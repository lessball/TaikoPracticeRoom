#ifndef RENDERCORE_H
#define RENDERCORE_H

#include <boost/any.hpp>
#include <rapidxml.hpp>

#include <RenderCore/RenderAtom.h>
#include <RenderCore/RenderMesh.h>
#include <RenderCore/RenderTexture.h>
#include <RenderCore/RenderState.h>

/**
 * 渲染核心
 * 封装底层渲染api。控制渲染状态，创建和销毁渲染资源。执行渲染操作
 */
class RenderCore
{
public:
	static RenderCore *createRenderCore(const boost::any &window, int colorbit, int depthbit, int stencilbit, int multisample, int swapInterval);
	static void destroyRenderCore(RenderCore *core);

	bool init(const rapidxml::xml_node<> *desc, const char *basepath);
	void release();

	void begin();
	void end();
	
	/**
	 * 生成透视矩阵
	 * target 输出目标，float[16]
	 * tanx   右边界z/x数值
	 * tany   上边界z/y数值
	 * zNear  近裁剪距离
	 * zFar   远裁剪距离
	 * renderToTexture 是否渲染到纹理
	 */
	void matrixPerspective(float *target, float zdx, float zdy, float zNear, float zFar, bool renderToTexture);
	/**
	 * 生成平行投影矩阵
	 * target 输出目标，float[16]
	 * width  宽度
	 * height 高度
	 * zNear  近裁剪距离
	 * zFar   远裁剪距离
	 * renderToTexture 是否渲染到纹理
	 */
	void matrixOrthographic(float *target, float width, float height, float zNear,  float zFar, bool renderToTexture);

	/**
	 * 渲染到纹理时是否需要翻转
	 */
	bool isFilpRenderToTexture();

	void setMainViewSize(int w, int h);
	void getMainViewSize(int &w, int &h);

	void setViewPort(int left, int top, int right, int bottom);
	void getViewPort(int &left, int &top, int &right, int &bottom);

	void setAlphaTest(bool enable, int mode=COMP_GREATER, int key=127);

	void setClipPlane(int count=0, const float *planes=NULL, const float *viewProjMatrix=NULL);

	void setCullMode(int mode);
	int getCullMode();
	void setScissorTest(bool enable, int left=-1, int top=-1, int right=-1, int bottom=-1);
	bool isScissorTestEnable();

	RenderBlendState *createBlendState(const BlendStateDesc *desc);
	void destroyBlendState(RenderBlendState *state);
	void setBlendState(RenderBlendState *state, const float *blendColor=NULL);
	const RenderBlendState *getCurrentBlendState() const;

	RenderDepthStencilState *createDepthStencilState(const DepthStencilStateDesc *desc);
	void destroyDepthStencilState(RenderDepthStencilState *state);
	void setDepthStencilState(RenderDepthStencilState *state, int stencilRef=0);
	const RenderDepthStencilState *getCurrentDepthStencilState() const;

	void clear(bool clearColor, float red, float green, float blue, float alpha, bool clearDepth, float depth, bool clearStencil, int stencil);

	/**
	 * 处理渲染操作。参考RenderAtom类
	 */
	void accept(RenderAtom *atom);

	/**
	 * 创建mesh
	 * vertexLayout 顶点数据布局
	 * numVertexElement vertexLayout数组大小
	 * vertex 顶点数据。vertex==NULL时,如果numVertex>0，则创建动态缓冲，否则没有顶点数据
	 * numVertex 顶点数量
	 * indexByteSize 单个索引数据的字节大小
	 * index 索引数据。index==NULL时，如果numIndex>0，则创建动态缓冲，否则没有索引数据
	 * numIndex 索引数量
	 * meshType MeshType枚举值
	 */
	RenderMesh *createMesh(const VertexElement *vertexLayout, int numVertexElement, const void *vertex, int numVertex, int indexByteSize, const void *index, int numIndex, int meshType);
	/**
	 * 销毁mesh
	 */
	void destroyMesh(RenderMesh *mesh);

	/**
	 * 创建贴图
	 * width 宽度
	 * height 高度
	 * format 像素格式 PixelFormat枚举值
	 * data 像素数据，如果为NULL，则创建动态贴图
	 * mipmap 是否允许mipmap
	 * renderTarget 是否作为renderTarget
	 * depthbit renderTarget的depth buffer的位深度
	 * stencilbit renderTarget的stencil buffer的位深度
	 */
	RenderTexture *createTexture(int width, int height, int format, const void *data, bool mipmap, bool renderTarget=false, int depthbit=0, int stencilbit=0);
	/**
	 * 创建cubemap
	 * size 尺寸（正方形）
	 * format 像素格式。PixelFormat枚举值
	 * data 像素数据，如果为NULL，则创建动态贴图
	 * mipmap 是否允许mipmap
	 * renderTarget 是否作为renderTarget
	 * depthbit renderTarget的depth buffer的位深度
	 * stencilbit renderTarget的stencil buffer的位深度
	 */
	RenderTexture *createCubeTexture(int size, int format, const void **data, bool mipmap, bool renderTarget=false, int depthbit=0, int stencilbit=0);
	/**
	 * 销毁贴图
	 */
	void destroyTexture(RenderTexture *texture);
	/**
	 * 绑定渲染目标。绑定之后需要手动设置Viewport
	 * rt 渲染目标。必须在创建时指定renderTarget。如果rt为NULL，则还原默认的渲染目标
	 * surface 要绑定的贴图表面。TextureSurface枚举值
	 */
	void bindRendeTarget(RenderTexture *rt, int surface=TS_2D);

	/**
	 * 创建RenderAtom。参考RenderAtom类
	 */
	RenderAtom *createRenderAtom(const char *technique, const RenderParam *param, int numParam);

	/**
	 * 销毁RenderAtom
	 */
	void destroyRenderAtom(RenderAtom *atom);
};

#endif
