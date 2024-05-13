#if 0
gcc -s -O2 -o ~/bin/ff-elementary -Wno-unused-result ff-elementary.c
exit
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned char buf[16];
static unsigned char rule[3];
static unsigned char*rowin;
static unsigned char*rowout;
static int width;

static void
usage(void)
{
	fprintf(stderr,
		"Usage: ff-elementary <wolfram> <?wolfram?> <?wolfram?>                        \n"
		"                                                                              \n"
		"This farbfeld filter implements elementary cellular automata.                 \n"
		"                                                                              \n"
		"It takes one or three arguments, each of which is the Wolfram code (a number  \n"
		"from 0 to 255) of the rule to use. If there are three arguments, the first is \n"
		"for the red output, second for green output, and third for blue output.       \n"
		"                                                                              \n"
		"This program uses only the high bit of each channel of input, and the output  \n"
		"uses only one bit per channel (the other bits copy the high bit).             \n"
		"                                                                              \n"
		"The top scanline is not altered by this program (except to reduce it to one   \n"
		"bit per channel). For the other scanlines, it is not altered if the alpha     \n"
		"channel is low, but is replaced with the calculation from the above pixels if \n"
		"the alpha channel is high. The new value is computed from the red channel of  \n"
		"the above and left pixel, the green channel of the above pixel, and the blue  \n"
		"channel of the above and right pixel. The left and right is considered to wrap\n"
		"around for this purpose.                                                      \n"
		"                                                                              \n"
		"Tip: You can use ff-gradient with mode n to generate a random initial         \n"
		"condition.                                                                    \n"
		"                                                                              \n"
		"Wrapping can be avoided by using ff-border to add a transparent border to the \n"
		"left and right edges of the picture.                                          \n"
		"                                                                              \n"
		"Example usage:                                                                \n"
		"   $ ff-elementary 11 < image.ff > elementary.ff                              \n"
		"   $ ff-elementary 13 83 4 < image.ff > elementary.ff                         \n"
		"\n"
	);
	exit(1);
}

int main(int argc,char**argv) {
	int i,j;
	if (argc>1 && (!strcmp(argv[1],"-h") || !strcmp(argv[1],"--help"))) {
		usage();
	}
	if(argc==2) {
		rule[0]=rule[1]=rule[2]=strtol(argv[1],0,0);
	} else if(argc==4) {
		rule[0]=strtol(argv[1],0,0);
		rule[1]=strtol(argv[2],0,0);
		rule[2]=strtol(argv[3],0,0);
	} else {
		fprintf(stderr,"Improper number of arguments\n");
		return 1;
	}
	fread(buf,1,16,stdin);
	width=(buf[8]<<24)|(buf[9]<<16)|(buf[10]<<8)|buf[11];
	if(!(rowout=malloc(width<<1))) {
		fprintf(stderr,"Allocation failed\n");
		return 1;
	}
	rowin=rowout+width;
	fwrite(buf,1,16,stdout);
	for(i=0;i<width;i++) {
		fread(buf,1,8,stdin);
		rowout[i]=((buf[6]>>4)&8)|((buf[0]>>5)&4)|((buf[2]>>6)&2)|((buf[4]>>7)&1);
	}
	for(;;) {
		for(i=0;i<width;i++) {
			j=rowin[i]=rowout[i];
			putchar(j&4?255:0); putchar(j&4?255:0);
			putchar(j&2?255:0); putchar(j&2?255:0);
			putchar(j&1?255:0); putchar(j&1?255:0);
			putchar(j&8?255:0); putchar(j&8?255:0);
		}
		for(i=0;i<width;i++) {
			if(fread(buf,1,8,stdin)<=0) return 0;
			if(buf[6]&128) {
				rowout[i]=8;
				j=(rowin[(i+1)%width]&1)|(rowin[i]&2)|(rowin[(i+width-1)%width]&4);
				if(rule[0]&(1<<j)) rowout[i]|=4;
				if(rule[1]&(1<<j)) rowout[i]|=2;
				if(rule[2]&(1<<j)) rowout[i]|=1;
			} else {
				rowout[i]=((buf[0]>>5)&4)|((buf[2]>>6)&2)|((buf[4]>>7)&1);
			}
		}
	}
}

