#if 0
gcc -s -O2 -o ~/bin/mac2ff -Wno-unused-result mac2ff.c
exit
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned char buf[512];
static int datacount=51840;

static void
usage(void)
{
	fprintf(stderr,
		"Usage: mac2ff                                                                 \n"
		"                                                                              \n"
		"Convert MacPaint image to farbfeld. mac2ff reads a MacPaint image from stdin, \n"
		"converts it to farbfeld and writes the result to stdout.                      \n"
		"                                                                              \n"
		"MacPaint is the native image format of the MacPaint paint program for Apple   \n"
		"Macintosh.                                                                    \n"
		"                                                                              \n"
		"Images are always bi-level, 576×720 pixels in size, and use PackBits          \n"
		"compression.                                                                  \n"
		"                                                                              \n"
		"Example usage:                                                                \n"
		"   $ mac2ff < image.mac > image.ff                                            \n"
		"   $ mac2ff < image.mac | bzip2 > image.ff.bz2                                \n"
		"\n"
	);
	exit(1);
}

static void
out_byte(int x)
{
	fwrite(buf+(x&0x80?8:0),1,8,stdout);
	fwrite(buf+(x&0x40?8:0),1,8,stdout);
	fwrite(buf+(x&0x20?8:0),1,8,stdout);
	fwrite(buf+(x&0x10?8:0),1,8,stdout);
	fwrite(buf+(x&0x08?8:0),1,8,stdout);
	fwrite(buf+(x&0x04?8:0),1,8,stdout);
	fwrite(buf+(x&0x02?8:0),1,8,stdout);
	fwrite(buf+(x&0x01?8:0),1,8,stdout);
	--datacount;
}

static inline void
process(void)
{
	int n=fgetc(stdin);
	int x;
	if (n>=0 && n<128) {
		// Heterogeneous run
		n++;
		while (n--) out_byte(fgetc(stdin));
	} else if (n>128 && n<256) {
		// Homogeneous run
		x=fgetc(stdin);
		n=257-n;
		while (n--) out_byte(x);
	}
}

int
main(int argc, char **argv)
{
	if (argc > 1) {
		usage();
	}

	fread(buf,1,128,stdin);
	// This attempts to detect a MacBinary header. Is this OK?
	if (!buf[0] && buf[1] && buf[1]<64 && !buf[74] && !buf[82]) fread(buf,1,128,stdin);
	fread(buf,1,512-128,stdin);
	fwrite("farbfeld\0\0\x02\x40\0\0\x02\xD0",1,16,stdout);
	buf[6]=buf[7]=buf[14]=buf[15]=255;
	buf[0]=buf[1]=buf[2]=buf[3]=buf[4]=buf[5]=255;
	buf[8]=buf[9]=buf[10]=buf[11]=buf[12]=buf[13]=0;
	while (datacount>0 && !feof(stdin)) process();
	return 0;
}
