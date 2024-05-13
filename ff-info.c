#if 0
gcc -s -O2 -o ~/bin/ff-info -Wno-unused-result ff-info.c
exit
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned char buf[16];

#define width (buf[8]<<24)|(buf[9]<<16)|(buf[10]<<8)|buf[11]
#define height (buf[12]<<24)|(buf[13]<<16)|(buf[14]<<8)|buf[15]

static void
usage(void)
{
	fprintf(stderr,
		"Usage: ff-info <?option?>                                                     \n"
		"                                                                              \n"
		"This program prints the height and/or width of a farbfeld image.              \n"
		"                                                                              \n"
		"ff-info optionally takes a single argument which is either \"w\" to only print\n"
		"the width or \"h\" to only print the height.                                  \n"
		"                                                                              \n"
		"If no arguments are provided then both the width and height will be printed   \n"
		"separated with a space character.                                             \n"
		"                                                                              \n"
		"The output is decimal ASCII followed by a line feed (newline).                \n"
		"                                                                              \n"
		"Example usage:                                                                \n"
		"   $ ff-info < image.ff                                                       \n"
		"   $ ff-info w < image.ff                                                     \n"
		"   $ ff-info h < image.ff                                                     \n"
		"\n"
	);
	exit(1);
}

int main(int argc,char**argv) {
	if (argc>1 && (!strcmp(argv[1],"-h") || !strcmp(argv[1],"--help"))) {
		usage();
	}
	fread(buf,1,16,stdin);
	if(argc>1 && argv[1][0]=='w') printf("%d\n",width);
	else if(argc>1 && argv[1][0]=='h') printf("%d\n",height);
	else printf("%d %d\n",width,height);
	return 0;
}
