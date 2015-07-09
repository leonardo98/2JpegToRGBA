#include <stdio.h>
#include <stdlib.h>

#ifdef WIN32
#include <windows.h>
#endif

bool Write32BitPNGWithPitch(FILE* fp, void* pBits, bool bNeedAlpha, int nWidth, int nHeight, int nPitch);

int png_texture_load(const char * file_name, int * width, int * height, unsigned char *&imageData, int * pitch = NULL);