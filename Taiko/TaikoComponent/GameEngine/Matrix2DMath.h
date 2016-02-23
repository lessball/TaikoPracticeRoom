#ifndef MATRIX2DMATH_H
#define MATRIX2DMATH_H

#include <limits>
#include <math.h>

/**
 * 计算2D坐标的矩阵变换
 * out 输出坐标，可以等于in
 * in 输入坐标
 * mat 矩阵
 * return 等于out
 */
inline float *matrix2DTransform(float *out, const float *in, const float *mat)
{
	float x = in[0]*mat[0] + in[1]*mat[1] + mat[2];
	out[1] = in[0]*mat[3] + in[1]*mat[4] + mat[5];
	out[0] = x;
	return out;
}

/**
 * 3x2 2D矩阵乘法
 * out 输出。不能等于a或b
 * a 第一个矩阵
 * b 第二个矩阵
 * return 等于out
 */
inline float *matrix2DMul(float *out, const float *a, const float *b)
{
	out[0] = a[0] * b[0] + a[3] * b[1];
	out[1] = a[1] * b[0] + a[4] * b[1];
	out[2] = a[2] * b[0] + a[5] * b[1] + b[2];
	out[3] = a[0] * b[3] + a[3] * b[4];
	out[4] = a[1] * b[3] + a[4] * b[4];
	out[5] = a[2] * b[3] + a[5] * b[4] + b[5];
	return out;
}

/**
 * 计算逆矩阵
 * out 输出。可以等于in
 * in 输入
 * return 等于out
 */
inline float *matrix2DInverse(float *out, const float *in)
{
	float m_00 = in[4];
	float m_01 = -in[1];
	float m_02 = in[1]*in[5] - in[4]*in[2];
	float m_10 = -in[3];
	float m_11 = in[0];
	float m_12 = in[3]*in[2] - in[0]*in[5];
	float D = 1.0f / (in[0]*m_00 + in[3]*m_01);
	out[0] = m_00*D;
	out[1] = m_01*D;
	out[2] = m_02*D;
	out[3] = m_10*D;
	out[4] = m_11*D;
	out[5] = m_12*D;
	return out;
}

/**
 * 创建单位矩阵
 * out 输出
 * return 等于out
 */
inline float *matrix2DIdentity(float *out)
{
	out[0] = out[4] = 1.0f;
	out[1] = out[2] = out[3] = out[5] = 0.0f;
	return out;
}

/**
 * 使用给定参数顺序构建3x2 2D矩阵
 * out 输出
 * scalex x轴缩放
 * scaley y轴缩放
 * x x坐标
 * y y坐标
 * return 等于out
 */
inline float *matrix2DScalePosition(float *out, float scalex, float scaley, float x, float y)
{
	out[0] = scalex;
	out[2] = x;
	out[4] = scaley;
	out[5] = y;
	out[1] = out[3] = 0.0f;
	return out;
}
/**
 * 使用给定参数顺序构建3x2 2D矩阵
 * out 输出
 * scalex x轴缩放
 * scaley y轴缩放
 * rotation 旋转角度
 * x x坐标
 * y y坐标
 * return 等于out
 */
inline float *matrix2DScaleRotationPosition(float *out, float scalex, float scaley, float rotation, float x, float y)
{
	float sinr = sin(rotation);
	float cosr = cos(rotation);
	out[0] = cosr * scalex;
	out[1] = -sinr * scaley;
	out[2] = x;
	out[3] = sinr * scalex;
	out[4] = cosr * scaley;
	out[5] = y;
	return out;
}
/**
 * 使用给定参数构建3x2 2D矩阵
 * out 输出
 * width 图片宽度
 * height 图片高度
 * pivotx 图片自身原点x坐标
 * pivoty 图片自身原点y坐标
 * scalex 图片自身x轴缩放
 * scaley 图片自身y轴缩放
 * rotation 旋转角度
 * x x坐标
 * y y坐标
 * return 等于out
 */
inline float *matrix2DComplex(float *out, float width, float height, float pivotx, float pivoty, float scalex, float scaley, float rotation, float x, float y)
{
	float mata[6];
	float matb[6];
	matrix2DScalePosition(mata, width, height, -pivotx, -pivoty);
	matrix2DScaleRotationPosition(matb, scalex, scaley, rotation, x, y);
	matrix2DMul(out, mata, matb);
	return out;
}

/**
 * 计算两个2D坐标之间的距离的平方
 */
inline float distance2DSquare(const float *a, const float *b)
{
	return (a[0]-b[0])*(a[0]-b[0]) + (a[1]-b[1])*(a[1]-b[1]);
}
/**
 * 计算两个2D坐标之间的距离
 */
inline float distance2D(const float *a, const float *b)
{
	return sqrt(distance2DSquare(a, b));
}

/** 定义float类型的nan值 */
#define FLOAT_NAN (std::numeric_limits<float>::quiet_NaN())
/**
 * 判断float值是否nan
 */
inline bool isNaN(float f)
{
	return f != f;
}

#endif
