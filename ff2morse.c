#if 0
gcc -s -O2 -o ~/bin/ff2morse -Wno-unused-result ff2morse.c
exit
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int width,height;
static char opt[128];
static unsigned char buf[16];

typedef char Morse[2];

static const Morse morse[128] = {
	[0] = {'E','T'},
	['A'] = {'R','W'},
	['B'] = {'6',0},
	['D'] = {'B','X'},
	['E'] = {'I','A'},
	['G'] = {'Z','Q'},
	['H'] = {'5','4'},
	['I'] = {'S','U'},
	['J'] = {0,'1'},
	['K'] = {'C','Y'},
	['M'] = {'G','O'},
	['N'] = {'D','K'},
	['O'] = {'~','o'},
	['R'] = {'L',0},
	['S'] = {'H','V'},
	['T'] = {'N','M'},
	['U'] = {'F','u'},
	['V'] = {0,'3'},
	['W'] = {'P','J'},
	['Z'] = {'7',0},
	['o'] = {'9','0'},
	['u'] = {0,'2'},
	['~'] = {'8',0},
};

static void
usage(void)
{
	fprintf(stderr,
		"Usage: ff2morse <?option?>                                                    \n"
		"                                                                              \n"
		"This program implements a file format used in one episode of Murdoch Mysteries\n"
		"television show. Each line has letters/numbers which are converted into morse \n"
		"code and then the dots and dashes are converted to pixels (ignoring the breaks\n"
		"between letters); each line of text is a new scanline of the picture. (Only   \n"
		"letters and numbers can be used; punctuations, accented alphabets, etc are not\n"
		"possible).                                                                    \n"
		"                                                                              \n"
		"The program takes one optional argument being any combination of the following\n"
		"letters:                                                                      \n"
		"                                                                              \n"
		"   c: Encodes using a compact format (everything on one line, starting with   \n"
		"      the width number inside of <>).                                         \n"
		"   r: Reverse video. If specified, then a dot is white and a dash is black.   \n"
		"      Normally (if not specified), then a dot is black and a dash is white.   \n"
		"                                                                              \n"
		"Example usage:                                                                \n"
		"   $ ff2morse cr < image.ff                                                   \n"
		"   $ ff2morse r < image.ff > morse.txt                                        \n"
		"\n"
	);
	exit(1);
}

static void
process(void)
{
	int x,y,c,d;
	c=0;
	for (y=0; y<height; y++) {
		for (x=0; x<width; x++) {
			fread(buf,1,8,stdin);
			d=morse[c][(*buf>>7)^opt['r']];
			if (d) {
				c=d;
			} else {
				if (c=='~') c='O',d='E'; else if (c>'Z') c-=32,d='T';
				putchar(c);
				c=morse[d][(*buf>>7)^opt['r']];
			}
		}
		if (c) {
			if (c=='~') putchar('M'),putchar('N');
			else if (c>'Z') putchar(c-32),putchar('T');
			else putchar(c);
			c=0;
		}
		putchar('\n');
	}
}

int
main(int argc, char **argv)
{
	int i;
	if (argc>1) {
		for (i=0; argv[1][i]; i++) {
			if (argv[1][i] != 'c' || argv[1][i] != 'r') {
				usage();
			}
			opt[argv[1][i]&127]=1;
		}
	}
	fread(buf,1,16,stdin);
	if (memcmp(buf,"farbfeld",8)) {
		fprintf(stderr,"Not farbfeld\n");
		return 1;
	}
	width=(buf[8]<<24)|(buf[9]<<16)|(buf[10]<<8)|buf[11];
	height=(buf[12]<<24)|(buf[13]<<16)|(buf[14]<<8)|buf[15];
	if (opt['c']) {
		printf("<%d>",width);
		width*=height;
		height=1;
	}
	process();
	return 0;
}
