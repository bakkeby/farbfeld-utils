#if 0
gcc -s -O2 -o ~/bin/ff-probaut -Wno-unused-result ff-probaut.c
exit
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define fatal(...) do{ fprintf(stderr,__VA_ARGS__); exit(1); }while(0)

static unsigned char buf[16];
static int width,height;
static unsigned char*row;

static void
usage(void)
{
	fprintf(stderr,
		"Usage: ff-probaut <?seed?>                                                    \n"
		"                                                                              \n"
		"This farbfeld filter is used to generate probabilistic cellular automata.     \n"
		"                                                                              \n"
		"Takes an input picture where the pixels specify the probabilities. The output \n"
		"is only with black and white pixels (all opaque), and is same size as the     \n"
		"input.                                                                        \n"
		"                                                                              \n"
		"It starts with random data for the row above the first scanline. After that,  \n"
		"to determine the new colour of each pixel, it uses the colour up and left and \n"
		"the colour up and right (wrapping left and right) to determine which channel  \n"
		"of the input specifies the probability to use, as follows:                    \n"
		"   - if the left pixel is black and right pixel is black, use red channel     \n"
		"   - if the left pixel is white and right pixel is black, use green channel   \n"
		"   - if the left pixel is black and right pixel is white, use blue channel    \n"
		"   - if the left pixel is white and right pixel is white, use alpha channel   \n"
		"                                                                              \n"
		"The filter takes one argument which is a random seed.                         \n"
		"                                                                              \n"
		"Example usage:                                                                \n"
		"   $ ff-probaut < image.ff > image-probaut.ff                                 \n"
		"   $ ff-probaut 23 < image.ff > image-probaut.ff                              \n"
		"\n"
	);
	exit(1);
}

static void process(void) {
	int w,x,y,z;
	w=row[width-1];
	for(x=0;x<width;x++) {
		fread(buf,1,8,stdin);
		y=(row[(x+1)%width]<<1)|(w<<2);
		w=row[x];
		y=(buf[y]<<8)|buf[y+1];
		z=random()&0xFFFF;
		if(z<y || y==0xFFFF) {
			row[x]=1;
			putchar(255); putchar(255);
			putchar(255); putchar(255);
			putchar(255); putchar(255);
		} else {
			row[x]=0;
			putchar(0); putchar(0);
			putchar(0); putchar(0);
			putchar(0); putchar(0);
		}
		putchar(255); putchar(255);
	}
}

int main(int argc,char**argv) {
	int i;
	if(argc>2) usage();
	if(argc==2) srandom(strtol(argv[1],0,0));
	fread(buf,1,16,stdin);
	width=(buf[8]<<24)|(buf[9]<<16)|(buf[10]<<8)|buf[11];
	height=(buf[12]<<24)|(buf[13]<<16)|(buf[14]<<8)|buf[15];
	row=malloc(width);
	if(!row) fatal("Allocation failed\n");
	fwrite(buf,1,16,stdout);
	for(i=0;i<width;i++) row[i]=random()&1;
	for(i=0;i<height;i++) process();
	return 0;
}
