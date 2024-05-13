#if 0
gcc -s -O2 -o ~/bin/gif2ff -Wno-unused-result gif2ff.c
exit
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_STATIC
#define STBI_ONLY_GIF
#include "stb_image.h"

static unsigned char *pic;
static int width, height, i;

static void
usage(void)
{
	fprintf(stderr,
		"Usage: gif2ff                                                                 \n"
		"                                                                              \n"
		"Convert Graphics Interchange Format (GIF) image to farbfeld. gif2ff reads a   \n"
		"GIF image from stdin, converts it to farbfeld and writes the result to stdout.\n"
		"                                                                              \n"
		"The tool takes no arguments and can not load animations; only the first frame \n"
		"is loaded.                                                                    \n"
		"                                                                              \n"
		"Example usage:                                                                \n"
		"   $ gif2ff < image.gif > image.ff                                            \n"
		"   $ gif2ff < image.gif | bzip2 > image.ff.bz2                                \n"
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

	// Should add stuff for consideration of loading animations.
	// Currently it can load only one frame.
	pic=stbi_load_from_file(stdin, &width, &height, &i, 4);
	if (!pic || !width || !height) {
		fprintf(stderr,"Error: %s\n", stbi_failure_reason() ? : "Unknown error");
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
	while (i--) putchar(*pic), putchar(*pic++);
	return 0;
}
