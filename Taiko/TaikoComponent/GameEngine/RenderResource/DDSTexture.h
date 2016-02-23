#ifndef DDSTEXTURE_H
#define DDSTEXTURE_H

#include <RenderCore/RenderTexture.h>
#include <RenderCore/RenderCore.h>

RenderTexture *loadDDSTexture(void *data, int dataSize, RenderCore *rc);

#endif
