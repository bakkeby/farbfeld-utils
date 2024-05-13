#if 0
gcc -s -O2 -o ~/bin/ff2nrrd -Wno-unused-result ff2nrrd.c
exit
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned char buf[16];
static int width,height;

static void
usage(void)
{
	fprintf(stderr,
		"Usage: ff2nrrd                                                                \n"
		"                                                                              \n"
		"Converts farbfeld to Nearly Raw Raster Data (NRRD) format. ff2nrrd reads      \n"
		"farbfeld from stdin, converts that to NRRD format and writes the result to    \n"
		"stdout.                                                                       \n"
		"                                                                              \n"
		"NRRD is a file format designed to support scientific visualization and image  \n"
		"processing involving N-dimensional raster data.                               \n"
		"                                                                              \n"
		"Disclaimer: this implementation has not been tested and may not produce the   \n"
		"right output.                                                                 \n"
		"                                                                              \n"
		"This encoder takes no command line arguments.                                 \n"
		"                                                                              \n"
		"Example usage:                                                                \n"
		"   $ ff2nrrd < image.ff > image.nrrd                                          \n"
		"\n"
	);
	exit(1);
}

int main(int argc,char**argv) {
	if (argc > 1 && (!strcmp(argv[1],"-h") || !strcmp(argv[1],"--help"))) {
		usage();
	}
	fread(buf,1,16,stdin);
	if(memcmp(buf,"farbfeld",8)) {
		fprintf(stderr,"Not farbfeld\n");
		return 1;
	}
	width=(buf[8]<<24)|(buf[9]<<16)|(buf[10]<<8)|buf[11];
	height=(buf[12]<<24)|(buf[13]<<16)|(buf[14]<<8)|buf[15];
	printf("NRRD0003\n");
	printf("type: ushort\n");
	printf("endian: big\n");
	printf("dimension: 3\n");
	printf("sizes: 4 %d %d\n",width,height);
	printf("kinds: RGBA-color space space\n");
	printf("encoding: raw\n");
	putchar('\n');
	while(fread(buf,1,8,stdin)>0) fwrite(buf,1,8,stdout);
	return 0;
}

// I am not sure if the above is a correct use of NRRD format; it is untested
