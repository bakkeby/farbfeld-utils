#if 0
gcc -s -O2 -o ~/bin/ff-swizzle -Wno-unused-result ff-swizzle.c
exit
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned char buf[16];

static void
usage(void)
{
	fprintf(stderr,
		"Usage: ff-swizzle <rgba>                                                      \n"
		"                                                                              \n"
		"This farbfeld filter is used to swizzle the channels.                         \n"
		"                                                                              \n"
		"Swizzle in this context means changing the channels of a picture, which may   \n"
		"involve swapping, removing or altering channels.                              \n"
		"                                                                              \n"
		"The filter takes one argument, which consists of 4 characters, one for each of\n"
		"the red, green, blue and alpha output.                                        \n"
		"                                                                              \n"
		"The character options are:                                                    \n"
		"   - a lowercase letter r, g, b, a: copy that channel of input                \n"
		"   - an uppercase letter R, G, B, A: copy that channel of input, but negated  \n"
		"   - number 0: all bits clear                                                 \n"
		"   - number 1: all bits set                                                   \n"
		"                                                                              \n"
		"It is also possible to alter the width of the picture less or more than four  \n"
		"characters are used. It can be one, two, or a multiple of four.               \n"
		"                                                                              \n"
		"With one character, each pixel of input becomes one channel of one pixel of   \n"
		"output.                                                                       \n"
		"                                                                              \n"
		"Example combinations:                                                         \n"
		"   rgba - straight copy (no change)                                           \n"
		"   bgra - red and blue channels are swapped                                   \n"
		"   0gba - red channel is removed                                              \n"
		"   00b1 - keep blue channel and make image fully opaque                       \n"
		"   RGBa - invert the image                                                    \n"
		"                                                                              \n"
		"Example usage:                                                                \n"
		"   $ ff-swizzle r001 < image.ff > red-channel.ff                              \n"
		"\n"
	);
	exit(1);
}

int main(int argc,char**argv) {
	int i;
	if (argc<2 || !strcmp(argv[1],"-h") || !strcmp(argv[1],"--help")) {
		usage();
	}
	i=strlen(argv[1]);
	if(!i || i==3 || (i>4 && (i&3))) {
		fprintf(stderr,"Improper swizzle\n");
		return 1;
	}
	fread(buf,1,16,stdin);
	i=(buf[8]<<24)|(buf[9]<<16)|(buf[10]<<8)|buf[11];
	i=(i*strlen(argv[1]))>>2;
	buf[8]=i>>24;
	buf[9]=i>>16;
	buf[10]=i>>8;
	buf[11]=i;
	fwrite(buf,1,16,stdout);
	while(fread(buf,1,8,stdin)) {
		for(i=0;argv[1][i];i++) switch(argv[1][i]) {
			case '0': putchar(0); putchar(0); break;
			case '1': putchar(255); putchar(255); break;
			case 'r': putchar(buf[0]); putchar(buf[1]); break;
			case 'g': putchar(buf[2]); putchar(buf[3]); break;
			case 'b': putchar(buf[4]); putchar(buf[5]); break;
			case 'a': putchar(buf[6]); putchar(buf[7]); break;
			case 'R': putchar(~buf[0]); putchar(~buf[1]); break;
			case 'G': putchar(~buf[2]); putchar(~buf[3]); break;
			case 'B': putchar(~buf[4]); putchar(~buf[5]); break;
			case 'A': putchar(~buf[6]); putchar(~buf[7]); break;
			case '_': default: putchar(128); putchar(0); break;
		}
	}
	return 0;
}

