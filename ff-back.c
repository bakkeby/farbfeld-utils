#if 0
gcc -s -O2 -o ~/bin/ff-back ff-back.c
exit
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned char buf[16];
static double alpha;
static int param[3];

static void
usage(void)
{
	fprintf(stderr,
		"Usage: ff-back <colour>                                                       \n"
		"                                                                              \n"
		"This farbfeld filter applies a background colour to a picture with transparent\n"
		"parts.                                                                        \n"
		"                                                                              \n"
		"It takes one argument, which is a colour without alpha. The picture is then   \n"
		"composed onto the background of that colour, making it fully opaque.          \n"
		"                                                                              \n"
		"The colour can be defined through one of the following patterns:              \n"
		"   r g b             three 16-bit integer values (0 to 65535)                 \n"
		"   rrggbb            three pairs of 8-bit hex values                          \n"
		"   rrrrggggbbbb      three pairs of 16-bit hex values                         \n"
		"                                                                              \n"
		"Example usage:                                                                \n"
		"   $ ff-back FF9090 < image.ff > image-back.ff                                \n"
		"   $ ff-back FF0090009000 < image.ff > image-back.ff                          \n"
		"   $ ff-back 65280 36864 36864 < image.ff > image-back.ff                     \n"
		"\n"
	);
	exit(1);
}

static void process(int pos) {
	int v=(buf[pos]<<8)|buf[pos+1];
	v=alpha*v+(1.0-alpha)*param[pos>>1];
	buf[pos]=v>>8;
	buf[pos+1]=v;
}

int main(int argc,char**argv) {
	int i;
	if (argc>1 && (!strcmp(argv[1],"-h") || !strcmp(argv[1],"--help"))) {
		usage();
	}
	i=fread(buf,1,16,stdin);
	i=fwrite(buf,1,16,stdout);
	if(argc==4) {
		param[0]=strtol(argv[1],0,0);
		param[1]=strtol(argv[2],0,0);
		param[2]=strtol(argv[3],0,0);
	} else if(argc==2 && strlen(argv[1])==6) {
		sscanf(argv[1],"%2X%2X%2X",param+0,param+1,param+2);
	} else if(argc==2 && strlen(argv[1])==12) {
		sscanf(argv[1],"%4X%4X%4X",param+0,param+1,param+2);
	} else {
		fprintf(stderr,"Incorrect number of arguments\n");
		return 1;
	}

	if (param[0] <= 0xFF && param[1] <= 0xFF && param[2] <= 0xFF) {
		param[0]*=0x101;
		param[1]*=0x101;
		param[2]*=0x101;
	}

	while(fread(buf,1,8,stdin)) {
		alpha=((double)((buf[6]<<8)|buf[7]))/65535.0;
		process(0);
		process(2);
		process(4);
		buf[6]=buf[7]=255;
		i=fwrite(buf,1,8,stdout);
	}
	return 0;
}

