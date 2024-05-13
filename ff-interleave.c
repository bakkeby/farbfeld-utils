#if 0
gcc -s -O2 -o ~/bin/ff-interleave -Wno-unused-result ff-interleave.c
exit
#endif

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void
usage(void)
{
	fprintf(stderr,
		"Usage: ff-interleave <image> <image> <?option?>                               \n"
		"                                                                              \n"
		"This program interleaves two pictures and writes the result to stdout.        \n"
		"                                                                              \n"
		"No data is read from stdin, and both pictures must be of the same size.       \n"
		"                                                                              \n"
		"An optional third argument specifies the mode of interleaving:                \n"
		"  w -- interleave individual channels                                         \n"
		"  x -- interleave columns                                                     \n"
		"  y -- interleave scanlines (default)                                         \n"
		"                                                                              \n"
		"Example usage:                                                                \n"
		"   $ ff-interleave image1.ff image2.ff > image-interleaved.ff                 \n"
		"   $ ff-interleave image1.ff image2.ff w > image-interleaved.ff               \n"
		"   $ ff-interleave image1.ff image2.ff x > image-interleaved.ff               \n"
		"\n"
	);
	exit(1);
}

int main(int argc,char**argv) {
	unsigned char buf[32];
	unsigned char*row1;
	unsigned char*row2;
	int w,h,i;
	FILE*f1;
	FILE*f2;
	if (argc<3 || !strcmp(argv[1],"-h") || !strcmp(argv[1],"--help")) {
		usage();
	}
	f1=fopen(argv[1],"r");
	i=errno;
	f2=fopen(argv[2],"r");
	if(!f1 || !f2) {
		if(i) errno=i;
		perror(argv[i?1:2]);
		return 1;
	}
	if(fread(buf,1,16,f1)!=16 || fread(buf+16,1,16,f2)!=16 || memcmp(buf,buf+16,16)) {
		fprintf(stderr,"Header mismatch\n");
		return 1;
	}
	w=(buf[8]<<24)|(buf[9]<<16)|(buf[10]<<8)|buf[11];
	h=(buf[12]<<24)|(buf[13]<<16)|(buf[14]<<8)|buf[15];
	switch(argc>3?argv[3][0]?:'y':'y') {
		case 'w':
			i=2;
			w<<=1;
			break;
		case 'x':
			i=8;
			w<<=1;
			break;
		case 'y':
			i=w<<3;
			h<<=1;
			break;
		default:
			fprintf(stderr,"Invalid mode: %c\n",argv[3][0]);
			return 1;
	}
	row1=malloc(i);
	row2=malloc(i);
	if(!row1 || !row2) {
		fprintf(stderr,"Allocation failed\n");
		return 1;
	}
	buf[8]=w>>24; buf[9]=w>>16; buf[10]=w>>8; buf[11]=w;
	buf[12]=h>>24; buf[13]=h>>16; buf[14]=h>>8; buf[15]=h;
	fwrite(buf,1,16,stdout);
	while(fread(row1,1,i,f1)>0 && fread(row2,1,i,f2)>0) {
		fwrite(row1,1,i,stdout);
		fwrite(row2,1,i,stdout);
	}
	return 0;
}

