#if 0
gcc -s -O2 -o ~/bin/xyz2ff -Wno-unused-result xyz2ff.c lodepng.c
exit
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lodepng.h"

static unsigned char buf[8];
static unsigned char*cdata;
static size_t clen;
static unsigned char*udata;
static size_t ulen;
static LodePNGDecompressSettings opt;

static void
usage(void)
{
	fprintf(stderr,
		"Usage: xyz2ff                                                                 \n"
		"                                                                              \n"
		"Converts .XYZ picture files from RPG Maker and EasyRPG to farbfeld. xyz2ff    \n"
		"reads a .XYZ picture file from stdin, converts it to farbfeld and writes the  \n"
		"result to stdout.                                                             \n"
		"                                                                              \n"
		"The file format consists of:                                                  \n"
		"   - ASCII text XYZ1                                                          \n"
		"   - Small-endian 16-bit width                                                \n"
		"   - Small-endian 16-bit height                                               \n"
		"   - Zlib compressed data stream                                              \n"
		"                                                                              \n"
		"The zlib stream contains 256 palette entries, followed by the raster data (in \n"
		"the usual progressive television order). The palette entries consist of three \n"
		"bytes each, being red, green, and blue values. Each pixel of raster data is   \n"
		"one byte which is an index into the palette.                                  \n"
		"                                                                              \n"
		"This decoder takes no arguments.                                              \n"
		"                                                                              \n"
		"Example usage:                                                                \n"
		"   $ xyz2ff < image.xyz > image.ff                                            \n"
		"\n"
	);
	exit(1);
}

static void read_data(void) {
	size_t n=0;
	size_t v;
	for(;;) {
		cdata=realloc(cdata,n+=0x8000);
		if(!cdata) {
			fprintf(stderr,"Allocation failed\n");
			exit(1);
		}
		clen+=v=fread(cdata+clen,1,0x8000,stdin);
		if(ferror(stdin)) {
			perror(0);
			exit(1);
		} else if(v!=0x8000) {
			return;
		}
	}
}

int main(int argc,char**argv) {
	int i;
	if (argc > 1) {
		usage();
	}
	fread(buf,1,8,stdin);
	if(memcmp("XYZ1",buf,4)) {
		fprintf(stderr,"Unrecognized file format\n");
		return 1;
	}
	read_data();
	i=lodepng_zlib_decompress(&udata,&ulen,cdata,clen,&opt);
	if(i) {
		fprintf(stderr,"%s\n",lodepng_error_text(i));
		return 1;
	}
	fwrite("farbfeld",1,8,stdout);
	putchar(0); putchar(0); putchar(buf[5]); putchar(buf[4]);
	putchar(0); putchar(0); putchar(buf[7]); putchar(buf[6]);
	buf[6]=buf[7]=255;
	for(i=768;i<ulen;i++) {
		buf[0]=buf[1]=udata[udata[i]*3+0];
		buf[2]=buf[3]=udata[udata[i]*3+1];
		buf[4]=buf[5]=udata[udata[i]*3+2];
		fwrite(buf,1,8,stdout);
	}
	return 0;
}
