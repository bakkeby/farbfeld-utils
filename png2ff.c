#if 0
gcc -s -O2 -o ~/bin/png2ff png2ff.c lodepng.c
exit
#endif
/*
	Convert PNG to Farbfeld
	(public domain)
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lodepng.h"

static void
usage(void)
{
	fprintf(stderr,
		"Usage: png2ff                                                                 \n"
		"                                                                              \n"
		"Convert PNG to farbfeld. png2ff reads a PNG image from stdin, converts it to  \n"
		"farbfeld and writes the result to stdout.                                     \n"
		"                                                                              \n"
		"This can decode iPhone format PNG files in addition to proper PNG files. It   \n"
		"can load animations, but only the first frame will be used.                   \n"
		"                                                                              \n"
		"This decoder differs from the program with the same name in the suckless base \n"
		"farbfeld library in that it uses the standalone lodepng library for decoding  \n"
		"PNG files, whereas the base library version has a dependency on libpng.       \n"
		"                                                                              \n"
		"This decoder takes no command-line arguments.                                 \n"
		"                                                                              \n"
		"Example usage:                                                                \n"
		"   $ png2ff < image.png > image.ff                                            \n"
		"   $ png2ff < image.png | bzip2 > image.ff.bz2                                \n"
		"\n"
	);
	exit(1);
}

static void convert_iphone(unsigned char*p,size_t n) {
	int i[4];
	while(n--) {
		if(i[3]=(p[6]<<8)|p[7]) {
			i[0]=(65535LL*((p[4]<<8)|p[5]))/i[3];
			i[1]=(65535LL*((p[2]<<8)|p[3]))/i[3];
			i[2]=(65535LL*((p[0]<<8)|p[1]))/i[3];
		} else {
			i[0]=i[1]=i[2]=0;
		}
		p[0]=i[0]>>8; p[1]=i[0];
		p[2]=i[1]>>8; p[3]=i[1];
		p[4]=i[2]>>8; p[5]=i[2];
		p+=8;
	}
}

int main(int argc,char**argv) {
	unsigned char*out;
	unsigned char*in=0;
	size_t insize=0;
	size_t rsize;
	unsigned w,h,e;
	if (argc > 1) {
		usage();
	}

	do {
		in=realloc(in,insize+65536);
		if(!in) {
			fprintf(stderr,"Out of memory\n");
			return 1;
		}
		insize+=rsize=fread(in+insize,1,65536,stdin);
	} while(rsize);
	if(insize>16 && !memcmp(in+12,"CgBI",4)) {
		// It is a iPhone format PNG
		LodePNGState st;
		w=(in[8]<<24)|(in[9]<<16)|(in[10]<<8)|in[11];
		memmove(in+8,in+w+20,insize-w-20);
		insize-=w+12;
		lodepng_state_init(&st);
		st.info_raw.colortype=LCT_RGBA;
		st.info_raw.bitdepth=16;
		st.decoder.zlibsettings.custom_zlib=lodepng_inflate;
		if(e=lodepng_decode(&out,&w,&h,&st,in,insize)) {
			fprintf(stderr,"This seems to be a iPhone format PNG\n%s\n",lodepng_error_text(e));
			return 1;
		}
		convert_iphone(out,w*h);
	} else if(e=lodepng_decode_memory(&out,&w,&h,in,insize,LCT_RGBA,16)) {
		fprintf(stderr,"%s\n",lodepng_error_text(e));
		return 1;
	}
	fwrite("farbfeld",1,8,stdout);
	putchar(w>>24);
	putchar(w>>16);
	putchar(w>>8);
	putchar(w>>0);
	putchar(h>>24);
	putchar(h>>16);
	putchar(h>>8);
	putchar(h>>0);
	fwrite(out,w*h,8,stdout);
	return 0;
}
