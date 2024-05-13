#if 0
gcc -s -O2 -o ~/bin/ff-apclut -Wno-unused-result ff-apclut.c
exit
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned char buf[16];
static unsigned char clut[134217728];

static void
usage(void)
{
	fprintf(stderr,
		"Usage: ff-apclut <clut picture>                                               \n"
		"                                                                              \n"
		"This farbfeld filter applies a Hald CLUT picture to the input farbfeld image. \n"
		"                                                                              \n"
		"A Hald CLUT is a special image that encodes how source colors should be mapped\n"
		"to output colors.                                                             \n"
		"                                                                              \n"
		"The only argument is a Hald CLUT picture with a precision of 8-bits per       \n"
		"channel (although the pixels themselves are 16-bits per channel), and will    \n"
		"then apply it to the input picture. Use ff-idclut to generate an identity Hald\n"
		"CLUT.                                                                         \n"
		"                                                                              \n"
		"The alpha of the input picture is ignored; the alpha of the CLUT is used for  \n"
		"output.                                                                       \n"
		"                                                                              \n"
		"Example usage:                                                                \n"
		"   $ ff-apclut clut.ff < image.ff > image-adjusted.ff                         \n"
		"\n"
	);
	exit(1);
}

int main(int argc,char**argv) {
	FILE*fp;
	long k;
	if (argc<2 || !strcmp(argv[1],"-h") || !strcmp(argv[1],"--help")) {
		usage();
	}
	fp=fopen(argv[1],"r");
	if(!fp) {
		perror(argv[1]);
		return 1;
	}
	fread(buf,1,16,fp);
	fread(clut,1,134217728,fp);
	fread(buf,1,16,stdin);
	fwrite(buf,1,16,stdout);
	while(fread(buf,1,8,stdin)>0) {
		k=(((long)buf[0])<<3)|(((long)buf[2])<<11)|(((long)buf[4])<<19);
		fwrite(clut+k,1,8,stdout);
	}
	return 0;
}

