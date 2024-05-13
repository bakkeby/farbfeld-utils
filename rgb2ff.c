#if 0
gcc -s -O2 -o ~/bin/rgb2ff -Wno-unused-result rgb2ff.c
exit
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned long long*mem;
static unsigned char buf[8];
static int width,height;
static int opt[128];

static void
usage(void)
{
	fprintf(stderr,
		"Usage: rgb2ff <width> <height> <?opt?> ...                                    \n"
		"                                                                              \n"
		"Converts headerless true colour formats to farbfeld. rgb2ff reads a headerless\n"
		"true colour format file from stdin, converts it to farbfeld and writes the    \n"
		"result to stdout.                                                             \n"
		"                                                                              \n"
		"This decoder takes two or more arguments. The first two are the width and     \n"
		"height, and the rest consist of settings to override the defaults, which      \n"
		"consist of a (case-sensitive) letter, followed by a number, with no space or  \n"
		"anything else in between.                                                     \n"
		"                                                                              \n"
		"Valid options include:                                                        \n"
		"  A -- Number of bits of alpha channel. (If zero (the default) then all pixels\n"
		"       are opaque.)                                                           \n"
		"  B -- Number of bits of blue channel                                         \n"
		"  G -- Number of bits of green channel                                        \n"
		"  R -- Number of bits of red channel                                          \n"
		"  S -- If nonzero, automatically sets the s setting based on alignment, so    \n"
		"       that a scanline is always aligned to this many bytes boundary          \n"
		"  a -- Bit position of alpha channel                                          \n"
		"  b -- Bit position of blue channel                                           \n"
		"  e -- Endianness; 1 for big-endian (default) or 0 for small-endian           \n"
		"  g -- Bit position of green channel                                          \n"
		"  i -- Interlace mode. Zero is default. If positive, it is the number of      \n"
		"       planes, and it has first the low plane for all pixels of the first row,\n"
		"       the second plane for all pixels of the first row, etc, and then next   \n"
		"       plane, and so on. If -1, has the first plane for all pixels, before the\n"
		"       next one. The low plane always comes first; endianness only controls   \n"
		"       order of bytes within each plane.                                      \n"
		"  k -- Number of padding bytes after each plane                               \n"
		"  m -- If set to 1, indicates that the input file uses premultiplied alpha.   \n"
		"       Zero is the default setting meaning direct data.                       \n"
		"  p -- Bytes per pixel (1 to 8; default 3). In interlace mode, number of bytes\n"
		"       per pixel in each plane.                                               \n"
		"  r -- Bit position of red channel                                            \n"
		"  s -- Number of bytes of padding between scanlines                           \n"
		"                                                                              \n"
		"If the file contains a header, use tail -c+N to skip the header. If the order \n"
		"of scanlines isn't in television order, use ff-turn to correct it after.      \n"
		"                                                                              \n"
		"Channels are allowed to overlap (and need not be contiguous); this can be used\n"
		"for grey scale. The output of any channel is automatically maximum if the     \n"
		"number of bits of that channel is set to zero. An example of how the bit      \n"
		"positions work (assuming R=G=B=8, r=16, g=8, b=0, big-endian, which are the   \n"
		"default settings):                                                            \n"
		"                                                                              \n"
		"   Bit positions                  8 7 6 5 4 3 2 1 0                           \n"
		"   | . . . : . . . | . . . : . . . | . . . : . . . |                          \n"
		"    [     RED     ] [    GREEN    ] [    BLUE     ]                           \n"
		"                                                                              \n"
		"Formats:                                                                      \n"
		"  - Falcon True Color: 384 240 p2 R5 G6 B5 r11 g5 b0                          \n"
		"                                                                              \n"
		"Example usage:                                                                \n"
		"   $ rgb2ff 320 240 i2 s2 < image.bin > image.ff                              \n"
		"   $ rgb2ff 320 240 S2 < image.bin > image.ff                                 \n"
		"\n"
	);
	exit(1);
}

static inline int getchannel(unsigned long long z,int b,int s) {
	unsigned long long m=((1ULL<<b)-1)<<s;
	unsigned short i=(z&m)>>s;
	switch(b) {
		case 0: return 0xFFFF;
		case 1: return i?0xFFFF:0;
		case 2: return 0x5555*i;
		case 3: return (0x9249*i)>>2;
		case 4: return 0x1111*i;
		case 5: return (0x8421*i)>>4;
		case 6: return (0x1041*i)>>2;
		case 7: return (0x4081*i)>>5;
		case 8: return i|(i<<8);
		case 9 ... 15: return (i>>(b-8))|(i<<(16-b));
		case 16: return i;
		default: return (z&m)>>(s+b-16);
	}
}

static inline void getpixel(void) {
	int i;
	unsigned long long z=0;
	fread(buf,1,opt['p'],stdin);
	if(opt['e']) {
		for(i=0;i<opt['p'];i++) z=(z<<8)|buf[i];
	} else {
		for(i=opt['p']-1;i>=0;i--) z=(z<<8)|buf[i];
	}
	i=getchannel(z,opt['R'],opt['r']); buf[0]=i>>8; buf[1]=i;
	i=getchannel(z,opt['G'],opt['g']); buf[2]=i>>8; buf[3]=i;
	i=getchannel(z,opt['B'],opt['b']); buf[4]=i>>8; buf[5]=i;
	i=getchannel(z,opt['A'],opt['a']); buf[6]=i>>8; buf[7]=i;
	if(opt['m'] && i) {
		z=(65535LL*((buf[0]<<8)|buf[1]))/i; z=z>65535?65535:z; buf[0]=z>>8; buf[1]=z;
		z=(65535LL*((buf[2]<<8)|buf[3]))/i; z=z>65535?65535:z; buf[2]=z>>8; buf[3]=z;
		z=(65535LL*((buf[4]<<8)|buf[5]))/i; z=z>65535?65535:z; buf[4]=z>>8; buf[5]=z;
	}
}

static void read_planes(int n,int m) {
	int p=0;
	int b,i,k,s;
	for(k=0;k<m;k++) {
		for(i=0;i<n;i++) {
			s=p+(opt['e']?(opt['p']-1)<<3:0);
			for(b=0;b<opt['p'];b++) {
				mem[i]&=~(255ULL<<s);
				mem[i]|=((unsigned long long)fgetc(stdin))<<s;
				if(opt['e']) s-=8; else s+=8;
			}
		}
		p+=opt['p']<<3;
		for(i=0;i<opt['k'];i++) fgetc(stdin);
		if(feof(stdin)) break;
	}
}

static void getpixel_plane(unsigned long long z) {
	int i;
	i=getchannel(z,opt['R'],opt['r']); buf[0]=i>>8; buf[1]=i;
	i=getchannel(z,opt['G'],opt['g']); buf[2]=i>>8; buf[3]=i;
	i=getchannel(z,opt['B'],opt['b']); buf[4]=i>>8; buf[5]=i;
	i=getchannel(z,opt['A'],opt['a']); buf[6]=i>>8; buf[7]=i;
	if(opt['m'] && i) {
		z=(65535LL*((buf[0]<<8)|buf[1]))/i; z=z>65535?65535:z; buf[0]=z>>8; buf[1]=z;
		z=(65535LL*((buf[2]<<8)|buf[3]))/i; z=z>65535?65535:z; buf[2]=z>>8; buf[3]=z;
		z=(65535LL*((buf[4]<<8)|buf[5]))/i; z=z>65535?65535:z; buf[4]=z>>8; buf[5]=z;
	}
}

int main(int argc,char**argv) {
	int i,j;
	if (argc < 3) {
		usage();
	}
	width=strtol(argv[1],0,0);
	height=strtol(argv[2],0,0);
	opt['e']=1; opt['p']=3; opt['s']=0; opt['S']=0;
	opt['r']=16; opt['g']=8; opt['b']=0; opt['a']=0;
	opt['R']=opt['G']=opt['B']=8; opt['A']=0;
	opt['m']=0; opt['i']=0; opt['k']=0;
	for(i=3;i<argc;i++) if(argv[i][0]) opt[argv[i][0]&127]=strtol(argv[i]+1,0,0);
	if(opt['p']<1 || opt['p']>8) {
		fprintf(stderr,"Invalid number of bytes per pixel\n");
		return 1;
	}
	fwrite("farbfeld",1,8,stdout);
	putchar(width>>24); putchar(width>>16); putchar(width>>8); putchar(width);
	putchar(height>>24); putchar(height>>16); putchar(height>>8); putchar(height);
	if(opt['S']) opt['s']=(opt['S']-(width*opt['p'])%opt['S'])%opt['S'];
	if(!opt['i']) {
		for(i=0;i<height;i++) {
			for(j=0;j<width;j++) {
				getpixel();
				fwrite(buf,1,8,stdout);
			}
			for(j=0;j<opt['s'];j++) fgetc(stdin);
		}
	} else if(opt['i']>0) {
		mem=calloc(sizeof(long long),width);
		if(!mem) {
			fprintf(stderr,"Allocation failed\n");
			return 1;
		}
		for(i=0;i<height;i++) {
			read_planes(width,opt['i']);
			for(j=0;j<opt['s'];j++) fgetc(stdin);
			for(j=0;j<width;j++) {
				getpixel_plane(mem[j]);
				fwrite(buf,1,8,stdout);
			}
		}
	} else if(opt['i']==-1) {
		mem=calloc(sizeof(long long),width*height);
		if(!mem) {
			fprintf(stderr,"Allocation failed\n");
			return 1;
		}
		read_planes(width*height,8);
		for(i=0;i<height;i++) {
			for(j=0;j<width;i++) {
				getpixel_plane(mem[j]);
				fwrite(buf,1,8,stdout);
			}
		}
	} else {
		fprintf(stderr,"Invalid interlace mode %d\n",opt['i']);
		return 1;
	}
	return 0;
}
