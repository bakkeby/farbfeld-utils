#if 0
gcc -s -O2 -o ~/bin/ff-maxrgb -Wno-unused-result ff-maxrgb.c
exit
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned char buf[16];
static int mask,value;

static void
usage(void)
{
	fprintf(stderr,
		"Usage: ff-maxrgb [?rgbam?] <?colour?>                                         \n"
		"                                                                              \n"
		"This farbfeld filter is used to keep only one channel; the channel that has   \n"
		"the highest value or optionally the channel that has the lowest value.        \n"
		"                                                                              \n"
		"The filter has two optional arguments.                                        \n"
		"                                                                              \n"
		"The first argument is up to five letters: r, g, b, a and/or m.                \n"
		"   r -- red channel                                                           \n"
		"   g -- green channel                                                         \n"
		"   b -- blue channel                                                          \n"
		"   a -- alpha channel                                                         \n"
		"   m -- minimum - use the channel with the lowest value                       \n"
		"                                                                              \n"
		"The default is \"rgb\" if no arguments are specified.                         \n"
		"Only the channels specified will be considered for checking highest/lowest and\n"
		"for altering; other channels will remain untouched.                           \n"
		"                                                                              \n"
		"If \"m\" is specified then the channel with the lowest value is kept.         \n"
		"Otherwise, by default, the channel with the highest value is kept.            \n"
		"                                                                              \n"
		"The second argument is a number 0 to 65535, specifying the colour value to use\n"
		"for channels that are altered rather than kept. The default value is zero.    \n"
		"                                                                              \n"
		"In some scenarios there may be more than one channel that has the highest (or \n"
		"lowest) value. In such a case all of those channels are kept and the rest (if \n"
		"any) are altered.                                                             \n"
		"                                                                              \n"
		"Example usage:                                                                \n"
		"   $ ff-maxrgb < image.ff > image-maxrgb.ff                                   \n"
		"   $ ff-maxrgb rgbm < image.ff > image-minrgb.ff                              \n"
		"   $ ff-maxrgb rb 124 < image.ff > image-maxrb.ff                             \n"
		"\n"
	);
	exit(1);
}

static inline void
process(void)
{
	int i,j,y;
	unsigned short x=mask&16?65535:0;
	for (i=0,j=1; i<4; i++,j+=j) if (mask&j) {
		y=(buf[i+i]<<8)|buf[i+i+1];
		if (mask&16?(x>y):(x<y)) x=y;
	}
	for (i=0,j=1; i<4; i++,j+=j) if (mask&j) {
		y=(buf[i+i]<<8)|buf[i+i+1];
		if (x!=y) {
			buf[i+i]=value>>8;
			buf[i+i+1]=value;
		}
	}
	fwrite(buf,1,8,stdout);
}

int
main(int argc, char **argv)
{
	int i;
	if (argc>1) {
		if (!strcmp(argv[i+1],"--help") || !strcmp(argv[i+1],"-h")) {
			usage();
		}
		for (i=0; argv[1][i]; i++) {
			if (argv[1][i]=='r') mask|=1;
			if (argv[1][i]=='g') mask|=2;
			if (argv[1][i]=='b') mask|=4;
			if (argv[1][i]=='a') mask|=8;
			if (argv[1][i]=='m') mask|=16;
		}
	} else {
		mask=7;
	}
	if (argc>2) value=strtol(argv[2],0,0);
	fread(buf,1,16,stdin);
	fwrite(buf,1,16,stdout);
	while (fread(buf,1,8,stdin)) process();
	return 0;
}
