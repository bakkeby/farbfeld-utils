#if 0
gcc -s -O2 -o ~/bin/ff2avs -Wno-unused-result ff2avs.c
exit
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void
usage(void)
{
	fprintf(stderr,
		"Usage: ff2avs                                                                 \n"
		"                                                                              \n"
		"Convert farbfeld to Stardent AVS X image. ff2avs reads a farbfeld image from  \n"
		"stdin, converts it to AVS (8-bit RBGA) and writes the result to stdout.       \n"
		"                                                                              \n"
		"Example usage:                                                                \n"
		"   $ ff2avs < image.ff > image.avs                                            \n"
		"   $ bunzip2 < image.ff.bz2 | ff2avs > image.avs                              \n"
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
	if(memcmp(buf,"farbfeld",8)) {
		fprintf(stderr,"Not farbfeld\n");
		return 1;
	}
	fread(buf,1,8,stdin);
	fwrite(buf,1,8,stdout);
	while(fread(buf,1,8,stdin)>0) {
		putchar(buf[6]);
		putchar(buf[0]);
		putchar(buf[2]);
		putchar(buf[4]);
	}
	return 0;
}
