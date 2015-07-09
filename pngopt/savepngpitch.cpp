#include "pngopt.h"
#include "libPNG/png.h"

class PNGError {};

void WarningCallback(png_structp png_ptr,
					 png_const_charp msg)
{
  //  LOG(std::string("LIBPNG Warning: ") + msg);
}

void ErrorCallback(png_structp png_ptr,
				   png_const_charp msg)
{
    //LOG(std::string("LIBPNG Error: ") + msg);
	throw PNGError();
}

bool Write32BitPNGWithPitch(FILE* fp, void* pBits, bool bNeedAlpha, int nWidth, int nHeight, int nPitch)
{
	png_structp png_ptr = 0;
	png_infop info_ptr = 0;
	try 
	{
		//	create png structs
		png_ptr = png_create_write_struct
			(PNG_LIBPNG_VER_STRING, 0,
			ErrorCallback, WarningCallback);
		if (png_ptr == 0) return false;

		info_ptr = png_create_info_struct(png_ptr);
		if (info_ptr == 0) return false;

		//	init i/o
		png_init_io(png_ptr, fp);

		//	setup params
		if ( bNeedAlpha )
		{
			png_set_IHDR( png_ptr, info_ptr, nWidth, nHeight, 
				8, 
				PNG_COLOR_TYPE_RGB_ALPHA, 
				PNG_INTERLACE_NONE,
				PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
		}
		else
		{
			png_set_IHDR( png_ptr, info_ptr, nWidth, nHeight, 
				8, 
				PNG_COLOR_TYPE_RGB,
				PNG_INTERLACE_NONE,
				PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
		}

		png_write_info(png_ptr, info_ptr);
	
		if ( !bNeedAlpha )
			png_set_filler(png_ptr, 0, PNG_FILLER_AFTER);	//	strip alpha
		
		png_set_bgr(png_ptr);	//	switch to little-endian notation

		//	writing
		unsigned char* pSrc = (unsigned char*)pBits;
		for ( int i = 0; i < nHeight; i++, pSrc += nPitch*4 )
		{
			png_write_row( png_ptr, pSrc );
		}

		png_write_end(png_ptr, info_ptr);
	} 
	catch(PNGError)
	{
		png_destroy_write_struct(&png_ptr, (info_ptr == 0) ? NULL : &info_ptr);
		return false;
	}

	//	cleanup
	png_destroy_write_struct( &png_ptr, (info_ptr == 0) ? NULL : &info_ptr);
	return true;
}

int png_texture_load(const char * file_name, int * width, int * height, unsigned char *&imageData, int * pitch)
{
    png_byte header[8];

    FILE *fp = fopen(file_name, "rb");
    if (fp == 0)
    {
        perror(file_name);
        return 0;
    }

    // read the header
    fread(header, 1, 8, fp);

    if (png_sig_cmp(header, 0, 8))
    {
        fprintf(stderr, "error: %s is not a PNG.\n", file_name);
        fclose(fp);
        return 0;
    }

    png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png_ptr)
    {
        fprintf(stderr, "error: png_create_read_struct returned 0.\n");
        fclose(fp);
        return 0;
    }

    // create png info struct
    png_infop info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr)
    {
        fprintf(stderr, "error: png_create_info_struct returned 0.\n");
        png_destroy_read_struct(&png_ptr, (png_infopp)NULL, (png_infopp)NULL);
        fclose(fp);
        return 0;
    }

    // create png info struct
    png_infop end_info = png_create_info_struct(png_ptr);
    if (!end_info)
    {
        fprintf(stderr, "error: png_create_info_struct returned 0.\n");
        png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp) NULL);
        fclose(fp);
        return 0;
    }

    // the code in this if statement gets called if libpng encounters an error
    if (setjmp(png_jmpbuf(png_ptr))) {
        fprintf(stderr, "error from libpng\n");
        png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
        fclose(fp);
        return 0;
    }

    // init png reading
    png_init_io(png_ptr, fp);

    // let libpng know you already read the first 8 bytes
    png_set_sig_bytes(png_ptr, 8);

    // read all the info up to the image data
    png_read_info(png_ptr, info_ptr);

    // variables to pass to get info
    int bit_depth, color_type;
    png_uint_32 temp_width, temp_height;

    // get info about png
    png_get_IHDR(png_ptr, info_ptr, &temp_width, &temp_height, &bit_depth, &color_type,
        NULL, NULL, NULL);

    if (width){ *width = temp_width; }
    if (height){ *height = temp_height; }

    // Update the png info struct.
    png_read_update_info(png_ptr, info_ptr);

    // Row size in bytes.
    int rowbytes = png_get_rowbytes(png_ptr, info_ptr);

    // glTexImage2d requires rows to be 4-byte aligned
    rowbytes += 3 - ((rowbytes-1) % 4);

    // Allocate the image_data as a big block, to be given to opengl
    png_byte * image_data;
    image_data = (png_byte *)malloc(rowbytes * temp_height * sizeof(png_byte)+15);
    if (image_data == NULL)
    {
        fprintf(stderr, "error: could not allocate memory for PNG image data\n");
        png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
        fclose(fp);
        return 0;
    }

    // row_pointers is for pointing to image_data for reading the png with libpng
    png_bytep * row_pointers = (png_bytep *)malloc(temp_height * sizeof(png_bytep));
    if (row_pointers == NULL)
    {
        fprintf(stderr, "error: could not allocate memory for PNG row pointers\n");
        png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
        free(image_data);
        fclose(fp);
        return 0;
    }

    // set the individual row_pointers to point at the correct offsets of image_data
    int i;
    for (i = 0; i < temp_height; i++)
    {
        row_pointers[i] = image_data + i * rowbytes;
    }

    // read the png into image_data through row_pointers
    png_read_image(png_ptr, row_pointers);

     // clean up
    png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
    //free(image_data);
	imageData = (unsigned char *)image_data;
    free(row_pointers);
    fclose(fp);

	if (pitch)
	{
		*pitch = rowbytes;
	}
    return 1;
}