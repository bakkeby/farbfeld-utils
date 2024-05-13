#if 0
gcc -s -O2 -o ~/bin/avs2ff -Wno-unused-result avs2ff.c
exit
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void
usage(void)
{
	fprintf(stderr,
		"Usage: avs2ff                                                                 \n"
		"                                                                              \n"
		"Convert Stardent AVS X image to farbfeld. avs2ff reads an AVS image from      \n"
		"stdin, converts it to farbfeld and writes the result to stdout.               \n"
		"                                                                              \n"
		"The program takes no arguments.                                               \n"
		"                                                                              \n"
		"Example usage:                                                                \n"
		"   $ avs2ff < image.avs > image.ff                                            \n"
		"   $ avs2ff < image.avs | bzip2 > image.ff.bz2                                \n"
		"\n"
	);
	exit(1);
}

int
main(int argc, char **argv)
{
	unsigned char buf[8] = {0};

	if (argc > 1) {
		usage();
	}

	fread(buf,1,8,stdin);
	fwrite("farbfeld",1,8,stdout);
	fwrite(buf,1,8,stdout);
	while(fread(buf,1,4,stdin) > 0) {
		putchar(buf[1]); putchar(buf[1]);
		putchar(buf[2]); putchar(buf[2]);
		putchar(buf[3]); putchar(buf[3]);
		putchar(buf[0]); putchar(buf[0]); // is 0 or 255 opaque?
	}
	return 0;
}
