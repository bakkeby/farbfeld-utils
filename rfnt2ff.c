#if 0
gcc -s -O2 -o ~/bin/rfnt2ff -Wno-unused-result rfnt2ff.c
exit
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const unsigned char pal[24] = {255,255,255,255,255,255,255,255,0,0,0,0,0,0,255,255,0,0,0,0,0,0,0,0};

static unsigned char buf[4];
static char used[256];
static unsigned char*data;

static void
usage(void)
{
	fprintf(stderr,
		"Usage: rfnt2ff                                                                \n"
		"                                                                              \n"
		"Convert RawFont image format to farbfeld. rfnt2ff reads a RawFont image from  \n"
		"stdin, converts it to farbfeld and writes the result to stdout.               \n"
		"                                                                              \n"
		"Origin or context of the image format is not known.                           \n"
		"                                                                              \n"
		"The program takes no arguments.                                               \n"
		"                                                                              \n"
		"Example usage:                                                                \n"
		"   $ rfnt2ff < image.rfnt > image.ff                                          \n"
		"   $ rfnt2ff < image.rfnt | bzip2 > image.ff.bz2                              \n"
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

	int i,j,k,w,h,z;
	unsigned char*p;
	fread(buf,1,4,stdin);
	if (memcmp(buf,"RFNT",4)) {
		fprintf(stderr,"Bad magic\n");
		return 1;
	}
	w=fgetc(stdin);
	h=fgetc(stdin);
	if (w<=0 || h<=0) {
		fprintf(stderr,"Incorrect dimensions\n");
		return 1;
	}
	data=calloc(256,z=(w*h+7)/8);
	if (!data) {
		fprintf(stderr,"Allocation failed\n");
		return 1;
	}
	for (;;) {
		i=fgetc(stdin);
		if(i==EOF) break;
		fread(data+z*i,1,z,stdin);
		used[i]=1;
	}
	fwrite("farbfeld\0\0",1,11,stdout);
	putchar(w); putchar(0); putchar(0); putchar(h); putchar(0);
	for (i=0; i<256; i++) {
		if (used[i]) {
			p=data+z*i;
			for (j=0; j<w*h; j++) {
				if (!(j&7)) k=*p++;
				fwrite(pal+(k&128?8:0),1,8,stdout);
				k<<=1;
			}
		} else {
			for (j=0; j<w*h; j++) fwrite(pal+16,1,8,stdout);
		}
	}
	return 0;
}
