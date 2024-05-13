#if 0
gcc -s -O2 -o ~/bin/ff-tohs -Wno-unused-result ff-tohs.c
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
		"Usage: ff-tohs <mode>                                                         \n"
		"                                                                              \n"
		"This farbfeld filter converts RGB colours to HSV.                             \n"
		"                                                                              \n"
		"HSV is short for hue, saturation and value.                                   \n"
		"                                                                              \n"
		"See ff-frhs for converting back to RGB.                                       \n"
		"                                                                              \n"
		"The filter takes exactly one argument which is the mode.                      \n"
		"                                                                              \n"
		"The available modes are:                                                      \n"
		"   i --                                                                       \n"
		"   l --                                                                       \n"
		"   v --                                                                       \n"
		"   - -- no mode (do not use any of the above modes)                           \n"
		"                                                                              \n"
		"The modes affect how the RGB input is converted to HSV.                       \n"
		"                                                                              \n"
		"Example usage:                                                                \n"
		"   $ ff-tohs l < image.ff > image-hs.ff                                       \n"
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
		int r=(buf[0]<<8)|buf[1];
		int g=(buf[2]<<8)|buf[3];
		int b=(buf[4]<<8)|buf[5];
		int M=r>g?r>b?r:b:b>g?b:g;
		int m=r<g?r<b?r:b:b<g?b:g;
		int v=(mode=='i'?(r+g+b)/3:mode=='v'?M:mode=='l'?(M+m)>>1:M);
		/* This program had a tendency to crash with illegal hardware instructions or floating
		 * point exceptions due to dividing by (M-m). It doesn't make much sense, but moving
		 * the calculation out to a variable presumably helps with compiler optimisations.
		 * Added the Mmm variable. */
		int Mmm = M-m;
		int s=(mode=='i'?(M?65535-((196605LL*m)/(r+g+b)):0):mode=='v'?(65535LL*(M-m))/M:mode=='l'?(v==65535?0:(32768LL*(M-m))/(v&32768?65535-v:v)):m);
		int H=(((M==m?0:M==r?g-b:M==g?b-r:r-g)<<13)/(Mmm)+(M==r?0xC000:M==g?0x4000:0x8000))%0xC000;
		s=s<0?0:s>65535?65535:s;
		v=v<0?0:v>65535?65535:v;
		buf[0]=H>>8;
		buf[1]=H;
		buf[2]=s>>8;
		buf[3]=s;
		buf[4]=v>>8;
		buf[5]=v;
		fwrite(buf,1,8,stdout);
	}
	return 0;
}
