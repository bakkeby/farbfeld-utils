#if 0
gcc -s -O2 -o ~/bin/ff2xzip -Wno-unused-result ff2xzip.c
exit
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define fatal(...) do{ fprintf(stderr,__VA_ARGS__); fputc('\n',stderr); exit(1); }while(0)

typedef struct {
	FILE*fp;
	int w,h;
	char n[14];
} Picture;

static unsigned char buf[16];
static int gcount,goff,gsize,gflags,npic;
static Picture*pic;

static void
usage(void)
{
	fprintf(stderr,
		"Usage: ff2xzip <flag> <offset> <?options...?>                                 \n"
		"                                                                              \n"
		"Converts farbfeld to XZIP picture library format. ff2xzip reads farbfeld from \n"
		"stdin, converts that to Z-machine XZIP picture library file format and writes \n"
		"the result to stdout.                                                         \n"
		"                                                                              \n"
		"The format was documented together with XZIP, but was not given a particular  \n"
		"name. For the purpose of this documentation we refer to it as the XZIP        \n"
		"picture library format. It is not the same format as the more common \"YZIP   \n"
		"picture library\" format, of which there are several variants.                \n"
		"                                                                              \n"
		"ff2xzip takes three or more arguments.                                        \n"
		"                                                                              \n"
		"The first two arguments are the flag and the offset. The flag can be 1 if the \n"
		"first picture is a splash screen, or 0 otherwise. The offset is the number of \n"
		"the first picture in the file.                                                \n"
		"                                                                              \n"
		"After the first two arguments, there is either one more argument (for a       \n"
		"character set) or an even number of arguments.                                \n"
		"                                                                              \n"
		"In the case of a character set, the one argument is the character height, and \n"
		"the picture comes from stdin, which is expected to be a vertical strip.       \n"
		"                                                                              \n"
		"In the other case, the arguments form pairs of a DOS filename and the input   \n"
		"picture (stdin isn't used in this mode). The DOS filename is limited to 12    \n"
		"characters; if you put more than 12 characters then it is truncated, and if   \n"
		"you put less than 12 characters then it is padded with spaces. Instead of the \n"
		"input picture you can use an empty string to indicate a picture which isn't   \n"
		"present.                                                                      \n"
		"                                                                              \n"
		"In either case, transparent pixels are considered background and opaque       \n"
		"pictures are considered foreground (use ff-colorkey or ff-swizzle first to    \n"
		"convert if needed).                                                           \n"
		"                                                                              \n"
		"Example usage:                                                                \n"
		"   $ ff2xzip 0 2 14 < vertical_strip.ff > image.xzip                          \n"
		"\n"
	);
	exit(1);
}

static inline void put16(int x) {
	putchar(x>>8); putchar(x);
}

static void do_picture(const Picture*p) {
	int x,y,z;
	for(y=0;y<p->h;y++) for(x=z=0;x<p->w;x++) {
		fread(buf,1,8,p->fp);
		z=(z<<1)|(buf[6]&128?1:0);
		if(x==p->w-1 || (x&15)==15) put16(z),z=0;
	}
}

int main(int argc,char**argv) {
	int i;
	if(argc<3 || (argc>4 && !(argc&1))) usage();
	gflags=strtol(argv[1],0,0);
	goff=strtol(argv[2],0,0);
	if(argc==4) {
		pic=malloc(sizeof(Picture));
		if(!pic) fatal("Allocation failed");
		pic->fp=stdin;
		npic=1;
		gsize=strtol(argv[3],0,0);
		if(gsize<1 || gsize>255) fatal("Invalid character height");
		fread(buf,1,16,stdin);
		if(memcmp(buf,"farbfeld",8)) fatal("Not farbfeld");
		if(buf[8] || buf[9] || buf[10] || buf[12] || buf[13]) fatal("Picture too big");
		pic->w=buf[11];
		pic->h=(buf[12]<<8)|buf[13];
		if(!pic->w) fatal("Input width is zero");
		if(pic->h%gsize) fatal("Height of input is not divisible by %d",gsize);
		gcount=pic->h/gsize;
		if(gcount<1 || gcount>65535) fatal("Number of characters is not valid");
		gsize|=buf[11]<<8;
	} else {
		gsize=0;
		npic=gcount=(argc-2)>>1;
		pic=malloc(npic*sizeof(Picture));
		if(npic && !pic) fatal("Allocation failed");
		for(i=0;i<npic;i++) {
			snprintf(pic[i].n,13,"%s            ",argv[i+i+3]);
			if(argv[i+i+4][0]) {
				pic[i].fp=fopen(argv[i+i+4],"r");
				if(!pic[i].fp) {
					perror(argv[i+i+4]);
					fatal("Cannot input picture %d",i+goff);
				}
				fread(buf,1,16,pic[i].fp);
				if(memcmp(buf,"farbfeld",8)) fatal("Not farbfeld");
				if(buf[8] || buf[9] || buf[12] || buf[13]) fatal("Picture %d is too big",i+goff);
				pic[i].w=(buf[10]<<8)|buf[11];
				pic[i].h=(buf[14]<<8)|buf[15];
			} else {
				pic[i].fp=0;
				pic[i].w=pic[i].h=0;
			}
		}
	}
	put16(gcount); put16(goff); put16(gsize); put16(gflags);
	put16(0); put16(0); put16(0); put16(0);
	put16(0); put16(0); put16(0); put16(0);
	put16(0); put16(0); put16(0); put16(0);
	if(!gsize) {
		for(i=0;i<npic;i++) put16(pic[i].w),put16(pic[i].h);
		for(i=0;i<npic;i++) fwrite(pic[i].n,1,12,stdout);
	}
	for(i=0;i<npic;i++) do_picture(pic+i);
	return 0;
}
