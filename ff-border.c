#if 0
gcc -s -O2 -o ~/bin/ff-border -Wno-unused-result ff-border.c
exit
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
	unsigned short r;
	unsigned short g;
	unsigned short b;
	unsigned short a;
} Color;

static unsigned char*picture;
static unsigned char buf[16];
static unsigned int width,height;
static int top,bottom,left,right;
static Color bcolor;
static int btype,bparam;

static void
usage(void)
{
	fprintf(stderr,
		"Usage: ff-border <border style> <size> <?size?> <?size?> <?size?>             \n"
		"                                                                              \n"
		"This farbfeld filter adds an outer border to the picture.                     \n"
		"                                                                              \n"
		"The filter takes up to five arguments.                                        \n"
		"                                                                              \n"
		"The first argument is the border style and the rest are the border size.      \n"
		"                                                                              \n"
		"The border style can be one of three kinds:                                   \n"
		"   - a colour (to make a solid colour border)                                 \n"
		"   - a single letter (see list below)                                         \n"
		"   - a single letter followed by an equal sign and a number (applicable if an \n"
		"     option in the list below takes any parameters)                           \n"
		"                                                                              \n"
		"The colour can be defined using any of the following patterns:                \n"
		"   hh                single pair of 8-bit hex values; the same value is used  \n"
		"                     for all channels and alpha is fully opaque               \n"
		"   hhaa              two pairs of 8-bit hex values; the first is used for all \n"
		"                     channels and the second value is used for the alpha      \n"
		"   rrggbb            three pairs of 8-bit hex values, alpha is fully opaque   \n"
		"   rrggbbaa          four pairs of 8-bit hex values                           \n"
		"   rrrrggggbbbb      three pairs of 16-bit hex values, alpha is fully opaque  \n"
		"   rrrrggggbbbbaaaa  four pairs of 16-bit hex values                          \n"
		"                                                                              \n"
		"The single letter options are:                                                \n"
		"   e -- edge; just copy the edge colour                                       \n"
		"   o -- offset; takes an extra parameter being the offset into the picture    \n"
		"        raster data, and then the border just copies the pixels disregarding  \n"
		"        the width and using the specified offset to start with                \n"
		"   t -- tiled; repeat the picture from the other side to be wrapped around    \n"
		"   x -- transparent (same as \"00000000\")                                    \n"
		"                                                                              \n"
		"The size can be given as one, two, three, or four numbers:                    \n"
		"   1 -- top=bottom=left=right                                                 \n"
		"   2 -- top=bottom, left=right                                                \n"
		"   3 -- top, left=right, bottom                                               \n"
		"   4 -- top, right, bottom, left                                              \n"
		"                                                                              \n"
		"Example usage:                                                                \n"
		"   $ ff-border FF9090 12 < image.ff > image-border.ff                         \n"
		"   $ ff-border e 10 < image.ff > image-border.ff                              \n"
		"   $ ff-border o=10 5 8 < image.ff > image-border.ff                          \n"
		"   $ ff-border t 50 < image.ff > image-border.ff                              \n"
		"   $ ff-border x 10 10 10 10 < image.ff > image-border.ff                     \n"
		"\n"
	);
	exit(1);
}

static Color parse_color(const char*s) {
	Color c;
	switch(strlen(s)) {
		case 2:
			sscanf(s,"%2hx",&c.r);
			c.r*=0x101;
			c.g=c.b=c.r;
			c.a=0xFFFF;
			break;
		case 4:
			sscanf(s,"%4hx",&c.r);
			c.g=c.b=c.r;
			c.a=0xFFFF;
			break;
		case 6:
			sscanf(s,"%2hx%2hx%2hx",&c.r,&c.g,&c.b);
			c.r*=0x101;
			c.g*=0x101;
			c.b*=0x101;
			c.a=0xFFFF;
			break;
		case 8:
			sscanf(s,"%2hx%2hx%2hx%2hx",&c.r,&c.g,&c.b,&c.a);
			c.r*=0x101;
			c.g*=0x101;
			c.b*=0x101;
			c.a*=0x101;
			break;
		case 12:
			sscanf(s,"%4hx%4hx%4hx",&c.r,&c.g,&c.b);
			c.a=0xFFFF;
			break;
		case 16:
			sscanf(s,"%4hx%4hx%4hx%4hx",&c.r,&c.g,&c.b,&c.a);
			break;
	}
	return c;
}

static void process_tb(int which) {
	int i,j,x,y;
	switch(btype) {
		case 'e': // Edge
			i=which?(height-1)*width<<3:0;
			j=which?bottom:top;
			while(j--) {
				x=left; while(x--) fwrite(picture+i,1,8,stdout);
				fwrite(picture+i,8,width,stdout);
				x=right; while(x--) fwrite(picture+i+(width-1)*8,1,8,stdout);
			}
			break;
		case 'o': // Offset
			i=(width*height+bparam)%(width*height);
			j=(width+left+right)*(which?bottom:top);
			while(j>width*height-i) fwrite(picture+i*8,8,width*height-i,stdout),j-=width*height-i,i=0;
			fwrite(picture+i*8,8,j,stdout);
			bparam+=width*height-i-j;
			break;
		case 't': // Tiled
			y=which?0:-top; while(y<0) y+=height;
			i=which?bottom:top;
			while(i--) {
				j=width+left+right;
				x=-left; while(x<0) x+=width;
				while(j--) fwrite(picture+((width*y+x)<<3),1,8,stdout),x=(x+1)%width;
				y=(y+1)%height;
			}
			break;
		case 'x': // Transparent (or solid colour)
			i=(width+left+right)*(which?bottom:top);
			while(i--) fwrite(buf,1,8,stdout);
			break;
		default:
			fprintf(stderr,"Invalid border mode: %c\n",btype);
			exit(1);
	}
}

static void process_lr(int which,int pos) {
	int i,j,x,y;
	switch(btype) {
		case 'e':
			i=((pos+which)*width-which)<<3;
			j=which?right:left;
			while(j--) fwrite(picture+i,1,8,stdout);
			break;
		case 'o': // Offset
			i=(width*height+bparam)%(width*height);
			j=which?right:left;
			while(j>width*height-i) fwrite(picture+i*8,8,width*height-i,stdout),j-=width*height*i,i=0;
			fwrite(picture+i*8,8,j,stdout);
			bparam=(bparam+width*height-i-j)%(width*height);
			break;
		case 't':
			y=pos%height;
			x=which?0:-left; while(x<0) x+=width;
			i=which?right:left;
			while(i--) fwrite(picture+((width*y+x)<<3),1,8,stdout),x=(x+1)%width;
			break;
		case 'x':
			i=which?right:left;
			while(i--) fwrite(buf,1,8,stdout);
			break;
#if __GNUC__ > 4
		default:
			__builtin_unreachable();
#endif
	}
}

int main(int argc,char**argv) {
	int i;

	if (argc<2 || !strcmp(argv[1],"-h") || !strcmp(argv[1],"--help")) {
		usage();
	}
	if (argc<3 || argc>6) {
		fprintf(stderr,"Too %s arguments\n",argc<3?"few":"many");
		return 1;
	}
	i=strlen(argv[1]);
	if(i==1) {
		btype=argv[1][0];
	} else if(i && argv[1][1]=='=') {
		btype=argv[1][0];
		bparam=strtol(argv[1]+2,0,0);
	} else if(i==2 || i==4 || i==6 || i==8 || i==12 || i==16) {
		btype='x';
		bcolor=parse_color(argv[1]);
	} else {
		fprintf(stderr,"Unrecognized border mode\n");
		return 1;
	}
	top=bottom=left=right=strtol(argv[2],0,0);
	if(argc>3) left=right=strtol(argv[3],0,0);
	if(argc>4) bottom=strtol(argv[4],0,0);
	if(argc>5) left=strtol(argv[5],0,0);
	if(top<0 || bottom<0 || left<0 || right<0) {
		fprintf(stderr,"Improper border size\n");
		return 1;
	}
	fread(buf,1,16,stdin);
	width=(buf[8]<<24)|(buf[9]<<16)|(buf[10]<<8)|buf[11];
	height=(buf[12]<<24)|(buf[13]<<16)|(buf[14]<<8)|buf[15];
	picture=calloc(width,height<<3);
	if(!picture) {
		fprintf(stderr,"Allocation failed\n");
		return 1;
	}
	fread(picture,8,width*height,stdin);
	buf[8]=(width+left+right)>>24;
	buf[9]=(width+left+right)>>16;
	buf[10]=(width+left+right)>>8;
	buf[11]=(width+left+right);
	buf[12]=(height+top+bottom)>>24;
	buf[13]=(height+top+bottom)>>16;
	buf[14]=(height+top+bottom)>>8;
	buf[15]=(height+top+bottom);
	fwrite(buf,1,16,stdout);
	if(btype=='x') {
		buf[0]=bcolor.r>>8;
		buf[1]=bcolor.r;
		buf[2]=bcolor.g>>8;
		buf[3]=bcolor.g;
		buf[4]=bcolor.b>>8;
		buf[5]=bcolor.b;
		buf[6]=bcolor.a>>8;
		buf[7]=bcolor.a;
	}
	process_tb(0);
	for(i=0;i<height;i++) {
		process_lr(0,i);
		fwrite(picture+width*8*i,8,width,stdout);
		process_lr(1,i);
	}
	process_tb(1);
	return 0;
}

