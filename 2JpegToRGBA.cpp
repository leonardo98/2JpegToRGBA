// 2JpegToRGBA.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "pngopt/pngopt.h"
#include <stdio.h>
#include <string>

extern "C" {
#include "jpeglib.h"
#include "setjmp.h"
}


unsigned char *ImageBuffer = NULL;
int Width; 
int Height;

int read_JPEG_file (const char * filename);

int _tmain(int argc, _TCHAR* argv[])
{

	if (argc != 2)
	{
		printf("usage: 2JpegToGRBA <alpha_channel_jpeg>");
		return 0;
	}

	/* beautifull image
	for (unsigned int i = 0; i < 256; ++i)
	{
		for (unsigned int j = 0; j < 256; ++j)
		{
			ImageBuffer[(i + j * 256) * 4] = i;
			ImageBuffer[(i + j * 256) * 4 + 1] = 255 - j;
			ImageBuffer[(i + j * 256) * 4 + 2] = 255;
			ImageBuffer[(i + j * 256) * 4 + 3] = 255;
		}
	}
	*/

	std::string fileName = argv[1];

	read_JPEG_file(fileName.c_str());
	fileName.replace(fileName.length() - 5, 1, "b");
	read_JPEG_file(fileName.c_str());

	fileName.replace(fileName.length() - 7, 7, ".png");
	FILE *fp = fopen(fileName.c_str(), "wb");
	if(!fp)
	{
		printf("Can't write to file.\n");
		return -1;
	}

	if(!Write32BitPNGWithPitch(fp, ImageBuffer, true, Width, Height, Width))
	{
		printf("Error writing data.\n");
	}
    else
    {
        printf("Ok\n");
    }

	fclose(fp);

	delete [] ImageBuffer;

	return 0;
}

struct my_error_mgr {
  struct jpeg_error_mgr pub;	/* "public" fields */

  jmp_buf setjmp_buffer;	/* for return to caller */
};


typedef struct my_error_mgr * my_error_ptr;

METHODDEF(void)
my_error_exit (j_common_ptr cinfo)
{
  /* cinfo->err really points to a my_error_mgr struct, so coerce pointer */
  my_error_ptr myerr = (my_error_ptr) cinfo->err;

  /* Always display the message. */
  /* We could postpone this until after returning, if we chose. */
  (*cinfo->err->output_message) (cinfo);

  /* Return control to the setjmp point */
  longjmp(myerr->setjmp_buffer, 1);
}

int read_JPEG_file (const char * filename)
{
    struct jpeg_decompress_struct cinfo;
    struct my_error_mgr jerr;

    /* More stuff */
    FILE * infile;      /* source file */
	int buffer_height = 1;
	JSAMPARRAY buffer = (JSAMPARRAY)malloc(sizeof(JSAMPROW) * buffer_height);
    int row_stride;     /* physical row width in output buffer */

    if ((infile = fopen(filename, "rb")) == NULL) {
        fprintf(stderr, "can't open %s\n", filename);
        return -1;
    }

    /* Step 1: allocate and initialize JPEG decompression object */

    /* We set up the normal JPEG error routines, then override error_exit. */
    cinfo.err = jpeg_std_error(&jerr.pub);
    jerr.pub.error_exit = my_error_exit;
    /* Establish the setjmp return context for my_error_exit to use. */
    if (setjmp(jerr.setjmp_buffer)) {

        jpeg_destroy_decompress(&cinfo);
        fclose(infile);
        return 0;
    }
    /* Now we can initialize the JPEG decompression object. */
    jpeg_create_decompress(&cinfo);

    /* Step 2: specify data source (eg, a file) */

    jpeg_stdio_src(&cinfo, infile);

    /* Step 3: read file parameters with jpeg_read_header() */

    (void) jpeg_read_header(&cinfo, TRUE);
    /* Step 4: set parameters for decompression */

    /* In this example, we don't need to change any of the defaults set by
     * jpeg_read_header(), so we do nothing here.
     */

    /* Step 5: Start decompressor */

    (void) jpeg_start_decompress(&cinfo);


    row_stride = cinfo.output_width * cinfo.output_components;
    /* Make a one-row-high sample array that will go away when done with image */
    buffer = (*cinfo.mem->alloc_sarray) ((j_common_ptr) &cinfo, JPOOL_IMAGE, row_stride, 1);

    Width = cinfo.output_width;
    Height = cinfo.output_height;

	if (ImageBuffer == NULL)
	{
	    ImageBuffer = new unsigned char [cinfo.output_width * cinfo.output_height * 4];
	}
    //ImageBuffer = new unsigned char [cinfo.output_width * cinfo.output_height * cinfo.output_components];
    long counter = 0;
	
	buffer[0] = (JSAMPROW)malloc(sizeof(JSAMPLE) * row_stride);

   //step 6, read the image line by line
	unsigned int imageBytes = 0;

	if (cinfo.output_components == 3)
	{
		while (cinfo.output_scanline < cinfo.output_height) {
			jpeg_read_scanlines(&cinfo, buffer, 1);
			for (unsigned int i = 0; i < row_stride; i += 3)
			{
				ImageBuffer[imageBytes] = buffer[0][i + 2];
				ImageBuffer[imageBytes + 1] = buffer[0][i + 1];
				ImageBuffer[imageBytes + 2] = buffer[0][i];
				imageBytes += 4;
			}
			//memcpy(ImageBuffer + counter, buffer[0], row_stride);
			counter += row_stride;
		}	
	}
	else if (cinfo.output_components == 1)
	{
		while (cinfo.output_scanline < cinfo.output_height) {
			jpeg_read_scanlines(&cinfo, buffer, 1);
			for (unsigned int i = 0; i < row_stride; ++i)
			{
				ImageBuffer[imageBytes + 3] = buffer[0][i];
				imageBytes += 4;
			}
			counter += row_stride;
		}	
	} 
	else 
	{
		return 100;
	}

       /* Step 7: Finish decompression */

    (void) jpeg_finish_decompress(&cinfo);
    /* Step 8: Release JPEG decompression object */

    /* This is an important step since it will release a good deal of memory. */
    jpeg_destroy_decompress(&cinfo);

    fclose(infile);
    /* And we're done! */
    return 1;
}