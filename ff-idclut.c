#if 0
gcc -s -O2 -o ~/bin/ff-idclut -Wno-unused-result ff-idclut.c
exit
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char head[16]={'f','a','r','b','f','e','l','d',0,0,16,0,0,0,16,0};

static void
usage(void)
{
	fprintf(stderr,
		"Usage: ff-idclut                                                              \n"
		"                                                                              \n"
		"This program generates an identity Hald CLUT with a precision of 8-bits per   \n"
		"channel. A Color LookUp Table or CLUT (some times written as \"Color LUT\") is\n"
		"a picture that has a specific colour pattern on it that represents all the    \n"
		"colours in the colour space. This can be used to create colour adjustment     \n"
		"templates that can be applied to other pictures.                              \n"
		"                                                                              \n"
		"An identity CLUT is a CLUT that doesn't affect the image it is applied to.    \n"
		"                                                                              \n"
		"See ff-apclut for how to apply a CLUT to an image.                            \n"
		"                                                                              \n"
		"This program takes no arguments and no input.                                 \n"
		"                                                                              \n"
		"Example usage:                                                                \n"
		"   $ ff-idclut > idclut.ff                                                    \n"
		"\n"
	);
	exit(1);
}

int main(int argc,char**argv) {
	int r,g,b;
	if (argc>1) {
		usage();
	}
	fwrite(head,1,16,stdout);
	for(b=0;b<256;b++) for(g=0;g<256;g++) for(r=0;r<256;r++) {
		putchar(r); putchar(r);
		putchar(g); putchar(g);
		putchar(b); putchar(b);
		putchar(255); putchar(255);
	}
	return 0;
}

