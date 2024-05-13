#if 0
gcc -s -O2 -o ~/bin/ff-mux -Wno-unused-result ff-mux.c
exit
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char buf[64];
static FILE*in1;
static FILE*in2;
static unsigned char i,j,k;

static void
usage(void)
{
	fprintf(stderr,
		"Usage: ff-mux <image1.ff> <image2.ff>                                         \n"
		"                                                                              \n"
		"This farbfeld program uses bits of one picture to select between two other    \n"
		"pictures.                                                                     \n"
		"                                                                              \n"
		"The program takes exactly two arguments, which must be two files that have the\n"
		"same size. The picture passed via standard in also have to have the same size.\n"
		"                                                                              \n"
		"Example usage:                                                                \n"
		"   $ ff-mux image1.ff image2.ff < selector.ff > image-mux.ff                  \n"
		"\n"
	);
	exit(1);
}

int main(int argc,char**argv) {
	if (argc>1 && (!strcmp(argv[1],"-h") || !strcmp(argv[1],"--help"))) {
		usage();
	}
	if(argc!=3) {
		fprintf(stderr,"Too %s arguments\n",argc<3?"few":"many");
		return 1;
	}
	if(!(in1=fopen(argv[1],"r"))) {
		perror(argv[1]);
		return 1;
	}
	if(!(in2=fopen(argv[2],"r"))) {
		perror(argv[2]);
		return 1;
	}
	fread(buf,1,16,stdin);
	fread(buf+16,1,16,in1);
	fread(buf+32,1,16,in2);
	if(memcmp(buf,buf+16,16) || memcmp(buf,buf+32,16)) {
		fprintf(stderr,"Header mismatch\n");
		return 1;
	}
	fwrite(buf,1,16,stdout);
	while((i=fgetc(stdin))!=EOF) {
		j=fgetc(in1);
		k=fgetc(in2);
		putchar((k&i)|(j&~i));
	}
	return 0;
}
