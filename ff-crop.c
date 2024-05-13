#if 0
gcc -s -O2 -o ~/bin/ff-crop -Wno-unused-result ff-crop.c
exit
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned char buf[16];
static int width,height,xoffs,yoffs,xsize,ysize,active;
static unsigned char*row;
static unsigned char*actrow;

static void
usage(void)
{
	fprintf(stderr,
		"Usage: ff-crop <x-offset> <y-offset> <width> <height> <?border colour?>       \n"
		"                                                                              \n"
		"This farbfeld filter is used to crop a picture.                               \n"
		"                                                                              \n"
		"ff-crop can also be used to increase the canvas of an image and/or to add a   \n"
		"border around an image.                                                       \n"
		"                                                                              \n"
		"The filter takes four or five arguments.                                      \n"
		"                                                                              \n"
		"The first two are the X and Y offset of the top-left corner in the original   \n"
		"picture to crop to, and the next two are the width and height of the result.  \n"
		"It is OK for the X and Y offsets to be negative, and it is also OK to take a  \n"
		"piece larger than the original picture. The fifth argument is optional and is \n"
		"the border colour to use for parts that are included in the new picture that  \n"
		"are out of the range of the old picture.                                      \n"
		"                                                                              \n"
		"The border colour can be defined through one of the following patterns:       \n"
		"   rrggbb            three pairs of 8-bit hex values                          \n"
		"   rrggbbaa          four pairs of 8-bit hex values                           \n"
		"   rrrrggggbbbb      three pairs of 16-bit hex values                         \n"
		"   rrrrggggbbbbaaaa  four pairs of 16-bit hex values                          \n"
		"                                                                              \n"
		"If the border colour is not specified then it will default to be fully        \n"
		"transparent.                                                                  \n"
		"                                                                              \n"
		"Example usage:                                                                \n"
		"   $ ff-crop 0 0 640 480 < image.ff > image-crop.ff                           \n"
		"   $ ff-crop 150 112 300 300 < image.ff > image-crop.ff                       \n"
		"   $ ff-crop 0 0 100 100 < image.ff | ff-crop -5 -5 110 110 > crop_border.ff  \n"
		"\n"
	);
	exit(1);
}

static void parse_color(unsigned char*out,const char*in) {
	switch(strlen(in)) {
		case 6:
			sscanf(in,"%02hhX%02hhX%02hhX",out+0,out+2,out+4);
			out[1]=out[0];
			out[3]=out[2];
			out[5]=out[4];
			out[6]=out[7]=255;
			break;
		case 8:
			sscanf(in,"%02hhX%02hhX%02hhX%02hhX",out+0,out+2,out+4,out+6);
			out[1]=out[0];
			out[3]=out[2];
			out[5]=out[4];
			out[7]=out[6];
			break;
		case 12:
			sscanf(in,"%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX",out+0,out+1,out+2,out+3,out+4,out+5);
			out[6]=out[7]=255;
			break;
		case 16:
			sscanf(in,"%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX",out+0,out+1,out+2,out+3,out+4,out+5,out+6,out+7);
			break;
		default:
			fprintf(stderr,"Invalid color format (%d)\n",(int)strlen(in));
			exit(1);
	}
}

int main(int argc,char**argv) {
	int i;
	if(argc<5 || !strcmp(argv[1],"-h") || !strcmp(argv[1],"--help")) {
		usage();
	}
	xoffs=strtol(argv[1],0,0);
	yoffs=strtol(argv[2],0,0);
	xsize=strtol(argv[3],0,0);
	ysize=strtol(argv[4],0,0);
	fread(buf,1,16,stdin);
	width=(buf[8]<<24)|(buf[9]<<16)|(buf[10]<<8)|buf[11];
	height=(buf[12]<<24)|(buf[13]<<16)|(buf[14]<<8)|buf[15];
	row=calloc(8,width);
	if(!row) {
		fprintf(stderr,"Out of memory\n");
		return 1;
	}
	buf[8]=xsize>>24;
	buf[9]=xsize>>16;
	buf[10]=xsize>>8;
	buf[11]=xsize;
	buf[12]=ysize>>24;
	buf[13]=ysize>>16;
	buf[14]=ysize>>8;
	buf[15]=ysize;
	fwrite(buf,1,16,stdout);
	if(argc>5 && argv[5][0]) parse_color(buf,argv[5]); else memset(buf,0,8);
	while(yoffs<0) {
		yoffs++;
		i=xsize;
		while(i--) fwrite(buf,1,8,stdout);
	}
	while(yoffs-->0) {
		fread(row,8,width,stdin);
		height--;
	}
	active=xsize;
	if(xoffs<0) active+=xoffs;
	if(active>width-xoffs) active=width-xoffs;
	if(active>width) active=width;
	actrow=row+(xoffs<0?0:xoffs*8);
	while(height-->0 && ysize-->0) {
		fread(row,8,width,stdin);
		if(xoffs<0) for(i=xoffs;i;i++) fwrite(buf,1,8,stdout);
		if(xoffs<width) fwrite(actrow,8,active,stdout);
		for(i=active-(xoffs<0?xoffs:0);i<xsize;i++) fwrite(buf,1,8,stdout);
	}
	while(ysize-->0) {
		i=xsize;
		while(i--) fwrite(buf,1,8,stdout);
	}
	return 0;
}
