#ifndef RENDERTEXTURE_H
#define RENDERTEXTURE_H

/**
 * 定义渲染用的纹理
 */

/** 定义像素格式 */
enum PixelFormat
{
	PF_R8G8B8,
	PF_R8G8B8A8,
	PF_R5G6B5,
	PF_R4G4B4A4,
	PF_R5G5B5A1,
	PF_L8,
	PF_L16,
};

/** 定义纹理表面 */
enum TextureSurface
{
	TS_2D,
	TS_CUBE_X_POS,
	TS_CUBE_X_NEG,
	TS_CUBE_Y_POS,
	TS_CUBE_Y_NEG,
	TS_CUBE_Z_POS,
	TS_CUBE_Z_NEG,
};

//bool isPixelFormatUsable(int format, bool renderTarget);

/**
 * 调用RenderCore中的方法创建或销毁RenderTexture对象
 */
class RenderTexture
{
public:
	int getWidth() const;
	int getHeight() const;
	int getPixelFormat() const;
	bool isCubeMap() const;
	/**
	 * 更新贴图数据
	 * surface 纹理表面 TextureSurface枚举值
	 */
	void updateDynamicRect(int surface, int x, int y, int width, int height, int format, void *data);
	/**
	 * 从主缓冲区拷贝贴图数据
	 */
	void copyFromMainBuffer(bool mipmap);
	/**
	 * 更新mipmap
	 */
	void updateRenderTargetMipmap();
};

/** 纹理过滤选项，通过组合位标记使用 */
enum TextureFilter
{
	TF_MASK_MIN_LINEAR = 1,
	TF_MASK_MAG_LINEAR = 2,
	TF_MASK_MIP = 4,
	TF_MASK_MIP_LINEAR = 8,
	//预定义常用的组合
	TF_NEAREST = 0,
	TF_BILINEAR = TF_MASK_MIN_LINEAR | TF_MASK_MAG_LINEAR | TF_MASK_MIP,
	TF_TRILINEAR = TF_MASK_MIN_LINEAR | TF_MASK_MAG_LINEAR | TF_MASK_MIP_LINEAR,
	TF_MIPLINEAR = TF_MASK_MAG_LINEAR | TF_MASK_MIP_LINEAR,
};

/** 定义纹理wrap模式 */
enum TextureWrap
{
	TW_REPEAT,
	TW_CLAMP,
	TW_MIRROR_REPEAT,
};
/** 定义纹理sampler，作为渲染参数使用 */
struct RenderSampler
{
	RenderTexture *texture; //贴图
	unsigned char filter; //纹理过滤
	unsigned char anisotropy; //各向异性
	unsigned char wrapx; //x轴wrap模式
	unsigned char wrapy; //y轴wrap模式
};

#endif
