#if 0
gcc -s -O2 -o ~/bin/jpeg2ff -Wno-unused-result jpeg2ff.c
exit
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_STATIC
#define STBI_ONLY_JPEG
#include "stb_image.h"

static unsigned char*pic;
static int width,height,i;

static void
usage(void)
{
	fprintf(stderr,
		"Usage: jpeg2ff                                                                \n"
		"                                                                              \n"
		"Converts JPEG (Joint Photographic Experts Group) image to farbfeld. jpeg2ff   \n"
		"reads a JPEG image from stdin, converts that to farbfeld and writes the       \n"
		"result to stdout.                                                             \n"
		"                                                                              \n"
		"jpeg2ff does not support 12-bits encoding and it does not support arithmetic  \n"
		"coding.                                                                       \n"
		"                                                                              \n"
		"This decoder takes no arguments.                                              \n"
		"                                                                              \n"
		"Example usage:                                                                \n"
		"   $ jpeg2ff < image.jpg > image.ff                                           \n"
		"   $ jpeg2ff < image.jpg | bzip2 > image.ff.bz2                               \n"
		"\n"
	);
	exit(1);
}

int
main(int argc, char **argv)
{
	if (argc > 1 && (!strcmp(argv[1],"-h") || !strcmp(argv[1],"--help"))) {
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
	while (i--) putchar(*pic),putchar(*pic++);
	return 0;
}
