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
int Quality;

int read_JPEG_file (const char * filename);
GLOBAL(void)
write_JPEG_file (char * filename, int quality, unsigned char *image_buffer, int component);

int _tmain(int argc, _TCHAR* argv[])
{

	if (argc != 5)
	{
		printf("usage:\n2JpegToGRBA compress <rgba_png> <output_base_name> <Quality>\nor\n2JpegToGRBA check <rgb_jpeg> <alpha(grayscaled)_jpeg> <output_png>\n");
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

	std::string task = argv[1];

	if (task == "compress")
	{
		Quality = atoi(argv[4]);
		if (png_texture_load(argv[2], &Width, &Height, ImageBuffer) != 0)
		{
			printf("can't load file: %s\n", argv[2]);
			unsigned char *rgb = new unsigned char[Width * Height * 3];
			unsigned char *alpha = new unsigned char[Width * Height];
			for (unsigned int i = 0; i < Width; ++i)
			{
				for (unsigned int j = 0; j < Height; ++j)
				{
					rgb[(i + j * Width) * 3] = ImageBuffer[(i + j * Width) * 4];
					rgb[(i + j * Width) * 3 + 1] = ImageBuffer[(i + j * Width) * 4 + 1];
					rgb[(i + j * Width) * 3 + 2] = ImageBuffer[(i + j * Width) * 4 + 2];
					alpha[i + j * Width] = ImageBuffer[(i + j * Width) * 4 + 3];
				}
			}
			char buff[100];
			sprintf(buff, "%s_rgb.jpg", argv[3]);
			write_JPEG_file(buff, Quality, rgb, 3);
			sprintf(buff, "%s_alpha.jpg", argv[3]);
			write_JPEG_file(buff, Quality, alpha, 1);
			delete [] rgb;
			delete [] alpha;
		}
	}
	else if (task == "check")
	{
		read_JPEG_file(argv[2]);
		read_JPEG_file(argv[3]);

		FILE *fp = fopen(argv[4], "wb");
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
	}
	else
	{
		printf("usage:\n2JpegToGRBA compress <rgba_png> <output_base_name> <Quality>\nor\n2JpegToGRBA check <rgb_jpeg> <alpha(grayscaled)_jpeg> <output_png>\n");
		return 0;
	}

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

GLOBAL(void)
write_JPEG_file (char * filename, int quality, unsigned char *image_buffer, int component)
{
  /* This struct contains the JPEG compression parameters and pointers to
   * working space (which is allocated as needed by the JPEG library).
   * It is possible to have several such structures, representing multiple
   * compression/decompression processes, in existence at once.  We refer
   * to any one struct (and its associated working data) as a "JPEG object".
   */
  struct jpeg_compress_struct cinfo;
  /* This struct represents a JPEG error handler.  It is declared separately
   * because applications often want to supply a specialized error handler
   * (see the second half of this file for an example).  But here we just
   * take the easy way out and use the standard error handler, which will
   * print a message on stderr and call exit() if compression fails.
   * Note that this struct must live as long as the main JPEG parameter
   * struct, to avoid dangling-pointer problems.
   */
  struct jpeg_error_mgr jerr;
  /* More stuff */
  FILE * outfile;		/* target file */
  JSAMPROW row_pointer[1];	/* pointer to JSAMPLE row[s] */
  int row_stride;		/* physical row width in image buffer */

  /* Step 1: allocate and initialize JPEG compression object */

  /* We have to set up the error handler first, in case the initialization
   * step fails.  (Unlikely, but it could happen if you are out of memory.)
   * This routine fills in the contents of struct jerr, and returns jerr's
   * address which we place into the link field in cinfo.
   */
  cinfo.err = jpeg_std_error(&jerr);
  /* Now we can initialize the JPEG compression object. */
  jpeg_create_compress(&cinfo);

  /* Step 2: specify data destination (eg, a file) */
  /* Note: steps 2 and 3 can be done in either order. */

  /* Here we use the library-supplied code to send compressed data to a
   * stdio stream.  You can also write your own code to do something else.
   * VERY IMPORTANT: use "b" option to fopen() if you are on a machine that
   * requires it in order to write binary files.
   */
  if ((outfile = fopen(filename, "wb")) == NULL) {
    fprintf(stderr, "can't open %s\n", filename);
    exit(1);
  }
  jpeg_stdio_dest(&cinfo, outfile);

  /* Step 3: set parameters for compression */

  /* First we supply a description of the input image.
   * Four fields of the cinfo struct must be filled in:
   */
  cinfo.image_width = Width; 	/* image width and height, in pixels */
  cinfo.image_height = Height;
  if (component == 3)
  {
	  cinfo.input_components = 3;		/* # of color components per pixel */
	  cinfo.in_color_space = JCS_RGB; 	/* colorspace of input image */
  }
  else if (component == 1)
  {
	  cinfo.input_components = 1;		/* # of color components per pixel */
	  cinfo.in_color_space = JCS_GRAYSCALE; 	/* colorspace of input image */
  }
  else
  {
	  exit(-1);
  }

  /* Now use the library's routine to set default compression parameters.
   * (You must set at least cinfo.in_color_space before calling this,
   * since the defaults depend on the source color space.)
   */
  jpeg_set_defaults(&cinfo);
  /* Now you can set any non-default parameters you wish to.
   * Here we just illustrate the use of quality (quantization table) scaling:
   */
  jpeg_set_quality(&cinfo, quality, TRUE /* limit to baseline-JPEG values */);

  /* Step 4: Start compressor */

  /* TRUE ensures that we will write a complete interchange-JPEG file.
   * Pass TRUE unless you are very sure of what you're doing.
   */
  jpeg_start_compress(&cinfo, TRUE);

  /* Step 5: while (scan lines remain to be written) */
  /*           jpeg_write_scanlines(...); */

  /* Here we use the library's state variable cinfo.next_scanline as the
   * loop counter, so that we don't have to keep track ourselves.
   * To keep things simple, we pass one scanline per call; you can pass
   * more if you wish, though.
   */
  row_stride = Width * cinfo.input_components;	/* JSAMPLEs per row in image_buffer */

  while (cinfo.next_scanline < cinfo.image_height) {
    /* jpeg_write_scanlines expects an array of pointers to scanlines.
     * Here the array is only one element long, but you could pass
     * more than one scanline at a time if that's more convenient.
     */
    row_pointer[0] = & image_buffer[cinfo.next_scanline * row_stride];
    (void) jpeg_write_scanlines(&cinfo, row_pointer, 1);
  }

  /* Step 6: Finish compression */

  jpeg_finish_compress(&cinfo);
  /* After finish_compress, we can close the output file. */
  fclose(outfile);

  /* Step 7: release JPEG compression object */

  /* This is an important step since it will release a good deal of memory. */
  jpeg_destroy_compress(&cinfo);

  /* And we're done! */
}
