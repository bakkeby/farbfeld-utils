#if 0
gcc -s -O2 -o ~/bin/ff-bitmask -Wno-unused-result ff-bitmask.c
exit
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned char buf[16];
static unsigned char andm[8];
static unsigned char xorm[8];
static unsigned int total;

static void
usage(void)
{
	fprintf(stderr,
		"Usage: ff-bitmask <AND mask> <XOR mask>                                       \n"
		"                                                                              \n"
		"This farbfeld filter applies bit mask (AND/XOR) operations to each pixel.     \n"
		"                                                                              \n"
		"The filter needs two arguments; the AND mask and then the XOR mask.           \n"
		"Both masks are hexadecimal in \"rrrrggggbbbbaaaa\" format.                    \n"
		"                                                                              \n"
		"Example usage:                                                                \n"
		"   $ ff-bitmask FFFFFFFF0F0FFFFF 0000000000000000 < image.ff > image-masked.ff\n"
		"\n"
	);
	exit(1);
}

#define Step(n) putchar((getchar()&andm[n])^xorm[n])
int main(int argc,char**argv) {
	int i;
	if (argc!=3 || !strcmp(argv[1],"-h") || !strcmp(argv[1],"--help")) {
		usage();
	}
	sscanf(argv[1],"%2hhX%2hhX%2hhX%2hhX%2hhX%2hhX%2hhX%2hhX",andm+0,andm+1,andm+2,andm+3,andm+4,andm+5,andm+6,andm+7);
	sscanf(argv[2],"%2hhX%2hhX%2hhX%2hhX%2hhX%2hhX%2hhX%2hhX",xorm+0,xorm+1,xorm+2,xorm+3,xorm+4,xorm+5,xorm+6,xorm+7);
	fread(buf,1,16,stdin);
	fwrite(buf,1,16,stdout);
	total=(buf[8]<<24)|(buf[9]<<16)|(buf[10]<<8)|buf[11];
	total*=(buf[12]<<24)|(buf[13]<<16)|(buf[14]<<8)|buf[15];
	while(total--) {
		Step(0);
		Step(1);
		Step(2);
		Step(3);
		Step(4);
		Step(5);
		Step(6);
		Step(7);
	}
	return 0;
}
