#ifndef IMAGEIO_H
#define IMAGEIO_H

#include <RenderCore/RenderTexture.h>
#include <RenderCore/RenderCore.h>

/**
 * 加载贴图
 * path 贴图文件路径
 * rc 渲染核心
 */
RenderTexture *loadTexture(const char *path, RenderCore *rc);

RenderTexture *createStringTexture(const char *str, int size, RenderCore *rc);

#endif
