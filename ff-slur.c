#if 0
gcc -s -O2 -o ~/bin/ff-slur -Wno-unused-result ff-slur.c
exit
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned char buf[16];
static int width,height;
static unsigned char*pic;
static int p1,p2,p3,p4;

static void
usage(void)
{
	fprintf(stderr,
		"Usage: ff-slur <seed> <probability> <probability> <probability> <probability> \n"
		"                                                                              \n"
		"This farbfeld filter applies a random melting like filter.                    \n"
		"                                                                              \n"
		"The filter takes exactly five arguments:                                      \n"
		"   - a random seed value                                                      \n"
		"   - probability 1                                                            \n"
		"   - probability 2                                                            \n"
		"   - probability 3                                                            \n"
		"   - probability 4                                                            \n"
		"                                                                              \n"
		"The four probabilities affect the strength of the filter and are integer      \n"
		"values in the range from 0 to 255.                                            \n"
		"                                                                              \n"
		"The first probability will be added to the second, and the second probability \n"
		"will be added to the third. That means that by providing 30 20 40 the end     \n"
		"result will be that p1 = 30, p2 = 50 and p3 = 90. This is something to keep in\n"
		"mind when adjusting the values.                                               \n"
		"                                                                              \n"
		"The fourth probability is standalone and is not affected by the other three.  \n"
		"                                                                              \n"
		"Example usage:                                                                \n"
		"   $ ff-slur 313 130 90 30 150 < image.ff > image-slur.ff                     \n"
		"\n"
	);
	exit(1);
}

int main(int argc,char**argv) {
	int i,j,k,x;
	if(argc!=6 || !strcmp(argv[1],"-h") || !strcmp(argv[1],"--help")) {
		usage();
	}
	srandom(strtol(argv[1],0,0));
	p1=strtol(argv[2],0,0);
	p2=p1+strtol(argv[3],0,0);
	p3=p2+strtol(argv[4],0,0);
	p4=strtol(argv[5],0,0);
	fread(buf,1,16,stdin);
	width=(buf[8]<<24)|(buf[9]<<16)|(buf[10]<<8)|buf[11];
	height=(buf[12]<<24)|(buf[13]<<16)|(buf[14]<<8)|buf[15];
	pic=malloc(width<<4); // two rows
	if(!pic) {
		fprintf(stderr,"Allocation failed\n");
		return 1;
	}
	fwrite(buf,1,16,stdout);
	fread(pic,8,width,stdin);
	memcpy(pic+8*width,pic,8*width);
	while(height--) {
		j=height&1?0:8*width;
		k=height&1?8*width:0;
		for(i=0;i<width;i++) {
			x=random()&255;
			if(x<p3) {
				memcpy(buf,pic+k+(x<p1?(i?-8:0):x<p2?0:i==width-1?0:8),8);
				fwrite(buf,1,8,stdout);
				if((random()&255)<p4) memcpy(pic+j,buf,8);
			} else {
				fwrite(pic+j,1,8,stdout);
			}
			j+=8;
			k+=8;
		}
		fread(pic+(height&1?8*width:0),8,width,stdin);
	}
	return 0;
}
