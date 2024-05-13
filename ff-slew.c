#if 0
gcc -s -O2 -o ~/bin/ff-slew -Wno-unused-result ff-slew.c
exit 0
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned char buf[16];
static unsigned char*row;
static int width,height;
static int hu,hd,vu,vd,x,y;

static void
usage(void)
{
	fprintf(stderr,
		"Usage: ff-slew <slew rate>                                                    \n"
		"                                                                              \n"
		"This farbfeld filter applies two dimensional slew rate limiting on a picture. \n"
		"                                                                              \n"
		"The filter takes one, two or four arguments.                                  \n"
		"                                                                              \n"
		"With one argument:                                                            \n"
		"   - slew rate                                                                \n"
		"                                                                              \n"
		"With two arguments:                                                           \n"
		"   - horizontal slew rate                                                     \n"
		"   - vertical slew rate                                                       \n"
		"                                                                              \n"
		"With four arguments:                                                          \n"
		"   - positive horizontal slew rate                                            \n"
		"   - negative horizontal slew rate                                            \n"
		"   - positive vertical slew rate                                              \n"
		"   - negative vertical slew rate                                              \n"
		"                                                                              \n"
		"The maximum slew rate is 65535 which is normal (no change), while lower       \n"
		"numbers result in pixels with less changing.                                  \n"
		"                                                                              \n"
		"The pixels on the top and left edges have nothing to compare to so are assumed\n"
		"to start with those values. The ones on the left edge can still be limited    \n"
		"relative to the pixels above, and the ones on the top edge can still be       \n"
		"limited relative to the pixels to left. Use ff-border to add a border with    \n"
		"different initial values if so desired.                                       \n"
		"                                                                              \n"
		"Note that even though it says \"positive\" and \"negative\" above, all of the \n"
		"numbers should nevertheless be positive, in the range from 0 to 65535.        \n"
		"                                                                              \n"
		"Example usage:                                                                \n"
		"   $ ff-slew 240 < image.ff > image-slew.ff                                   \n"
		"   $ ff-slew 240 4000 < image.ff > image-slew.ff                              \n"
		"   $ ff-slew 240 90 4000 5100 < image.ff > image-slew.ff                      \n"
		"\n"
	);
	exit(1);
}

static inline void process(void) {
	static int cur[4];
	static int i,j;
	for(i=0;i<4;i++) cur[i]=(row[x*8+i*2]<<8)|row[x*8+i*2+1];
	if(x) for(i=0;i<4;i++) {
		j=(row[x*8+i*2-8]<<8)|row[x*8+i*2-7];
		if(cur[i]<j-hd) cur[i]=j-hd;
		if(cur[i]>j+hu) cur[i]=j+hu;
	}
	if(y) for(i=0;i<4;i++) {
		j=(row[(x+width)*8+i*2]<<8)|row[(x+width)*8+i*2+1];
		if(cur[i]<j-vd) cur[i]=j-vd;
		if(cur[i]>j+vu) cur[i]=j+vu;
	}
	for(i=0;i<4;i++) row[x*8+i*2]=cur[i]>>8,row[x*8+i*2+1]=cur[i];
}

int main(int argc,char**argv) {
	if (argc>1 && (!strcmp(argv[1],"-h") || !strcmp(argv[1],"--help"))) {
		usage();
	}
	if(argc==2) {
		hu=hd=vu=vd=strtol(argv[1],0,0);
	} else if(argc==3) {
		hu=hd=strtol(argv[1],0,0);
		vu=vd=strtol(argv[2],0,0);
	} else if(argc==5) {
		hu=strtol(argv[1],0,0);
		hd=strtol(argv[2],0,0);
		vu=strtol(argv[3],0,0);
		vd=strtol(argv[4],0,0);
	} else {
		fprintf(stderr,"Improper number of arguments\n");
		return 1;
	}
	fread(buf,1,16,stdin);
	width=(buf[8]<<24)|(buf[9]<<16)|(buf[10]<<8)|buf[11];
	height=(buf[12]<<24)|(buf[13]<<16)|(buf[14]<<8)|buf[15];
	row=calloc(16,width);
	if(!row) {
		fprintf(stderr,"Allocation failed\n");
		return 1;
	}
	fwrite(buf,1,16,stdout);
	for(y=0;y<height;y++) {
		fread(row,8,width,stdin);
		for(x=0;x<width;x++) process();
		fwrite(row,8,width,stdout);
		memcpy(row+width*8,row,width*8);
	}
	return 0;
}

