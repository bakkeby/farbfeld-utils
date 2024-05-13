#if 0
gcc -s -O2 -o ~/bin/qoi2ff -Wno-unused-result qoi2ff.c
exit
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
	unsigned char d[4];
} Color;

static void
usage(void)
{
	fprintf(stderr,
		"Usage: qoi2ff                                                                 \n"
		"                                                                              \n"
		"Convert Quite OK Image Format (QOI) to farbfeld. qoi2ff reads a QOI image from\n"
		"stdin, converts it to farbfeld and writes the result to stdout.               \n"
		"                                                                              \n"
		"Quite OK Image Format (QOI) is a raster image file format. It was developed by\n"
		"Dominic Szablewski.                                                           \n"
		"                                                                              \n"
		"Its compression scheme (RLE with some bells and whistles) is designed to be   \n"
		"faster and simpler than that of formats like PNG, while having a file size    \n"
		"that is only moderately larger.                                               \n"
		"                                                                              \n"
		"The program takes no arguments.                                               \n"
		"                                                                              \n"
		"Example usage:                                                                \n"
		"   $ qoi2ff < image.qoi > image.ff                                            \n"
		"   $ qoi2ff < image.qoi | bzip2 > image.ff.bz2                                \n"
		"\n"
	);
	exit(1);
}

static inline void
send(Color c)
{
	putchar(c.d[0]);
	putchar(c.d[0]);
	putchar(c.d[1]);
	putchar(c.d[1]);
	putchar(c.d[2]);
	putchar(c.d[2]);
	putchar(c.d[3]);
	putchar(c.d[3]);
}

int
main(int argc, char **argv)
{
	static Color last={{0,0,0,255}};
	static Color ind[64];
	static unsigned char buf[16];
	long t;
	int c,v;

	if (argc > 1) {
		usage();
	}

	fread(buf,1,14,stdin);
	if (memcmp(buf,"qoif",4) || buf[12]<3 || buf[12]>4 || buf[13]>1) {
		fprintf(stderr,"Not a valid qoif file\n");
		return 1;
	}
	fwrite("farbfeld",1,8,stdout);
	fwrite(buf+4,1,8,stdout);
	t=(buf[4]<<24)|(buf[5]<<16)|(buf[6]<<8)|buf[7];
	t*=(buf[8]<<24)|(buf[9]<<16)|(buf[10]<<8)|buf[11];
	while (t-->0) {
		c=getchar();
		if (c<0x40) {
			last=ind[c];
		} else if (c<0x80) {
			last.d[0]+=((c>>4)&3)-2;
			last.d[1]+=((c>>2)&3)-2;
			last.d[2]+=((c>>0)&3)-2;
		} else if (c<0xC0) {
			c=(c&0x3F)-32;
			v=getchar();
			last.d[0]+=c+((v>>4)&15)-8;
			last.d[1]+=c;
			last.d[2]+=c+(v&15)-8;
		} else if (c<0xFE) {
			t-=c&=0x3F;
			while (c--) send(last);
		} else {
			last.d[0]=getchar();
			last.d[1]=getchar();
			last.d[2]=getchar();
			if (c==0xFF) last.d[3]=getchar();
		}
		ind[(3*last.d[0]+5*last.d[1]+7*last.d[2]+11*last.d[3])&63]=last;
		send(last);
	}
	return 0;
}
