#if 0
gcc -s -O2 -o ~/bin/ff-bright ff-bright.c -lm
exit
#endif

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned char buf[16];
static char chan;
static double param[3];

static void
usage(void)
{
	fprintf(stderr,
		"Usage: ff-bright <?channels?> <?add?> <?multiply?> <?exponent?>               \n"
		"                                                                              \n"
		"This farbfeld filter is used to add/multiply/exponent pixel values by a       \n"
		"constant to adjust brightness for one or more channels.                       \n"
		"                                                                              \n"
		"It scales the values to the range 0 to 1, and then after applying the         \n"
		"specified arithmetic will clip to that range and expand back to a 16-bit      \n"
		"integer.                                                                      \n"
		"                                                                              \n"
		"The first argument specifies which channels to affect; use one or more of the \n"
		"letters r, g, b, and a.                                                       \n"
		"                                                                              \n"
		"Second argument is amount to add, default is 0.0. Greater than 0.0 will result\n"
		"in a brighter image and less than 0.0 will result in a darker image.          \n"
		"                                                                              \n"
		"Third argument is amount to multiply by, default to 1.0. Greater than 1.0 will\n"
		"result in a brigher image and less than 1.0 will result in a darker image.    \n"
		"                                                                              \n"
		"Fourth argument is amount to raise to the power of, default to 1.0. Greater   \n"
		"than 1.0 will result in a darker image and less than 1.0 will result in a     \n"
		"brighter image.                                                               \n"
		"                                                                              \n"
		"Order of operation is first exponent, and then multiply, and then add.        \n"
		"                                                                              \n"
		"Example usage:                                                                \n"
		"   $ ff-bright rgb 0.25 < image.ff > image-bright.ff                          \n"
		"   $ ff-bright rgb -0.25 < image.ff > image-dark.ff                           \n"
		"   $ ff-bright g 0.0 1.1 < image.ff > image-green-hue.ff                      \n"
		"   $ ff-bright b 0.0 1.0 0.04 < image.ff > image-blue-hue.ff                  \n"
		"   $ ff-bright rb 0.0 1.0 0.4 < image.ff > image-increase-red-and-blue.ff     \n"
		"\n"
	);
	exit(1);
}

static void process(int pos) {
	int iv=(buf[pos]<<8)|buf[pos+1];
	double v=((double)iv)/65535.0;
	v=param[1]*pow(v,param[2])+param[0];
	iv=v*65535.0;
	if(iv<0) iv=0;
	if(iv>65535) iv=65535;
	buf[pos]=iv>>8;
	buf[pos+1]=iv;
}

int main(int argc,char**argv) {
	int i;
	if (argc>1 && (!strcmp(argv[1],"-h") || !strcmp(argv[1],"--help"))) {
		usage();
	}
	i=fread(buf,1,16,stdin);
	i=fwrite(buf,1,16,stdout);
	if(argc>1) {
		chan=0;
		for(i=0;argv[1][i];i++) {
			if(argv[1][i]=='r') chan|=1;
			if(argv[1][i]=='g') chan|=2;
			if(argv[1][i]=='b') chan|=4;
			if(argv[1][i]=='a') chan|=8;
		}
	} else {
		chan=7;
	}
	param[0]=argc>2?strtod(argv[2],0):0.0;
	param[1]=argc>3?strtod(argv[3],0):1.0;
	param[2]=argc>4?strtod(argv[4],0):1.0;
	while(fread(buf,1,8,stdin)) {
		if(chan&1) process(0);
		if(chan&2) process(2);
		if(chan&4) process(4);
		if(chan&8) process(6);
		i=fwrite(buf,1,8,stdout);
	}
	return 0;
}

