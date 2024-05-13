#if 0
gcc -s -O2 -o ~/bin/ff-poster ff-poster.c
exit
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned char buf[16];
static char chan;
static int param;

static void
usage(void)
{
	fprintf(stderr,
		"Usage: ff-poster <?num bits to retain?> <?channels to affect?>                \n"
		"                                                                              \n"
		"This farbfeld filter is used to reduce bit depth of the picture.              \n"
		"                                                                              \n"
		"The filter takes two arguments.                                               \n"
		"                                                                              \n"
		"The first argument is a number from 1 to 16 indicating how many significant   \n"
		"bits to retain (specifying 16 has no effect). The remaining bits are copied   \n"
		"from the significant bits. The default is to retain 1 bit.                    \n"
		"                                                                              \n"
		"The second argument is optional, and consists of one or more of letters rgba  \n"
		"which indicates which channels to affect; the default is affecting all        \n"
		"channels.                                                                     \n"
		"                                                                              \n"
		"Example usage:                                                                \n"
		"   $ ff-poster < image.ff > image-poster.ff                                   \n"
		"   $ ff-poster 3 < image.ff > image-poster.ff                                 \n"
		"   $ ff-poster 1 rb < image.ff > image-poster.ff                              \n"
		"\n"
	);
	exit(1);
}

static void process(int pos) {
	int v=(buf[pos]<<8)|buf[pos+1];
	switch(param) {
		case 1: v=v&0x8000?0xFFFF:0x0000; break;
		case 2: v&=0xC000,v|=v>>8,v|=v>>4,v|=v>>2; break;
		case 3: v&=0xE000,v|=v>>12,v|=v>>6,v|=v>>3; break;
		case 4: v&=0xF000,v|=v>>8,v|=v>>4; break;
		case 5: v&=0xF800,v|=v>>10,v|=v>>5; break;
		case 6: v&=0xFC00,v|=v>>12,v|=v>>6; break;
		case 7: v&=0xFE00,v|=v>>14,v|=v>>7; break;
		case 8: v&=0xFF00,v|=v>>8; break;
		case 9: v&=0xFF80,v|=v>>9; break;
		case 10: v&=0xFFC0,v|=v>>10; break;
		case 11: v&=0xFFE0,v|=v>>11; break;
		case 12: v&=0xFFF0,v|=v>>12; break;
		case 13: v&=0xFFF8,v|=v>>13; break;
		case 14: v&=0xFFFC,v|=v>>14; break;
		case 15: v&=0xFFFE,v|=v>>15; break;
		case 16: v=v; break;
	}
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
	if(argc>2) {
		chan=0;
		for(i=0;argv[2][i];i++) {
			if(argv[2][i]=='r') chan|=1;
			if(argv[2][i]=='g') chan|=2;
			if(argv[2][i]=='b') chan|=4;
			if(argv[2][i]=='a') chan|=8;
		}
	} else {
		chan=15;
	}
	param=argc>1?strtol(argv[1],0,0):1;
	if(param<0 || param>16) return 1;
	while(fread(buf,1,8,stdin)) {
		if(chan&1) process(0);
		if(chan&2) process(2);
		if(chan&4) process(4);
		if(chan&8) process(6);
		i=fwrite(buf,1,8,stdout);
	}
	return 0;
}

