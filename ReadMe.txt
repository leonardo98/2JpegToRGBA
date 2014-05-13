========================================================================
    CONSOLE APPLICATION : 2JpegToRGBA Project Overview
========================================================================

Jpeg does not support alpha channel, that is why this utility was written.

usage:
2JpegToGRBA compress <rgba_png> <output_base_name> <Quality>
or
2JpegToGRBA check <rgb_jpeg> <alpha(grayscaled)_jpeg> <output_png>


1. 2JpegToGRBA compress <rgba_png> <output_base_name> <Quality>
	It takes rgba_png and split it in 2 jpeg - rgb and alpha(grayscaled) <Quality> - jpeg quality for both.

2. JpegToGRBA check <rgb_jpeg> <alpha(grayscaled)_jpeg> <output_png>
	It takes 2 Jpeg images: rgb_jpeg and alpha(grayscaled)_jpeg and create RGBA image (it saved as output_png).


Later it will be tested for loading PNG vs Jpeg(colored and grayscaled).

/////////////////////////////////////////////////////////////////////////////
