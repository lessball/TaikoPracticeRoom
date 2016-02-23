#include "DDSTexture.h"
#include <FileResource/MemoryFileReader.h>
#include <LogPrint.h>

struct _DDS_PIXELFORMAT
{
	int dwSize;
	int dwFlags;
	int dwFourCC;
	int dwRGBBitCount;
	int dwRBitMask;
	int dwGBitMask;
	int dwBBitMask;
	int dwABitMask;
	bool operator == (const _DDS_PIXELFORMAT &r) const
	{
		return dwSize == r.dwSize && dwFlags == r.dwFlags && dwFourCC == r.dwFourCC && dwRGBBitCount == r.dwRGBBitCount
			&& dwRBitMask == r.dwRBitMask && dwGBitMask == r.dwGBitMask && dwBBitMask == r.dwBBitMask && dwABitMask == r.dwABitMask;
	}
};
struct _DDSHeader
{
	int dwMagic;
	int dwSize;
	int dwFlags;
	int dwHeight;
	int dwWidth;
	int dwPitchOrLinearSize;
	int dwDepth;
	int dwMipMapCount;
	int dwReserved1[11];
	_DDS_PIXELFORMAT ddspf;
	int dwCaps;
	int dwCaps2;
	int dwCaps3;
	int dwCaps4;
	int dwReserved2;
};

RenderTexture *loadDDSTexture(void *data, int dataSize, RenderCore *rc)
{
	MemoryFileReader reader(data, dataSize);
	_DDSHeader *header = (_DDSHeader*)reader.readData(sizeof(_DDSHeader));
	if(header == NULL || header->dwMagic != 0x20534444) //"DDS "
		return NULL;
	const _DDS_PIXELFORMAT DDSPF_A8R8G8B8 =
		{sizeof(_DDS_PIXELFORMAT), 0x00000041, 0, 32, 0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000};
	const _DDS_PIXELFORMAT DDSPF_B8G8R8 =
		{sizeof(_DDS_PIXELFORMAT), 0x00000040, 0, 24, 0x000000ff, 0x0000ff00, 0x00ff0000, 0x00000000};
	const _DDS_PIXELFORMAT DDSPF_R5G6B5 =
		{sizeof(_DDS_PIXELFORMAT), 0x00000040, 0, 16, 0x0000f800, 0x000007e0, 0x0000001f, 0x00000000};
	int pixelFormat;
	if(header->ddspf == DDSPF_A8R8G8B8)
	{
		pixelFormat = PF_R8G8B8A8;
	}else if(header->ddspf == DDSPF_B8G8R8)
	{
		pixelFormat = PF_R8G8B8;
	}else if(header->ddspf == DDSPF_R5G6B5)
	{
		pixelFormat = PF_R5G6B5;
	}else
	{
		LOG_PRINT("error: unsuppoted dds format");
		return NULL;
	}
	int width = header->dwWidth;
	int height = header->dwHeight;
	int numFace;
	if((header->dwCaps & 0x8) != 0 && (header->dwCaps2 & 0x200) != 0) //cubemap
	{
		if(width != height)
		{
			LOG_PRINT("error: cube map width != height");
		}
		if(header->dwCaps2 != 0xfe00)
		{
			LOG_PRINT("error: require all cube face");
			return NULL;
		}
		numFace = 6;
	}else
	{
		numFace = 1;
	}
	int pixelSize = header->ddspf.dwRGBBitCount / 8;
	int pitch = width * pixelSize;
	int facesize = pitch * height;
	if((header->dwFlags & 0x20000) != 0)
	{
		int tsize = facesize;
		for(int i=1; i<header->dwMipMapCount; i++)
			facesize += tsize>>i;
	}
	unsigned char *faces[6];
	for(int i=0; i<numFace; i++)
	{
		faces[i] = (unsigned char *)reader.readData(facesize);
		if(faces[i] == NULL)
		{
			LOG_PRINT("error: unexpected end of dds data");
			return NULL;
		}
	}
	if(numFace == 6)
	{
		return rc->createCubeTexture(width, pixelFormat, (const void**)faces, true);
	}else
	{
		return rc->createTexture(width, height, pixelFormat, faces[0], true);
	}
}