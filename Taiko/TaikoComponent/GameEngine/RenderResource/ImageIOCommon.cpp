#include "ImageIO.h"
#include "DDSTexture.h"
#include <FileResource/FileResource.h>
#include <png.h>
#include <jpeglib.h>
#include <assert.h>

struct PngIO
{
	char *data;
	int leftSize;
};
void PngRead(png_structp png_ptr, png_bytep buffer, png_size_t size)
{
	PngIO *pio = (PngIO*)png_get_io_ptr(png_ptr);
	assert(pio != NULL);
	if((int)size > pio->leftSize)
		png_error(png_ptr, "");
	memcpy(buffer, pio->data, size);
	pio->data += size;
	pio->leftSize -= size;
}

struct JpegErrorMgr {
	struct jpeg_error_mgr pub;	/* "public" fields */

	jmp_buf setjmp_buffer;	/* for return to caller */
};
void JpegErrorExit (j_common_ptr cinfo)
{
	/* cinfo->err really points to a my_error_mgr struct, so coerce pointer */
	JpegErrorMgr *myerr = (JpegErrorMgr*) cinfo->err;

	/* Always display the message. */
	/* We could postpone this until after returning, if we chose. */
	(*cinfo->err->output_message) (cinfo);

	/* Return control to the setjmp point */
	longjmp(myerr->setjmp_buffer, 1);
}

RenderTexture *loadTexture(const char *path, RenderCore *rc)
{
	FileResource * file = FileResource::open(path);
	if(file == NULL)
		return NULL;
	int filesize = file->size();
	char *filedata = file->readAll();
	FileResource::close(file);

	RenderTexture *tex = NULL;
	switch(*(int*)filedata)
	{
	case 0x20534444: //DDS
		tex = loadDDSTexture(filedata, filesize, rc);
		break;
	case 0x474e5089: //png
		{
			png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
			if(png_ptr == NULL)
				break;
			png_infop info_ptr = png_create_info_struct(png_ptr);
			if(info_ptr == NULL)
			{
				png_destroy_read_struct(&png_ptr, NULL, NULL);
				break;
			}
			if(setjmp(png_jmpbuf(png_ptr)))
			{
				png_destroy_read_struct(&png_ptr, NULL, NULL);
				break;
			}
			PngIO pio = {filedata, filesize};
			png_set_read_fn(png_ptr, &pio, PngRead);
			png_read_png(png_ptr, info_ptr, PNG_TRANSFORM_STRIP_16, NULL);
			int width = png_get_image_width(png_ptr, info_ptr);
			int height = png_get_image_height(png_ptr, info_ptr);
			png_bytepp prows = png_get_rows(png_ptr, info_ptr);
			int channels = png_get_channels(png_ptr, info_ptr);
			int rowsize = channels*width;
			int format;
			switch(channels)
			{
			case 3:
				format = PF_R8G8B8;
				break;
			case 4:
				format = PF_R8G8B8A8;
				break;
			default:
				prows = NULL;
				break;
			}
			if(width == 0 || height == 0 || prows == NULL || rowsize != png_get_rowbytes(png_ptr, info_ptr))
			{
				png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
				break;
			}
			char *texdata = new char[rowsize*height];
			for(int i=0; i<height; i++)
				memcpy(texdata+rowsize*i, prows[i], rowsize);
			png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
			tex = rc->createTexture(width, height, format, texdata, true, false);
			delete[] texdata;
		}
		break;
	default:
		if (*(short*)filedata == (short)0xd8ff && filedata[filesize - 1] == (char)0xd9 && filedata[filesize - 2] == (char)0xff) //jpg
		{
			jpeg_decompress_struct cinfo;
			JpegErrorMgr jerr;
			JSAMPARRAY buffer;
			char *texdata = NULL;
			cinfo.err = jpeg_std_error(&jerr.pub);
			cinfo.err->error_exit = JpegErrorExit;
			if(setjmp(jerr.setjmp_buffer))
			{
				jpeg_destroy_decompress(&cinfo);
				delete[] texdata;
				texdata = NULL;
				break;
			}
			jpeg_create_decompress(&cinfo);
			jpeg_mem_src(&cinfo, (unsigned char*)filedata, filesize);
			if(jpeg_read_header(&cinfo, TRUE) != JPEG_HEADER_OK
				|| jpeg_start_decompress(&cinfo) == FALSE)
			{
				jpeg_destroy_decompress(&cinfo);
				break;
			}
			int width = cinfo.output_width;
			int height = cinfo.output_height;
			int rowsize = width * cinfo.output_components;
			int format;
			switch(cinfo.output_components)
			{
			case 3:
				format = PF_R8G8B8;
				break;
			case 4:
				format = PF_R8G8B8A8;
				break;
			default:
				rowsize = 0;
				break;
			}
			if(rowsize <= 0)
			{
				jpeg_destroy_decompress(&cinfo);
				break;
			}
			buffer = (*cinfo.mem->alloc_sarray)
				((j_common_ptr) &cinfo, JPOOL_IMAGE, rowsize, 1);
			texdata = new char[rowsize*cinfo.output_height];
			while(cinfo.output_scanline < cinfo.output_height)
			{
				int iline = cinfo.output_scanline;
				jpeg_read_scanlines(&cinfo, buffer, 1);
				memcpy(texdata+iline*rowsize, buffer[0], rowsize);
			}
			jpeg_finish_decompress(&cinfo);
			jpeg_destroy_decompress(&cinfo);
			tex = rc->createTexture(width, height, format, texdata, true, false);
			delete[] texdata;
		}
		break;
	}
	delete[] filedata;
	return tex;
}
