#if 0
gcc -s -O2 -o ~/bin/ff-wood -Wno-unused-result ff-wood.c -lm
exit
#endif

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef int(*ModeFunc)(double x,double y,double p,double z);

static unsigned char buf[16];
static int width,height;
static char mode;
static double xperiod,yperiod,extra;
static double power[4];
static ModeFunc fmode;

static void
usage(void)
{
	fprintf(stderr,
		"Usage: ff-wood <mode:dec> <xperiod> <yperiod> <power> <power> <power> <power> \n"
		"                                                                              \n"
		"This farbfeld filter applies wood or marble using random turbulence data.     \n"
		"                                                                              \n"
		"The image output is always black and white.                                   \n"
		"                                                                              \n"
		"The filter takes exactly 7 arguments:                                         \n"
		"   - mode - specifies the algorithm to use                                    \n"
		"     optionally the mode can be followed by a decimal value that can affect   \n"
		"     the output for modes k and w                                             \n"
		"   - xperiod - how often to render the pattern on the x-axis                  \n"
		"   - yperiod - how often to render the pattern on the y-axis                  \n"
		"   - power - multiplier for the red channel on the x-axis                     \n"
		"   - power - multiplier for the green channel on the y axis                   \n"
		"   - power - multiplier for the blue channel (no effect on modes m and x)     \n"
		"   - power - multiplier for the alpha channel (no effect on modes m and x)    \n"
		"                                                                              \n"
		"The periods and the multipliers have various effects on the generated output. \n"
		"                                                                              \n"
		"The available modes are:                                                      \n"
		"   k -- bars radiating from the center                                        \n"
		"   m -- bars cutting across the picture (/////)                               \n"
		"   w -- rings radiating from the center                                       \n"
		"   x -- crossed fence (XXXX), may also look like zig-zag (VVVVV)              \n"
		"                                                                              \n"
		"Example usage:                                                                \n"
		"   $ ff-wood k1.59 8 5 1.0 0.9 0.4 0.2 < image.ff > image-wood.ff             \n"
		"   $ ff-wood m 13 3 0.9 2.8 1.1 0.3 < image.ff > image-marble.ff              \n"
		"   $ ff-wood w9.3 15 5 0.9 1.8 3.1 0.2 < image.ff > image-wood.ff             \n"
		"   $ ff-wood x 24 12 1.9 0.8 2.1 0.8 < image.ff > image-marble.ff             \n"
		"\n"
	);
	exit(1);
}

static int mode_k(double x,double y,double p,double z) {
	return 65535.5*fabs(sin(z*fmod(atan2(y,x)+M_PI*(p+1.0),M_PI)));
}

static int mode_m(double x,double y,double p,double z) {
	return 65535.5*fabs(sin(x+y));
}

static int mode_w(double x,double y,double p,double z) {
	double d=hypot(x,y)+p;
	return 65535.5*fabs(sin(d*z));
}

static int mode_x(double x,double y,double p,double z) {
	return 32767.75*fabs(sin(x)+sin(y));
}

static ModeFunc modes[128]={
	['k']=mode_k,
	['m']=mode_m,
	['w']=mode_w,
	['x']=mode_x,
};

int main(int argc,char**argv) {
	int x,y,i;
	if (argc>1 && (!strcmp(argv[1],"-h") || !strcmp(argv[1],"--help"))) {
		usage();
	}
	if(argc!=8) {
		fprintf(stderr,"Too %s arguments\n",argc<8?"few":"many");
		return 1;
	}
	mode=argv[1][0];
	if(mode<0 || mode>=127 || !modes[mode]) {
		fprintf(stderr,"Mode value out of range\n");
		return 1;
	}
	fmode=modes[mode];
	fread(buf,1,16,stdin);
	width=(buf[8]<<24)|(buf[9]<<16)|(buf[10]<<8)|buf[11];
	height=(buf[12]<<24)|(buf[13]<<16)|(buf[14]<<8)|buf[15];
	extra=strtod(argv[1]+1,0);
	xperiod=strtod(argv[2],0)/(double)width;
	yperiod=strtod(argv[3],0)/(double)height;
	for(i=0;i<4;i++) power[i]=strtod(argv[i+4],0)/65535.0;
	fwrite(buf,1,16,stdout);
	for(y=0;y<height;y++) for(x=0;x<width;x++) {
		fread(buf,1,8,stdin);
		i=fmode(
			(x-0.5*width)*xperiod+power[0]*((buf[0]<<8)|buf[1]),
			(y-0.5*height)*yperiod+power[1]*((buf[2]<<8)|buf[3]),
			power[2]*((buf[4]<<8)|buf[5]),
			extra+power[3]*((buf[6]<<8)|buf[7])
		);
		i=i<0?0:i>65535?65535:i;
		buf[0]=buf[2]=buf[4]=i>>8;
		buf[1]=buf[3]=buf[5]=i;
		buf[6]=buf[7]=255;
		fwrite(buf,1,8,stdout);
	}
	return 0;
}

