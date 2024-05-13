#if 0
gcc -s -O2 -o ~/bin/ff-colorkey ff-colorkey.c
exit
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned char buf[16];
static char chan;
static int param[3];
static unsigned char key[6];

static void
usage(void)
{
	fprintf(stderr,
		"Usage: ff-colorkey <colour>                                                   \n"
		"                                                                              \n"
		"This farbfeld filter makes pixels of a specific colour transparent.           \n"
		"                                                                              \n"
		"The colour can be defined through one of the following patterns:              \n"
		"   r g b             three 16-bit integer values (0 to 65535)                 \n"
		"   rrggbb            three pairs of 8-bit hex values                          \n"
		"   rrrrggggbbbb      three pairs of 16-bit hex values                         \n"
		"                                                                              \n"
		"Example usage:                                                                \n"
		"   $ ff-colorkey 65535 2570 43690 < image.ff > image-colorkey.ff              \n"
		"   $ ff-colorkey ff0aaa < image.ff > image-colorkey.ff                        \n"
		"   $ ff-colorkey ffff0a00aa00 < image.ff > image-colorkey.ff                  \n"
		"\n"
	);
	exit(1);
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
		param[0]*=0x101;
		param[1]*=0x101;
		param[2]*=0x101;
	} else if(argc==2 && strlen(argv[1])==12) {
		sscanf(argv[1],"%4X%4X%4X",param+0,param+1,param+2);
	} else {
		fprintf(stderr,"Incorrect number of arguments\n");
		return 1;
	}
	for(i=0;i<3;i++) key[i<<1]=param[i]>>8,key[(i<<1)+1]=param[i]&255;
	while(fread(buf,1,8,stdin)) {
		if(!memcmp(key,buf,6)) buf[6]=buf[7]=0;
		i=fwrite(buf,1,8,stdout);
	}
	return 0;
}

