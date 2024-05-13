#if 0
gcc -s -O2 -o ~/bin/ff-icgrid -Wno-unused-result ff-icgrid.c
exit
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static short pal[256];

static unsigned char morton[16]={
	0x00,0x01,0x04,0x05,0x10,0x11,0x14,0x15,
	0x40,0x41,0x44,0x45,0x50,0x51,0x54,0x55,
};

static void
usage(void)
{
	fprintf(stderr,
		"Usage: ff-icgrid <number> <number> ... <number>                               \n"
		"                                                                              \n"
		"This program generates an interleaved colours grid and writes the result to   \n"
		"stdout.                                                                       \n"
		"                                                                              \n"
		"It does not read data from stdin.                                             \n"
		"                                                                              \n"
		"Arguments are a list of 12-bit hexadecimal numbers (values 0 to 4095). The    \n"
		"maximum number of arguments are 255.                                          \n"
		"                                                                              \n"
		"Example usage:                                                                \n"
		"   $ ff-icgrid $(shuf -i1-4095 -n255 -r -z | xargs -0) > icgrid.ff            \n"
		"   $ ff-icgrid $(for I in $(seq 85); do printf '950 21 9 '; done) > icgrid.ff \n"
		"\n"
	);
	exit(1);
}

int main(int argc,char**argv) {
	int i,j,k;
	if (argc>1 && (!strcmp(argv[1],"-h") || !strcmp(argv[1],"--help"))) {
		usage();
	}
	if(argc<1 || argc>257) {
		fprintf(stderr,"Incorrect number of arguments\n");
		return 1;
	}
	for(i=0;i<argc-1;i++) pal[i]=strtol(argv[i+1],0,16);
	fwrite("farbfeld\0\0\0",1,11,stdout);
	putchar(argc-1);
	fwrite("\0\0",1,3,stdout);
	putchar(argc-1);
	for(i=0;i<argc-1;i++) for(j=0;j<argc-1;j++) {
		putchar(k=morton[(pal[i]>>8)&15]|(morton[(pal[j]>>8)&15]<<1)); putchar(k);
		putchar(k=morton[(pal[i]>>4)&15]|(morton[(pal[j]>>4)&15]<<1)); putchar(k);
		putchar(k=morton[(pal[i]>>0)&15]|(morton[(pal[j]>>0)&15]<<1)); putchar(k);
		putchar(255); putchar(255);
	}
	return 0;
}
