#if 0
gcc -s -O2 -o ~/bin/bmp2ff -Wno-unused-result bmp2ff.c
exit
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_STATIC
#define STBI_ONLY_BMP
#include "stb_image.h"

static unsigned char *pic;
static int width,height,i;

static void
usage(void)
{
	fprintf(stderr,
		"Usage: bmp2ff                                                                 \n"
		"                                                                              \n"
		"Convert BMP (bitmap) image to farbfeld. bmp2ff reads an BMP image from stdin, \n"
		"converts it to farbfeld and writes the result to stdout.                      \n"
		"                                                                              \n"
		"BMP is a raster graphics image format capable of storing two-dimensional      \n"
		"digital images in varous colour depths.                                       \n"
		"There is currently no support for BMP images with RLE (Run Length Encoding)   \n"
		"compression                                                                   \n"
		"                                                                              \n"
		"The program takes no arguments.                                               \n"
		"                                                                              \n"
		"Example usage:                                                                \n"
		"   $ bmp2ff < image.bmp > image.ff                                            \n"
		"   $ bmp2ff < image.bmp | bzip2 > image.ff.bz2                                \n"
		"\n"
	);
	exit(1);
}

int
main(int argc, char **argv)
{
	if (argc > 1) {
		usage();
	}

	pic=stbi_load_from_file(stdin,&width,&height,&i,4);
	if (!pic || !width || !height) {
		fprintf(stderr,"Error: %s\n",stbi_failure_reason()?:"Unknown error");
		return 1;
	}
	fwrite("farbfeld",1,8,stdout);
	putchar(width>>24);
	putchar(width>>16);
	putchar(width>>8);
	putchar(width>>0);
	putchar(height>>24);
	putchar(height>>16);
	putchar(height>>8);
	putchar(height>>0);
	i=width*height*4;
	while(i--) putchar(*pic),putchar(*pic++);
	return 0;
}
