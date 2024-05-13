#if 0
gcc -s -O2 -o ~/bin/ff-noise -Wno-unused-result ff-noise.c
exit
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned char buf[16];
static char opt[128];
static int prob,minmix,maxmix;

static void
usage(void)
{
	fprintf(stderr,
		"Usage: ff-noise <seed> <probability> <min> <max> <?options?>                  \n"
		"                                                                              \n"
		"This farbfeld filter adds random noise to a picture.                          \n"
		"                                                                              \n"
		"The filter takes the following arguments:                                     \n"
		"   seed -- a random number                                                    \n"
		"   probability -- probability of adding noise to each pixel (0 to 32767)      \n"
		"   min -- minimum mixture (0 to 256)                                          \n"
		"   max -- maximum mixture (0 to 256)                                          \n"
		"   options -- (one letter each) (optional)                                    \n"
		"                                                                              \n"
		"The option letters are:                                                       \n"
		"   r -- affect red channel                                                    \n"
		"   g -- affect green channel                                                  \n"
		"   b -- affect blue channel                                                   \n"
		"   a -- affect alpha channel                                                  \n"
		"   d -- noise values are mandatorily less than half                           \n"
		"   l -- noise values are mandatorily greater than half                        \n"
		"   p -- force the noise to be either minimum or maximum rather than numbers in\n"
		"        between                                                               \n"
		"   i -- the mixture amount is the same for all channels rather than           \n"
		"        independent per channel                                               \n"
		"   s -- keep brightness approximately the same                                \n"
		"   y -- the noise colour is the same for all channels rather than independent \n"
		"        per channel                                                           \n"
		"   q -- sets the noise colour to that of the previous colour rather than a    \n"
		"        random colour - can affect other options such as p (it is unclear what\n"
		"        the intended use for this option is as it is not documented)          \n"
		"   u -- sets previous colour to that of the noise colour instead of the actual\n"
		"        input colour - this affects the q option and has no effect without it \n"
		"                                                                              \n"
		"If the channels are not specified then it will default to affect red, green   \n"
		"and blue channels, but not alpha.                                             \n"
		"                                                                              \n"
		"Example usage:                                                                \n"
		"   $ ff-noise 5912 23982 2 255 l < image.ff > image-noise.ff                  \n"
		"   $ ff-noise 4233 19292 30 248 si < image.ff > image-noise.ff                \n"
		"\n"
	);
	exit(1);
}
static inline void process(void) {
	static int pcol[4]={32768,32768,32768,32768};
	int icol[4]={(buf[0]<<8)|buf[1],(buf[2]<<8)|buf[3],(buf[4]<<8)|buf[5],(buf[6]<<8)|buf[7]};
	int ncol[4]={random()&65535,random()&65535,random()&65535,random()&65535};
	int i,j;
	if(opt['q']) ncol[0]=pcol[0],ncol[1]=pcol[1],ncol[2]=pcol[2],ncol[3]=pcol[3];
	if(opt['y']) ncol[0]=ncol[1]=ncol[2]=ncol[3];
	if(opt['d']) ncol[0]&=32767,ncol[1]&=32767,ncol[2]&=32767;
	if(opt['l']) ncol[0]|=32768,ncol[1]|=32768,ncol[2]|=32768;
	if(opt['p']) {
		if(ncol[0]&32768) ncol[0]=65535; else ncol[0]=0;
		if(ncol[1]&32768) ncol[1]=65535; else ncol[1]=0;
		if(ncol[2]&32768) ncol[2]=65535; else ncol[2]=0;
		if(ncol[3]&32768) ncol[3]=65535; else ncol[3]=0;
	}
	for(i=0;i<4;i++) {
		if(!i || !opt['i']) j=random()%(maxmix+1-minmix)+minmix;
		ncol[i]=(icol[i]*j+ncol[i]*(256-j))>>8;
	}
	if(opt['s']) {
		i=icol[0]+icol[1]+icol[2];
		j=ncol[0]+ncol[1]+ncol[2];
		ncol[0]+=(i-j+1)/3;
		ncol[1]+=(i-j+1)/3;
		ncol[2]+=(i-j+1)/3;
		if(ncol[0]<0) ncol[0]=0;
		if(ncol[0]>65535) ncol[0]=65535;
		if(ncol[1]<0) ncol[1]=0;
		if(ncol[1]>65535) ncol[1]=65535;
		if(ncol[2]<0) ncol[2]=0;
		if(ncol[2]>65535) ncol[2]=65535;
	}
	if(opt['r']) buf[0]=ncol[0]>>8,buf[1]=ncol[0];
	if(opt['g']) buf[2]=ncol[1]>>8,buf[3]=ncol[1];
	if(opt['b']) buf[4]=ncol[2]>>8,buf[5]=ncol[2];
	if(opt['a']) buf[6]=ncol[3]>>8,buf[7]=ncol[3];
	if(opt['u']) pcol[0]=ncol[0],pcol[1]=ncol[1],pcol[2]=ncol[2],pcol[3]=ncol[3];
	else pcol[0]=icol[0],pcol[1]=icol[1],pcol[2]=icol[2],pcol[3]=icol[3];
}

int main(int argc,char**argv) {
	int i;
	if (argc<5 || !strcmp(argv[1],"-h") || !strcmp(argv[1],"--help")) {
		usage();
	}
	srandom(strtol(argv[1],0,0));
	prob=strtol(argv[2],0,0);
	minmix=strtol(argv[3],0,0);
	maxmix=strtol(argv[4],0,0);
	if(argc>5) for(i=0;argv[5][i];i++) opt[argv[5][i]&127]=1;
	if(!opt['r'] && !opt['g'] && !opt['b'] && !opt['a']) opt['r']=opt['g']=opt['b']=1;
	fread(buf,1,16,stdin);
	fwrite(buf,1,16,stdout);
	while(fread(buf,1,8,stdin)>0) {
		if((random()&32767)<prob) process();
		fwrite(buf,1,8,stdout);
	}
	return 0;
}

