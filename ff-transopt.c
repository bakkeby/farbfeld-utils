#if 0
gcc -s -O2 -o ~/bin/ff-transopt -Wno-unused-result ff-transopt.c
exit
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned char buf[8];
static int cutoff=0;

static void
usage(void)
{
	fprintf(stderr,
		"Usage: ff-transopt <?threshold?>                                              \n"
		"                                                                              \n"
		"This farbfeld program changes transparent pixels to zero (for optimization).  \n"
		"                                                                              \n"
		"If the alpha channel is zero, then the red, green, and blue channels also are \n"
		"set to zero.                                                                  \n"
		"                                                                              \n"
		"Doing this may allow for better compression.                                  \n"
		"                                                                              \n"
		"Optionally a threshold value can be specified; if the alpha channel is less or\n"
		"equal to the threshold value then all channels are set to zero.               \n"
		"                                                                              \n"
		"Example usage:                                                                \n"
		"   $ ff-transopt < image.ff > image-transopt.ff                               \n"
		"   $ ff-transopt 24000 < image.ff > image-transopt.ff                         \n"
		"\n"
	);
	exit(1);
}

int main(int argc,char**argv) {
	if (argc>1 && (!strcmp(argv[1],"-h") || !strcmp(argv[1],"--help"))) {
		usage();
	}
	if(argc>1) cutoff=strtol(argv[1],0,0);
	fread(buf,1,8,stdin); fwrite(buf,1,8,stdout);
	fread(buf,1,8,stdin); fwrite(buf,1,8,stdout);
	while(fread(buf,1,8,stdin)>0) {
		if((buf[6]<<8)+buf[7]<=cutoff) memset(buf,0,8);
		fwrite(buf,1,8,stdout);
	}
	return 0;
}
