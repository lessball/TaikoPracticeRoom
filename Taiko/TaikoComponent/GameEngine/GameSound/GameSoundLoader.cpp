#include "GameSoundLoader.h"
#include <FileResource/FileResource.h>
#include <FileResource/MemoryFileReader.h>
#include <vorbis/vorbisfile.h>
#include <string.h>

struct WavInfo
{
	short audioFormat;
	short numChannels;
	int sampleFrequency;
	int byteRate;
	short blockAlign;
	short bitsPerSample;
};
void GameSoundLoader::loadWav(void *data, int size)
{
	//²Î¿¼freealut
	const short AF_PCM = 1;
	const short AF_ULAW = 7;
	MemoryFileReader memfile(data, size);
	int chunkSize;
	int magic;
	if(!memfile.readData(&magic) || magic != 0x46464952) //RIFF
		return;
	if(!memfile.readData(&chunkSize) || !memfile.readData(&magic) || magic != 0x45564157) //magic "WAVE"
		return;
	WavInfo *info = NULL;
	void *wavdata = NULL;
	int datalength = 0;
	while(true)
	{
		if(!memfile.readData(&magic) || !memfile.readData(&chunkSize))
			return;
		if (magic == 0x20746d66) //"fmt "
		{
			if(chunkSize < (int)sizeof(info) || (info = (WavInfo*)memfile.readData(sizeof(WavInfo))) == NULL)
				return;
			if(info->audioFormat != AF_PCM && info->audioFormat != AF_ULAW)
				return;
			if(info->audioFormat == AF_ULAW)
				info->bitsPerSample *= 2;
			memfile.seek(chunkSize-sizeof(WavInfo), SEEK_CUR);
		}else if(magic == 0x61746164) //"data"
		{
			if(info == NULL || (wavdata = memfile.readData(chunkSize)) == NULL)
				return;
			datalength = chunkSize;
			if(info->audioFormat == AF_ULAW)
			{
				datalength *= 2;
				unsigned char *srcdata = (unsigned char *)wavdata;
				short *newdata = (short*)(new char[chunkSize*2]);
				for(int i=0; i<chunkSize; i++)
				{
					//mulaw2linear
					unsigned char mulawbyte = srcdata[i];
					static const short exp_lut[8] = {0, 132, 396, 924, 1980, 4092, 8316, 16764};
					short sign, exponent, mantissa, sample;
					mulawbyte = ~mulawbyte;
					sign = (mulawbyte & 0x80);
					exponent = (mulawbyte >> 4) & 0x07;
					mantissa = mulawbyte & 0x0F;
					sample = exp_lut[exponent] + (mantissa << (exponent + 3));
					if (sign != 0)
					{
						sample = -sample;
					}
					newdata[i] = sample;
				}
				wavdata = newdata;
			}
			break;
		}else
		{
			memfile.seek(chunkSize, SEEK_CUR);
		}
	}
	m_numChannel = info->numChannels;
	m_bits = info->bitsPerSample;
	m_freq = info->sampleFrequency;
	m_deletaData = info->audioFormat == AF_ULAW;
	m_data = (char*)wavdata;
	m_dataSize = datalength;
}
static size_t oggRead(void *ptr, size_t size, size_t nmemb, void *datasource)
{
	MemoryFileReader *reader = (MemoryFileReader *)datasource;
	int readed = reader->read(ptr, size*nmemb);
	int nsize = (int)size;
	if(readed % nsize != 0)
	{
		reader->seek(-(readed%nsize), SEEK_CUR);
	}
	return readed / nsize;
}
static int oggSeek(void *datasource, ogg_int64_t offset, int whence)
{
	MemoryFileReader *reader = (MemoryFileReader *)datasource;
	return reader->seek((int)offset, whence);
}
static long oggTell(void *datasource)
{
	MemoryFileReader *reader = (MemoryFileReader *)datasource;
	return reader->tell();
}
static ov_callbacks oggCallback = {oggRead, oggSeek, NULL, oggTell};
void GameSoundLoader::loadOgg(void *data, int size)
{
	MemoryFileReader memFile(data, size);
	OggVorbis_File oggFile;
	if(ov_open_callbacks(&memFile, &oggFile, NULL, 0, oggCallback) != 0)
		return;
	vorbis_info *oggInfo = ov_info(&oggFile, -1);
	int bufferSize = size*2;
	if(bufferSize < 1<<15)
		bufferSize = 1<<15;
	char *buffer = new char[bufferSize];
	int dataSize = 0;
	int readSize = 0;
	do
	{
		int bitStream;
		readSize = ov_read(&oggFile, buffer+dataSize, bufferSize-dataSize, 0, 2, 1, &bitStream);
		dataSize += readSize;
		if(bufferSize - dataSize < (1<<14))
		{
			int newsize = bufferSize + bufferSize/2;
			char *newbuf = new char[newsize];
			memcpy(newbuf, buffer, dataSize);
			delete[] buffer;
			buffer = newbuf;
			bufferSize = newsize;
		}
	}while(readSize > 0);
	m_numChannel = oggInfo->channels;
	m_bits = 16;
	m_freq = oggInfo->rate;
	m_deletaData = true;
	m_data = buffer;
	m_dataSize = dataSize;
	ov_clear(&oggFile);
}

GameSoundLoader::GameSoundLoader(const char *path)
{
	m_fileData = NULL;
	m_data = NULL;
	FileResource *file = FileResource::open(path);
	if(file == NULL)
		return;
	m_fileData = file->readAll();
	int fsize = file->size();
	FileResource::close(file);
	switch(*(int*)m_fileData)
	{
	case 0x5367674f: //OggS
		loadOgg(m_fileData, fsize);
		break;
	case 0x46464952: //RIFF
		loadWav(m_fileData, fsize);
		break;
	}
}
GameSoundLoader::~GameSoundLoader()
{
	if(m_deletaData)
	{
		delete[] m_data;
	}
	m_data = NULL;
	delete[] m_fileData;
	m_fileData = NULL;
}
