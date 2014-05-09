/*
** Haaf's Game Engine 1.8
** Copyright (C) 2003-2006, Relish Games
** hge.relishgames.com
**
** PNG Images Optimizer
*/


#include "pngopt.h"


struct color
{
	unsigned char r;
	unsigned char g;
	unsigned char b;
	unsigned char a;
};


struct filelist
{
	char		filename[256];
	filelist*	next;
};

filelist *files=0;


extern bool Write32BitPNGWithPitch(FILE* fp, void* pBits, bool bNeedAlpha, int nWidth, int nHeight, int nPitch);
bool convert(char *filename);


//bool convert(char *filename)
//{
//	HTEXTURE tex;
//	int width, height, pitch;
//	color *buf;
	
//	int i,j,k,l;
//	int r,g,b;
//	int count;

//	FILE *fp=0;
	
//	printf("%s - ", filename);
	
//	tex = hge->Texture_Load(filename);
//	if(!tex) { printf("Can't load texture.\n"); return false; }

//	width  = hge->Texture_GetWidth(tex, true);
//	height = hge->Texture_GetHeight(tex, true);
//	pitch  = hge->Texture_GetWidth(tex, false);

//	buf=(color *)hge->Texture_Lock(tex, false);
//	if(!buf) { printf("Can't lock texture.\n"); return false; }

//	for(i=0; i<height; i++)
//		for(j=0; j<width; j++)
//			if(!buf[i*pitch+j].a)
//			{
//				count = 0;
//				r=g=b = 0;

//				for(k=-1; k<=1; k++)
//					for(l=-1; l<=1; l++)
//						if(i+k >= 0 && i+k < height &&
//						   j+l >= 0 && j+l < width &&
//						   buf[(i+k)*pitch + (j+l)].a)
//						{
//							r += buf[(i+k)*pitch + (j+l)].r;
//							g += buf[(i+k)*pitch + (j+l)].g;
//							b += buf[(i+k)*pitch + (j+l)].b;
//							count++;
//						}

//				if(count)
//				{
//					buf[i*pitch+j].r = unsigned char(r / count);
//					buf[i*pitch+j].g = unsigned char(g / count);
//					buf[i*pitch+j].b = unsigned char(b / count);
//				}
//			}


//	fp=fopen(filename,"wb");
//	if(!fp)
//	{
//		hge->Texture_Unlock(tex);
//		printf("Can't write to file.\n");
//		return false;
//	}

//	if(!Write32BitPNGWithPitch(fp, buf, true, width, height, pitch))
//	{
//		hge->Texture_Unlock(tex);
//		fclose(fp);
//		printf("Error writing data.\n");
//		return false;
//	}

//	fclose(fp);
//	hge->Texture_Unlock(tex);

//	printf("Ok\n");
//	return true;
//}
