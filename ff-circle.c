#if 0
gcc -s -O2 -o ~/bin/ff-circle -Wno-unused-result ff-circle.c -lm
exit
#endif

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TAU (6.283185307179586476925286766559005768394338798750211641949889184615632812572417997256069650684234135964296173026564613294187689219101164463450718816256)
#define PI (0.5*TAU)

typedef struct {
	double start,end;
	unsigned char color[8];
} Slice;

static unsigned char buf[16];
static int width,height;
static char opt[128];
static Slice*slices;
static int nslice;
static int xorig,yorig;
static double radius,ellipse;

static void
usage(void)
{
	fprintf(stderr,
		"Usage: ff-circle <?option?> <X> <Y> <radius> <pos> <colour> <pos> ...         \n"
		"                                                                              \n"
		"This farbfeld filter draws a filled circle or pie slices onto a picture.      \n"
		"                                                                              \n"
		"The filter takes the following arguments:                                     \n"
		"   options -- affects the output in various ways                              \n"
		"   X origin -- starting X position (center of circle)                         \n"
		"   Y origin -- starting Y position (center of circle)                         \n"
		"   radius -- size of the circle (can incluce fractions)                       \n"
		"             the ellipse of the circle can be specified by adding a comma     \n"
		"             followed by the desired ellipse value (defaults to 1.0)          \n"
		"   alternating positions and colours                                          \n"
		"                                                                              \n"
		"The options start with a + followed by one or more of the these characters:   \n"
		"   v -- fill transparent parts of the picture with the pie colour             \n"
		"   t -- if the circle partially overlaps a transparent area then keep an      \n"
		"        outline of the non-transparent area being painted over                \n"
		"   a -- remove transparency in the circle area                                \n"
		"   n -- pie sections are each gradient from dark to given colour              \n"
		"   r -- gradient black radiating from the center of the circle                \n"
		"                                                                              \n"
		"The alternating positions and colours start with a position, and then a       \n"
		"colour, and then a position, and then a colour, and then a position, and so   \n"
		"on; it must start and end with a position, and there must be at least one     \n"
		"colour. A position is a floating number in range 0 to 1, where 0 is the top   \n"
		"position and increasing numbers are going counterclockwise around the circle, \n"
		"to make the size of the pie slice.                                            \n"
		"                                                                              \n"
		"The colours can have the following patterns:                                  \n"
		"   rrggbb            three pairs of 8-bit hex values                          \n"
		"   rrggbbaa          four pairs of 8-bit hex values                           \n"
		"   rrrrggggbbbb      three pairs of 16-bit hex values                         \n"
		"   rrrrggggbbbbaaaa  four pairs of 16-bit hex values                          \n"
		"                                                                              \n"
		"Example usage:                                                                \n"
		"   $ ff-circle 50 100 50 0 FF6060 1.0 < image.ff > image-circle.ff            \n"
		"   $ ff-circle 150 100 30 0 FF6060 0.45 60FF60 0.75 6060FF 1 < i.ff > o.ff    \n"
		"   $ ff-circle +v 50 50 30 0 FF6060 0.45 60FF60 1 < image.ff > image-circle.ff\n"
		"   $ ff-circle +tn 50 50 30 0 FF6060 1 < image.ff > image-circle.ff           \n"
		"   $ ff-circle +a 50 50 30 0 FF6060 1 < image.ff > image-circle.ff            \n"
		"\n"
	);
	exit(1);
}

static void get_color(const char*txt,unsigned char*color) {
	int i=strlen(txt);
	if(i==6) {
		sscanf(txt,"%2hhX%2hhX%2hhX",color+0,color+2,color+4);
		color[1]=color[0];
		color[3]=color[2];
		color[5]=color[4];
		color[7]=color[6]=255;
	} else if(i==8) {
		sscanf(txt,"%2hhX%2hhX%2hhX%2hhX",color+0,color+2,color+4,color+6);
		color[1]=color[0];
		color[3]=color[2];
		color[5]=color[4];
		color[7]=color[6];
	} else if(i==12) {
		sscanf(txt,"%2hhX%2hhX%2hhX%2hhX%2hhX%2hhX",color+0,color+1,color+2,color+3,color+4,color+5);
		color[7]=color[6]=255;
	} else if(i==16) {
		sscanf(txt,"%2hhX%2hhX%2hhX%2hhX%2hhX%2hhX%2hhX%2hhX",color+0,color+1,color+2,color+3,color+4,color+5,color+6,color+7);
	} else {
		fprintf(stderr,"Invalid color format\n");
		exit(1);
	}
}

static void process(void) {
	int i,x,y;
	double r,a;
	for(y=0;y<height;y++) for(x=0;x<width;x++) {
		fread(buf,1,8,stdin);
		r=hypot(x-xorig,(y-yorig)*ellipse);
		if(opt['v']) r*=((buf[6]<<8)|buf[7])/65535.0;
		if(r<=radius) {
			a=atan2(x-xorig,y-yorig)+PI;
			if(opt['t']) a=fmod(a+((buf[6]<<8)|buf[7])*TAU/65535.0,TAU);
			for(i=0;i<nslice;i++) {
				if(a>=slices[i].start && a<=slices[i].end) {
					if(opt['a']) memcpy(buf+6,slices[i].color+6,2);
					else memcpy(buf,slices[i].color,opt['c']?6:8);
					if(opt['n']) {
						i=65535.0*(a-slices[i].start)/(slices[i].end-slices[i].start); i=i>65535?65535:i<0?0:i;
						buf[6]=i>>8; buf[7]=i;
					} else if(opt['r']) {
						i=r*65535.0/radius; i=i>65535?65535:i<0?0:i;
						buf[6]=i>>8; buf[7]=i;
					}
					break;
				}
			}
		}
		fwrite(buf,1,8,stdout);
	}
}

static void get_slices(int argc,char**argv) {
	int i;
	nslice=argc>>1;
	slices=malloc(nslice*sizeof(Slice));
	if(!slices) {
		fprintf(stderr,"Allocation failed\n");
		exit(1);
	}
	for(i=0;i<nslice;i++) {
		slices[i].start=TAU*strtod(argv[i+i],0);
		get_color(argv[i+i+1],slices[i].color);
		slices[i].end=TAU*strtod(argv[i+i+2],0);
	}
}

int main(int argc,char**argv) {
	char*s;
	if (argc>1 && (!strcmp(argv[1],"-h") || !strcmp(argv[1],"--help"))) {
		usage();
	}
	if(argc>2 && argv[1][0]=='+') {
		int i;
		for(i=0;argv[1][i];i++) opt[argv[1][i]&127]=1;
		argc--;
		argv++;
	}
	if(argc<7) {
		fprintf(stderr,"Too few arguments\n");
		return 1;
	}
	if(!(argc&1)) {
		fprintf(stderr,"Incorrect number of arguments\n");
		return 1;
	}
	xorig=strtol(argv[1],0,0);
	yorig=strtol(argv[2],0,0);
	radius=strtod(argv[3],&s);
	if(*s==',') ellipse=strtod(s+1,0); else ellipse=1.0;
	get_slices(argc-4,argv+4);
	fread(buf,1,16,stdin);
	fwrite(buf,1,16,stdout);
	width=(buf[8]<<24)|(buf[9]<<16)|(buf[10]<<8)|buf[11];
	height=(buf[12]<<24)|(buf[13]<<16)|(buf[14]<<8)|buf[15];
	process();
	return 0;
}
