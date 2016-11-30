/* ////////////////////////////////////////////////////////////

YS Bitmap / YS PNG library
Copyright (c) 2014 Soji Yamakawa.  All rights reserved.
http://www.ysflight.com

Redistribution and use in source and binary forms, with or without modification, 
are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, 
   this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice, 
   this list of conditions and the following disclaimer in the documentation 
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, 
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR 
PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS 
BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE 
GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) 
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT 
LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT 
OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

File Name: yspng.h
//////////////////////////////////////////////////////////// */

#ifndef YSPNG_IS_INCLUDED
#define YSPNG_IS_INCLUDED
/* { */

#ifndef YSRESULT_IS_DEFINED
#define YSERR 0
#define YSOK 1
#endif

#ifndef YSBOOL_IS_DEFINED
#define YSBOOL_IS_DEFINED
#define YSFALSE 0
#define YSTRUE  1
#endif


class YsPngHuffmanTree
{
public:
	YsPngHuffmanTree();
	~YsPngHuffmanTree();
	YsPngHuffmanTree *zero,*one;
	unsigned int dat;
	unsigned int weight,depth;
	static int leakTracker;

	static void DeleteHuffmanTree(YsPngHuffmanTree *node);
};

class YsPngUncompressor
{
public:
	class YsGenericPngDecoder *output;

	inline unsigned int GetNextBit(const unsigned char dat[],unsigned &bytePtr,unsigned &bitPtr)
	{
		unsigned a;
		a=dat[bytePtr]&bitPtr;
		bitPtr<<=1;
		if(bitPtr>=256)
		{
			bitPtr=1;
			bytePtr++;
		}
		return (a!=0 ? 1 : 0);
	}
	inline unsigned int GetNextMultiBit(const unsigned char dat[],unsigned &bytePtr,unsigned &bitPtr,unsigned n)
	{
		unsigned value,mask,i;
		value=0;
		mask=1;
		for(i=0; i<n; i++)
		{
			if(GetNextBit(dat,bytePtr,bitPtr))
			{
				value|=mask;
			}
			mask<<=1;
		}
		return value;
	}

	void MakeFixedHuffmanCode(unsigned hLength[288],unsigned hCode[288]);
	static void MakeDynamicHuffmanCode(unsigned hLength[288],unsigned hCode[288],unsigned nLng,unsigned lng[]);
	int DecodeDynamicHuffmanCode
	   (unsigned int &hLit,unsigned int &hDist,unsigned int &hCLen,
	    unsigned int *&hLengthLiteral,unsigned int *&hCodeLiteral,
	    unsigned int *&hLengthDist,unsigned int *&hCodeDist,
	    unsigned int hLengthBuf[322],unsigned int hCodeBuf[322],
	    const unsigned char dat[],unsigned int &bytePtr,unsigned int &bitPtr);
	    
	YsPngHuffmanTree *MakeHuffmanTree(unsigned n,unsigned hLength[],unsigned hCode[]);
	void DeleteHuffmanTree(YsPngHuffmanTree *node);

	unsigned GetCopyLength(unsigned value,unsigned char dat[],unsigned &bytePtr,unsigned &bitPtr);
	unsigned GetBackwardDistance(unsigned distCode,unsigned char dat[],unsigned &bytePtr,unsigned &bitPtr);

	int Uncompress(unsigned length,unsigned char dat[]);
};

////////////////////////////////////////////////////////////

class YsPngHeader
{
public:
	unsigned int width, height;
	unsigned int bitDepth, colorType;
	unsigned int compressionMethod, filterMethod, interlaceMethod;

	void Decode(unsigned char dat[]);
};

class YsPngPalette
{
public:
	unsigned int nEntry;
	unsigned char *entry;

	YsPngPalette();
	~YsPngPalette();
	int Decode(unsigned length,unsigned char dat[]);
};

class YsPngTransparency
{
public:
	unsigned int col[3];

	// For color type 3, up to three transparent colors is supported.
	int Decode(unsigned length,unsigned char dat[],unsigned int colorType);
};

class YsPngGenericBinaryStream
{
public:
	virtual size_t GetSize(void) const=0;
	virtual size_t Read(unsigned char buf[],size_t readSize)=0;
};

class YsPngBinaryFileStream : public YsPngGenericBinaryStream
{
private:
	FILE *fp;
public:
	explicit YsPngBinaryFileStream(FILE *fp);
	virtual size_t GetSize(void) const;
	virtual size_t Read(unsigned char buf[],size_t readSize);
};

class YsPngBinaryMemoryStream : public YsPngGenericBinaryStream
{
private:
	size_t dataSize;
	unsigned char *binaryData;
public:
	~YsPngBinaryMemoryStream();
	YsPngBinaryMemoryStream(size_t dataSize);
	YsPngBinaryMemoryStream(size_t dataSize, unsigned char binaryData[]);
	size_t offset_rd;
	size_t offset_wr;
	size_t Write(unsigned char c);
	size_t Write(unsigned char buf[], size_t writeSize);
	virtual size_t GetSize(void) const;
	virtual size_t Read(unsigned char buf[],size_t readSize);
};

class YsGenericPngDecoder
{
public:
	enum
	{
		gamma_default=100000
	};

	YsPngHeader hdr;
	YsPngPalette plt;
	YsPngTransparency trns;
	unsigned int gamma;

	static unsigned int verboseMode;

	YsGenericPngDecoder();
	void Initialize(void);
	int CheckSignature(YsPngGenericBinaryStream &binStream);
	int ReadChunk(unsigned &length,unsigned char *&buf,unsigned &chunkType,unsigned &crc,YsPngGenericBinaryStream &binStream);
	int Decode(const char fn[]);
	int Decode(FILE *fp);
	int Decode(YsPngGenericBinaryStream &binStream);

	virtual int PrepareOutput(void);
	virtual int Output(unsigned char dat);
	virtual int EndOutput(void);
};



////////////////////////////////////////////////////////////

class YsRawPngDecoder : public YsGenericPngDecoder
{
public:
	YsRawPngDecoder();
	~YsRawPngDecoder();


	int wid, hei;

	int filter, x, y, firstByte;
	int inLineCount;
	int inPixelCount;
	unsigned int rgba; // Raw data of R,G,B,A
	unsigned int r, g, b, msb;  // msb for reading 16 bit depth
//	unsigned int index;

	unsigned int interlacePass;

	// For filtering
	unsigned char *twoLineBuf8, *curLine8, *prvLine8;

	void ShiftTwoLineBuf(void);

	virtual int PrepareOutput(void);
	virtual int Output(unsigned char dat);
	virtual int EndOutput(void);

	virtual int fnPreDraw(int wid, int hei, int bitDepth, int colorType);
	virtual void fnDraw(int x, int y, unsigned int rgba);
	virtual void fnEndDraw(void);

//	void Flip(void);  // For drawing in OpenGL
};

/* } */
#endif
