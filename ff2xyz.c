#if 0
gcc -s -O2 -o ~/bin/ff2xyz -Wno-unused-result ff2xyz.c lodepng.c
exit
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lodepng.h"

typedef struct {
	unsigned char rgb[3];
	unsigned char i;
	int n;
} Palette;

typedef struct {
	int a;
	int b;
} BytUse;

static unsigned char buf[16];
static LodePNGCompressSettings st;
static Palette pal[256];
static BytUse bytuse[256];
static int npal,npixels,fixpal;
static unsigned char*pic;
static unsigned char*cdata;
static size_t clen;

static void
usage(void)
{
	fprintf(stderr,
		"Usage: ff2xyz                                                                 \n"
		"                                                                              \n"
		"Converts farbfeld to .XYZ picture files to use with RPG Maker and EasyRPG.    \n"
		"ff2xyz reads farbfeld from stdin, converts it to .XYZ and and writes the      \n"
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
		"Refer to the man page for command line options.                               \n"
		"                                                                              \n"
		"Example usage:                                                                \n"
		"   $ xyz2ff < image.xyz > image.ff                                            \n"
		"\n"
	);
	exit(1);
}

static int bytuse_compar(const void*a,const void*b) {
	const BytUse*x=a;
	const BytUse*y=b;
	return y->a-x->a;
}

static int pal_compar(const void*a,const void*b) {
	const Palette*x=a;
	const Palette*y=b;
	return y->n-x->n;
}

int main(int argc,char**argv) {
	int i,e;
	lodepng_compress_settings_init(&st);
	for(e=1;e<argc;e++) {
		if (!strcmp(argv[e],"-h") || !strcmp(argv[e],"--help")) {
			usage();
		}
		switch(argv[e][0]) {
			case '+':
				e++;
				goto palet;
			case 'b':
				st.btype=argv[e][1]-'0';
				break;
			case 'e':
				st.blocksize=strtol(argv[e]+1,(char**)&buf,0);
				if(*buf==',') st.maxchainlength=strtol(buf+1,(char**)&buf,0);
				if(*buf==',') st.maxlazymatch=strtol(buf+1,(char**)&buf,0);
				if(*buf==',') st.usezeros=strtol(buf+1,(char**)&buf,0);
				if(*buf==',') st.too_far=strtol(buf+1,0,0);
				break;
			case 'f':
				fixpal=argv[e][1]-'0';
				break;
			case 'l':
				st.lazymatching=argv[e][1]-'0';
				break;
			case 'm':
				st.minmatch=strtol(argv[e]+1,0,0);
				break;
			case 'n':
				st.nicematch=strtol(argv[e]+1,0,0);
				break;
			case 'u':
				st.use_lz77=argv[e][1]-'0';
				break;
			case 'w':
				st.windowsize=strtol(argv[e]+1,0,0);
				break;
			default:
				if (argv[e][0]) {
					fprintf(stderr,"Unknown option: %c\n",argv[e][0]);
					return 1;
				}
		}
	}
	palet:
	if(e<argc) {
		for(;e<argc;e++) {
			if(npal==256) break;
			sscanf(argv[e],"%2hhX%2hhX%2hhX",pal[npal].rgb+0,pal[npal].rgb+1,pal[npal].rgb+2);
			npal++;
		}
		fixpal=npal=256;
	}
	fread(buf,1,16,stdin);
	if(memcmp("farbfeld",buf,8)) {
		fprintf(stderr,"Not farbfeld\n");
		return 1;
	}
	if(buf[8] || buf[9] || buf[12] || buf[13]) {
		fprintf(stderr,"Picture is too big\n");
		return 1;
	}
	npixels=((buf[10]<<8)|buf[11])*((buf[14]<<8)|buf[15]);
	pic=malloc(npixels+768);
	if(!pic) {
		fprintf(stderr,"Allocation failed\n");
		return 1;
	}
	for(e=0;e<npixels;e++) {
		fread(buf,1,8,stdin);
		for(i=0;i<npal;i++) {
			if(pal[i].rgb[0]==buf[0] && pal[i].rgb[1]==buf[2] && pal[i].rgb[2]==buf[4]) goto found;
		}
		if(npal==256) {
			fprintf(stderr,fixpal?"Color not in palette\n":"Too many colors\n");
			return 1;
		}
		i=npal++;
		pal[i].rgb[0]=buf[0];
		pal[i].rgb[1]=buf[2];
		pal[i].rgb[2]=buf[4];
		found: ++pal[pic[e+768]=i].n;
	}
	for(i=0;i<256;i++) bytuse[i].b=pal[i].i=i;
	if(!fixpal) {
		for(i=0;i<npal;i++) {
			++bytuse[pal[i].rgb[0]].a;
			++bytuse[pal[i].rgb[1]].a;
			++bytuse[pal[i].rgb[2]].a;
		}
		qsort(bytuse,256,sizeof(BytUse),bytuse_compar);
		for(i=npal;i<256;i++) pal[i].rgb[0]=pal[i].rgb[1]=pal[i].rgb[2]=bytuse->b;
		qsort(pal,256,sizeof(Palette),pal_compar);
		for(i=0;i<256;i++) bytuse[pal[i].i].a=i;
		for(i=0;i<256;i++) pal[i].i=bytuse[i].b;
		for(i=0;i<npixels;i++) pic[i+768]=pal[bytuse[pic[i+768]].a].i;
	}
	for(i=0;i<256;i++) memcpy(pic+pal[i].i*3,pal[i].rgb,3);
	if(e=lodepng_zlib_compress(&cdata,&clen,pic,npixels+768,&st)) {
		fprintf(stderr,"%s\n",lodepng_error_text(e));
		return 1;
	}
	memcpy(buf,"XYZ1",4);
	buf[4]=buf[11];
	buf[5]=buf[10];
	buf[6]=buf[15];
	buf[7]=buf[14];
	fwrite(buf,1,8,stdout);
	fwrite(cdata,1,clen,stdout);
	return 0;
}
