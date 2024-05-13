#if 0
gcc -s -O2 -o ~/bin/ff-frhs -Wno-unused-result ff-frhs.c
exit
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned char buf[16];
static char mode; // c i l v

static void
usage(void)
{
	fprintf(stderr,
		"Usage: ff-frhs <mode>                                                         \n"
		"                                                                              \n"
		"This farbfeld program converts HSV colours to RGB.                            \n"
		"                                                                              \n"
		"HSV is short for hue, saturation and value.                                   \n"
		"                                                                              \n"
		"See ff-tohs for how to convert RGB colours to HSV.                            \n"
		"                                                                              \n"
		"The filter takes exactly one argument which is the mode.                      \n"
		"                                                                              \n"
		"The available modes are:                                                      \n"
		"   i --                                                                       \n"
		"   l --                                                                       \n"
		"   v --                                                                       \n"
		"   - -- no mode (do not use any of the above modes)                           \n"
		"                                                                              \n"
		"The modes affect how the HSV input is converted to RGB.                       \n"
		"                                                                              \n"
		"It may not be the intended usage, but you can try passing a normal image      \n"
		"through this filter for interesting effects.                                  \n"
		"                                                                              \n"
		"Example usage:                                                                \n"
		"   $ ff-frhs < hsv.dat > rgb.ff                                               \n"
		"   $ ff-frhs < image.ff > image-hsv.ff                                        \n"
		"\n"
	);
	exit(1);
}

int main(int argc,char**argv) {
	if (argc>1 && (!strcmp(argv[1],"-h") || !strcmp(argv[1],"--help"))) {
		usage();
	}
	if(argc!=2 || !argv[1][0] || argv[1][1]) {
		fprintf(stderr,"Incorrect arguments\n");
		return 1;
	}
	mode=argv[1][0];
	fread(buf,1,16,stdin);
	fwrite(buf,1,16,stdout);
	while(fread(buf,1,8,stdin)) {
		int h=(buf[0]<<8)|buf[1];
		int s=(buf[2]<<8)|buf[3];
		long long v=(buf[4]<<8)|buf[5];
		int Z=((h&0x1FFF)<<3)^(h&0x2000?65535:0);
		int C=(mode=='v'?v*s:mode=='l'?(v&32768?65535-v:v)*s:mode=='i'?(196605*v*s)/(Z+65535):v*65535)/65535;
		int X=(C*Z)/65535;
		int m=(mode=='v'?v-C:mode=='l'?v-C/2:mode=='i'?(v*(65535-s))/65535:s);
		switch(h>>13) {
			case 0: buf[0]=(C+m)>>8; buf[1]=C+m; buf[2]=(X+m)>>8; buf[3]=X+m; buf[4]=m>>8; buf[5]=m; break;
			case 1: buf[2]=(C+m)>>8; buf[3]=C+m; buf[0]=(X+m)>>8; buf[1]=X+m; buf[4]=m>>8; buf[5]=m; break;
			case 2: buf[2]=(C+m)>>8; buf[3]=C+m; buf[4]=(X+m)>>8; buf[5]=X+m; buf[0]=m>>8; buf[1]=m; break;
			case 3: buf[4]=(C+m)>>8; buf[5]=C+m; buf[2]=(X+m)>>8; buf[3]=X+m; buf[0]=m>>8; buf[1]=m; break;
			case 4: buf[4]=(C+m)>>8; buf[5]=C+m; buf[0]=(X+m)>>8; buf[1]=X+m; buf[2]=m>>8; buf[3]=m; break;
			case 5: buf[0]=(C+m)>>8; buf[1]=C+m; buf[4]=(X+m)>>8; buf[5]=X+m; buf[2]=m>>8; buf[3]=m; break;
		}
		fwrite(buf,1,8,stdout);
	}
	return 0;
}
