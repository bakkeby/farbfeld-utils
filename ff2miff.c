#if 0
gcc -s -O2 -o ~/bin/ff2miff -Wno-unused-result ff2miff.c
exit
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lodepng.h"

static unsigned char buf[16];
static int width,height;

static void
usage(void)
{
	fprintf(stderr,
		"Usage: ff2miff                                                                \n"
		"                                                                              \n"
		"Convert farbfeld image to ImageMagick format (MIFF). ff2miff reads a farbfeld \n"
		"image from stdin, converts it to MIFF and writes the result to stdout.        \n"
		"                                                                              \n"
		"Magick Image File Format (MIFF) is a raster image file format associated with \n"
		"the image processing software ImageMagick. It was originally named Machine    \n"
		"Independent File Format.                                                      \n"
		"                                                                              \n"
		"It is designed to support all of the metadata and pixel formats supported     \n"
		"internally by ImageMagick. For example, it supports floating-point samples and\n"
		"high dynamic range.                                                           \n"
		"                                                                              \n"
		"There is no formal MIFF specification. Each copy of the ImageMagick software  \n"
		"includes a copy of the MIFF documentation, and each new version of ImageMagick\n"
		"may add new features to the format.                                           \n"
		"                                                                              \n"
		"Despite the potential instability and ambiguity of the format, it is portable \n"
		"enough to be usable by other applications. But it is primarily useful for     \n"
		"temporary files, and should be avoided for long term storage or public        \n"
		"distribution.                                                                 \n"
		"                                                                              \n"
		"Example usage:                                                                \n"
		"   $ ff2miff < image.ff > image.miff                                          \n"
		"   $ bunzip2 < image.ff.bz2 | ff2miff > image.miff                            \n"
		"\n"
	);
	exit(1);
}

int
main(int argc, char **argv)
{
	int n;

	if (argc > 1) {
		usage();
	}

	fread(buf,1,16,stdin);
	if (memcmp("farbfeld",buf,8)) {
		fprintf(stderr,"Not farbfeld\n");
		return 1;
	}
	width=(buf[8]<<24)|(buf[9]<<16)|(buf[10]<<8)|buf[11];
	height=(buf[12]<<24)|(buf[13]<<16)|(buf[14]<<8)|buf[15];
	printf("id=ImageMagick\ndepth=16\nmatte=True\ncolumns=%d\nrows=%d\n\f\n:\x1A",width,height);
	n=width*height;
	while (n--) fread(buf,1,8,stdin),fwrite(buf,1,8,stdout);
	return 0;
}
