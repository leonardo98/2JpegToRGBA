// 2JpegToRGBA.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "pngopt/pngopt.h"
#include <stdio.h>
#include <string>
#include <map>
#include <vector>
#include <algorithm>
#include "Timing.h"

extern "C" {
#include "jpeglib.h"
#include "setjmp.h"
}

int read_JPEG_file (const char * filename, int &width, int &height, unsigned char *&image_buffer);
GLOBAL(void)
write_JPEG_file (char * filename, int quality, int width, int height, unsigned char *image_buffer, int component);
void write32png(char *filename, unsigned char *imageBuffer, int width, int height);
void PrintUsage();
void writeFastPng(char *filename, unsigned char *imageBuffer, int width, int height);
void Downscale(int inputWidth, int inputHeight, unsigned char *InputBuffer, int Width, int Height, unsigned char *ResultBuffer);

int _tmain(int argc, _TCHAR* argv[])
{

	unsigned char *ImageBuffer = NULL;
	int Width; 
	int Height;
	int JpegSaveQuality;

	if (argc <= 2)
	{
		PrintUsage();
		return 0;
	}

	std::string task = argv[1];

	if (task == "compress" && argc == 5)
	{
		JpegSaveQuality = atoi(argv[4]);
		if (png_texture_load(argv[2], &Width, &Height, ImageBuffer) != 0)
		{
			unsigned char *rgb = new unsigned char[Width * Height * 3];
			unsigned char *alpha = new unsigned char[Width * Height];
			for (int i = 0; i < Width; ++i)
			{
				for (int j = 0; j < Height; ++j)
				{
					rgb[(i + j * Width) * 3] = ImageBuffer[(i + j * Width) * 4];
					rgb[(i + j * Width) * 3 + 1] = ImageBuffer[(i + j * Width) * 4 + 1];
					rgb[(i + j * Width) * 3 + 2] = ImageBuffer[(i + j * Width) * 4 + 2];
					alpha[i + j * Width] = ImageBuffer[(i + j * Width) * 4 + 3];
				}
			}
			char buff[100];
			sprintf(buff, "%s_rgb.jpg", argv[3]);
			write_JPEG_file(buff, JpegSaveQuality, Width, Height, rgb, 3);
			sprintf(buff, "%s_alpha.jpg", argv[3]);
			write_JPEG_file(buff, JpegSaveQuality, Width, Height, alpha, 1);
			delete [] rgb;
			delete [] alpha;
		}
		else
		{
			printf("can't load file: %s\n", argv[2]);
		}
	}
	else if (task == "fastpng" && argc == 4)
	{
		if (png_texture_load(argv[2], &Width, &Height, ImageBuffer) != 0)
		{
			writeFastPng(argv[3], ImageBuffer, Width, Height);
		}
		else
		{
			printf("can't load file: %s\n", argv[2]);
		}
	}
	else if (task == "check" && argc == 5)
	{
		// вызываем для jpg цветного и для градаций серого - порядок вызова не важен
		// функция не делает проверки на совпадение размеров, 
		// просто смотрит сколько компонент в изображении
		// и пишет данные либо в RGB(3 компоненты), либо только в альфа канал(1 компонента)
		read_JPEG_file(argv[2], Width, Height, ImageBuffer);
		read_JPEG_file(argv[3], Width, Height, ImageBuffer);

		write32png(argv[4], ImageBuffer, Width, Height);

	}
	else if (task == "add" && argc == 7) 
	{
		// вызываем для jpg цветного и для градаций серого - порядок вызова не важен
		// функция не делает проверки на совпадение размеров, 
		// просто смотрит сколько компонент в изображении
		// и пишет данные либо в RGB(3 компоненты), либо только в альфа канал(1 компонента)

		read_JPEG_file(argv[2], Width, Height, ImageBuffer); // исходный файл

		int outputWidth = atoi(argv[3]);
		int outputHeight = atoi(argv[4]);

		int addWidth;
		int addHeight;
		unsigned char *addBuffer;
		png_texture_load(argv[5], &addWidth, &addHeight, addBuffer);

		unsigned char *ResultBuffer  = new unsigned char [outputWidth * outputHeight * 4];

		Downscale(Width, Height, ImageBuffer, outputWidth, outputHeight, ResultBuffer);

		unsigned char *rgb = new unsigned char[outputWidth * outputHeight * 3];
		for (int i = 0; i < outputWidth; ++i)
		{
			for (int j = 0; j < outputHeight; ++j)
			{
				rgb[(i + j * outputWidth) * 3 + 0] = ResultBuffer[(i + j * outputWidth) * 4 + 2];
				rgb[(i + j * outputWidth) * 3 + 1] = ResultBuffer[(i + j * outputWidth) * 4 + 1];
				rgb[(i + j * outputWidth) * 3 + 2] = ResultBuffer[(i + j * outputWidth) * 4 + 0];
			}
		}
		for (int i = 0; i < addWidth; ++i)
		{
			for (int j = 0; j < addHeight; ++j)
			{
				float a = addBuffer[(i + j * addWidth) * 4 + 3] / 255.f;
				int src = (i + j * addWidth) * 4;
				unsigned int dst = ((outputWidth - addWidth) + i + (outputHeight - addHeight + j) * outputWidth) * 3;
				rgb[dst + 0] = rgb[dst + 0] * (1 - a) + a * addBuffer[src + 0];
				rgb[dst + 1] = rgb[dst + 1] * (1 - a) + a * addBuffer[src + 1];
				rgb[dst + 2] = rgb[dst + 2] * (1 - a) + a * addBuffer[src + 2];
			}
		}

		JpegSaveQuality = 90;
		write_JPEG_file(argv[6], JpegSaveQuality, outputWidth, outputHeight, rgb, 3);
		//write32png(argv[5], ResultBuffer, inputWidth, inputHeight);
		delete [] addBuffer;
		delete [] ResultBuffer;
		delete [] rgb;

	}
	else if (task == "test" && argc == 5)
	{
		{
			Timing timer;
			timer.StartTiming();
			read_JPEG_file(argv[2], Width, Height, ImageBuffer);
			read_JPEG_file(argv[3], Width, Height, ImageBuffer);
			timer.StopTiming();
			delete [] ImageBuffer;
			printf("2 jpeg loading time: ");
			printf("%.3G seconds.\n", timer.GetUserSeconds());
		}

		{
			Timing timer;
			timer.StartTiming();
			if (png_texture_load(argv[4], &Width, &Height, ImageBuffer) == 0)
			{
				printf("can't load file: %s\n", argv[4]);
			}
			timer.StopTiming();
			printf("png loading time: ");
			printf("%.3G seconds.\n", timer.GetUserSeconds());
		}
	}
	else if (task == "beauty" && argc == 3)
	{
		// beautifull image
		ImageBuffer = new unsigned char[256 * 256 * 3];
		Width = 256; 
		Height = 256;
		JpegSaveQuality = 50;
		for (unsigned int i = 0; i < 256; ++i)
		{
			for (unsigned int j = 0; j < 256; ++j)
			{
				ImageBuffer[(i + j * 256) * 3] = i;
				ImageBuffer[(i + j * 256) * 3 + 1] = 255 - j;
				ImageBuffer[(i + j * 256) * 3 + 2] = 255;
			}
		}
		write_JPEG_file(argv[2], JpegSaveQuality, Width, Height, ImageBuffer, 3);
	}
	else if (task == "noise" && argc == 3)
	{
		// beautifull image
		ImageBuffer = new unsigned char[256 * 256 * 3];
		Width = 256; 
		Height = 256;
		JpegSaveQuality = 50;

		int divWidth = Width;
		int divHeight = Height;



		for (unsigned int i = 0; i < 256; ++i)
		{
			for (unsigned int j = 0; j < 256; ++j)
			{
				ImageBuffer[(i + j * 256) * 3] = i;
				ImageBuffer[(i + j * 256) * 3 + 1] = 255 - j;
				ImageBuffer[(i + j * 256) * 3 + 2] = 255;
			}
		}
		write_JPEG_file(argv[2], JpegSaveQuality, Width, Height, ImageBuffer, 3);
	}
	else if (task == "slice" && argc == 4)
	{
		if (png_texture_load(argv[2], &Width, &Height, ImageBuffer) != 0)
		{
			unsigned char *rc = new unsigned char[Width * Height];
			for (int c = 0; c < 4; ++c)
			{
				for (int i = 0; i < Width; ++i)
				{
					for (int j = 0; j < Height; ++j)
					{
						rc[i + j * Width] = ImageBuffer[(i + j * Width) * 4 + c];
					}
				}
				char buff[100];
				sprintf(buff, "%s_%i.jpg", argv[3], c);
				write_JPEG_file(buff, 100, Width, Height, rc, 1);
			}
		}
		else
		{
			printf("can't load file: %s\n", argv[2]);
		}
	}
	else if (task == "merge" && argc == 7)
	{
		char rgba[4] = {2, 1, 0, 3};
		for (int c = 0; c < 4; ++c)
		{
			read_JPEG_file(argv[2 + rgba[c]], Width, Height, ImageBuffer);
			if (c == 3)
			{
				continue;
			}
			for (int i = 0; i < Width; ++i)
			{
				for (int j = 0; j < Height; ++j)
				{
					ImageBuffer[(i + j * Width) * 4 + c] = ImageBuffer[(i + j * Width) * 4 + 3];
				}
			}
		}
		write32png(argv[6], ImageBuffer, Width, Height);
	}
	else if (task == "crop" && argc == 3)
	{
		int pitch;
		if (png_texture_load(argv[2], &Width, &Height, ImageBuffer, &pitch) != 0)
		{
			bool exit = false;
			for (int i = 0; i < Width && !exit; ++i)
			{
				for (int j = 0; j < Height && !exit; ++j)
				{
					if (ImageBuffer[i * 4 + j * pitch + 3] != 0)
					{
						exit = true;
					}
				}
				if (exit)
				{
					printf("left: %i", i - 1);
				}
			}
			if (exit)
			{
				bool exit = false;
				for (int j = 0; j < Height && !exit; ++j)
				{
					for (int i = 0; i < Width && !exit; ++i)
					{
						if (ImageBuffer[i * 4 + j * pitch + 3] != 0)
						{
							exit = true;
						}
					}
					if (exit)
					{
						printf("\ntop: %i", j - 1);
					}
				}
				if (!exit)
				{
					printf("Error: wrong texture!\n");
				}
			}
			else
			{
				printf("Error: wrong texture!\n");
			}
		}
		else
		{
			printf("can't load file: %s\n", argv[2]);
		}
	}
	else
	{
		PrintUsage();
		return 0;
	}

	delete [] ImageBuffer;

	return 0;
}

void PrintUsage()
{
	printf("usage:\n2JpegToGRBA compress <rgba_png> <output_base_name> <Quality> (save PNG as Dowble JGP)\n");
	printf("2JpegToGRBA check <rgb_jpeg> <alpha(grayscaled)_jpeg> <output_png> (save Dowble JGP as PNG)\n");
	printf("2JpegToGRBA test <rgb_jpeg> <alpha(grayscaled)_jpeg> <rgba_png> (loading speed test: dowble JPG vs PNG)\n");
	printf("2JpegToGRBA beauty <output_jpeg> (create beautifull image)\n");
	printf("2JpegToGRBA slice <rgba_png> <output_base_name> (slice RGBA png to 4 channal grayscale)\n");
	printf("2JpegToGRBA merge <channal0_jpg> <channal1_jpg> <channal2_jpg> <channal3_jpg> <output_png> (merge 4 grayscaled channal \"RGBA\" to one RGBA_png)\n");
	printf("2JpegToGRBA crop <rgba_png> (calculate cropping values)\n");
}

void write32png(char *filename, unsigned char *imageBuffer, int width, int height)
{
	FILE *fp = fopen(filename, "wb");
	if(!fp)
	{
		printf("Can't write to file.\n");
		return;
	}

	if(!Write32BitPNGWithPitch(fp, imageBuffer, true, width, height, width))
	{
		printf("Error writing data.\n");
	}
	else
	{
		printf("Ok\n");
	}

	fclose(fp);
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

int read_JPEG_file (const char * filename, int &width, int &height, unsigned char *&image_buffer)
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

    width = cinfo.output_width;
    height = cinfo.output_height;

	if (image_buffer == NULL)
	{
	    image_buffer = new unsigned char [cinfo.output_width * cinfo.output_height * 4];
	}
    long counter = 0;
	
	buffer[0] = (JSAMPROW)malloc(sizeof(JSAMPLE) * row_stride);

   //step 6, read the image line by line
	unsigned int imageBytes = 0;

	if (cinfo.output_components == 3)
	{
		while (cinfo.output_scanline < cinfo.output_height) {
			jpeg_read_scanlines(&cinfo, buffer, 1);
			for (int i = 0; i < row_stride; i += 3)
			{
				image_buffer[imageBytes] = buffer[0][i + 2];
				image_buffer[imageBytes + 1] = buffer[0][i + 1];
				image_buffer[imageBytes + 2] = buffer[0][i];
				imageBytes += 4;
			}
			counter += row_stride;
		}	
	}
	else if (cinfo.output_components == 1)
	{
		while (cinfo.output_scanline < cinfo.output_height) {
			jpeg_read_scanlines(&cinfo, buffer, 1);
			for (unsigned int i = 0; i < row_stride; ++i)
			{
				image_buffer[imageBytes + 3] = buffer[0][i];
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
write_JPEG_file (char * filename, int quality, int width, int height, unsigned char *image_buffer, int component)
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
  cinfo.image_width = width; 	/* image width and height, in pixels */
  cinfo.image_height = height;
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
  row_stride = width * cinfo.input_components;	/* JSAMPLEs per row in image_buffer */

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

void ReadBuffer(unsigned char *dest, unsigned char *src, int x, int y, int width)
{
	for (int j = 0; j < 16; ++j)
	{
		memcpy(&(dest[j * 16 * 4]), &(src[(x + (y * width)) * 4]), 16 * 4);
	}
}

void writeFastPng(char *filename, unsigned char *imageBuffer, int width, int height)
{
	FILE *file = fopen(filename, "wb");
	FILE *log = fopen("fpng.log", "wt");

	unsigned char buffer[16 * 16 * 4];
	std::map<DWORD, std::pair<unsigned int, unsigned int> > colorMap;
	DWORD color;
	std::vector<unsigned int> maxColors;
	std::vector<unsigned int> masks;

	for (int x = 0; x < width; x += 16)
	{
		for (int y = 0; y < height; y += 16)
		{
			colorMap.clear();
			ReadBuffer(buffer, imageBuffer, x, y, width);

			for (unsigned int i = 0; i < 16; ++i)
			{
				for (unsigned int j = 0; j < 16; ++j)
				{
					memcpy(&color, &(buffer[(i + j * 16) * 4]), 4);
					std::map<DWORD, std::pair<unsigned int, unsigned int> >::iterator iter = colorMap.find(color);
					if (iter == colorMap.end())
					{
						colorMap[color].first = 1;
					}
					else
					{
						++(iter->second.first);
						//iter->second.second = 
					}
				}
			}
			maxColors.push_back(colorMap.size());
		}
	}

	fprintf(log, "%i\n", maxColors.size());

	std::sort(maxColors.rbegin(), maxColors.rend());
	for (unsigned int i = 0; i < maxColors.size(); ++i)
	{
		fprintf(log, "%i\n", maxColors[i]);
	}

	fclose(log);
	fclose(file);
}

void Downscale(int inputWidth, int inputHeight, unsigned char *InputBuffer, int Width, int Height, unsigned char *ResultBuffer)
{
	float *tmp = new float[Width * Height * 4];
	float *mass = new float[Width * Height];
	memset(tmp, 0, Width * Height * 4 * sizeof(float));
	memset(mass, 0, Width * Height * sizeof(float));

	float kx = float(Width) / inputWidth;
	float ky = float(Height) / inputHeight;

	for (int i = 0; i < inputWidth; ++i)
	{
		int di1 = floor(i * kx);
		int di2 = floor((i + 1) * kx);

		float koefXMul = 1.f;
		if (di1 != di2)
			koefXMul = di2 / kx - i;

		if (koefXMul == 1.f)
		{
			for (int j = 0; j < inputHeight; ++j)
			{
				int dj1 = floor(j * ky);
				int dj2 = floor((j + 1) * ky);

				float koefYMul = 1.f;
				if (dj1 != dj2)
					koefYMul = dj2 / ky - j;

				if (koefYMul == 1.f) // pixel inside new pixel
				{
					tmp[(di1 + dj1 * Width) * 4 + 0] += InputBuffer[(i + j * inputWidth) * 4 + 0];
					tmp[(di1 + dj1 * Width) * 4 + 1] += InputBuffer[(i + j * inputWidth) * 4 + 1];
					tmp[(di1 + dj1 * Width) * 4 + 2] += InputBuffer[(i + j * inputWidth) * 4 + 2];

					mass[di1 + dj1 * Width] += 1.f;
				}
				else // pixel divided by vertical
				{
					tmp[(di1 + dj1 * Width) * 4 + 0] += InputBuffer[(i + j * inputWidth) * 4 + 0] * koefYMul;
					tmp[(di1 + dj1 * Width) * 4 + 1] += InputBuffer[(i + j * inputWidth) * 4 + 1] * koefYMul;
					tmp[(di1 + dj1 * Width) * 4 + 2] += InputBuffer[(i + j * inputWidth) * 4 + 2] * koefYMul;
					
					mass[di1 + dj1 * Width] += koefYMul;
					
					tmp[(di1 + dj2 * Width) * 4 + 0] += InputBuffer[(i + j * inputWidth) * 4 + 0] * (1.f - koefYMul);
					tmp[(di1 + dj2 * Width) * 4 + 1] += InputBuffer[(i + j * inputWidth) * 4 + 1] * (1.f - koefYMul);
					tmp[(di1 + dj2 * Width) * 4 + 2] += InputBuffer[(i + j * inputWidth) * 4 + 2] * (1.f - koefYMul);

					mass[di1 + dj2 * Width] += (1.f - koefYMul);
				}
			}
		}
		else
		{
			for (int j = 0; j < inputHeight; ++j)
			{
				int dj1 = floor(j * ky);
				int dj2 = floor((j + 1) * ky);

				float koefYMul = 1.f;
				if (dj1 != dj2)
					koefYMul = dj2 / ky - j;

				if (koefYMul == 1.f) // pixel devided by horizontal
				{
					tmp[(di1 + dj1 * Width) * 4 + 0] += InputBuffer[(i + j * inputWidth) * 4 + 0] * koefXMul;
					tmp[(di1 + dj1 * Width) * 4 + 1] += InputBuffer[(i + j * inputWidth) * 4 + 1] * koefXMul;
					tmp[(di1 + dj1 * Width) * 4 + 2] += InputBuffer[(i + j * inputWidth) * 4 + 2] * koefXMul;

					mass[di1 + dj1 * Width] += koefXMul;

					tmp[(di2 + dj1 * Width) * 4 + 0] += InputBuffer[(i + j * inputWidth) * 4 + 0] * (1.f - koefXMul);
					tmp[(di2 + dj1 * Width) * 4 + 1] += InputBuffer[(i + j * inputWidth) * 4 + 1] * (1.f - koefXMul);
					tmp[(di2 + dj1 * Width) * 4 + 2] += InputBuffer[(i + j * inputWidth) * 4 + 2] * (1.f - koefXMul);

					mass[di2 + dj1 * Width] += (1.f - koefXMul);
				}
				else // pixel devided by horizontal and vertical
				{
					tmp[(di1 + dj1 * Width) * 4 + 0] += InputBuffer[(i + j * inputWidth) * 4 + 0] * koefXMul * koefYMul;
					tmp[(di1 + dj1 * Width) * 4 + 1] += InputBuffer[(i + j * inputWidth) * 4 + 1] * koefXMul * koefYMul;
					tmp[(di1 + dj1 * Width) * 4 + 2] += InputBuffer[(i + j * inputWidth) * 4 + 2] * koefXMul * koefYMul;

					mass[di1 + dj1 * Width] += (koefXMul * koefYMul);

					tmp[(di2 + dj1 * Width) * 4 + 0] += InputBuffer[(i + j * inputWidth) * 4 + 0] * (1.f - koefXMul) * koefYMul;
					tmp[(di2 + dj1 * Width) * 4 + 1] += InputBuffer[(i + j * inputWidth) * 4 + 1] * (1.f - koefXMul) * koefYMul;
					tmp[(di2 + dj1 * Width) * 4 + 2] += InputBuffer[(i + j * inputWidth) * 4 + 2] * (1.f - koefXMul) * koefYMul;

					mass[di2 + dj1 * Width] += ((1.f - koefXMul) * koefYMul);

					tmp[(di1 + dj2 * Width) * 4 + 0] += InputBuffer[(i + j * inputWidth) * 4 + 0] * koefXMul * (1.f - koefYMul);
					tmp[(di1 + dj2 * Width) * 4 + 1] += InputBuffer[(i + j * inputWidth) * 4 + 1] * koefXMul * (1.f - koefYMul);
					tmp[(di1 + dj2 * Width) * 4 + 2] += InputBuffer[(i + j * inputWidth) * 4 + 2] * koefXMul * (1.f - koefYMul);

					mass[di1 + dj2 * Width] += (koefXMul * (1.f - koefYMul));

					tmp[(di2 + dj2 * Width) * 4 + 0] += InputBuffer[(i + j * inputWidth) * 4 + 0] * (1.f - koefXMul) * (1.f - koefYMul);
					tmp[(di2 + dj2 * Width) * 4 + 1] += InputBuffer[(i + j * inputWidth) * 4 + 1] * (1.f - koefXMul) * (1.f - koefYMul);
					tmp[(di2 + dj2 * Width) * 4 + 2] += InputBuffer[(i + j * inputWidth) * 4 + 2] * (1.f - koefXMul) * (1.f - koefYMul);

					mass[di2 + dj2 * Width] += ((1.f - koefXMul) * (1.f - koefYMul));
				}
			}
		}
	}
	for (int j = 0; j < Height; ++j)
	{
		for (int i = 0; i < Width; ++i)
		{
			ResultBuffer[(i + j * Width) * 4 + 0] = tmp[(i + j * Width) * 4 + 0] / mass[i + j * Width];
			ResultBuffer[(i + j * Width) * 4 + 1] = tmp[(i + j * Width) * 4 + 1] / mass[i + j * Width];
			ResultBuffer[(i + j * Width) * 4 + 2] = tmp[(i + j * Width) * 4 + 2] / mass[i + j * Width];
			ResultBuffer[(i + j * Width) * 4 + 3] = 255;
		}
	}
	delete [] tmp;
	delete [] mass;
}