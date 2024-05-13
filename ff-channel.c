#if 0
gcc -s -O2 -o ~/bin/ff-channel -Wno-unused-result ff-channel.c
exit
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned char buf[64];
static FILE*file[4];

static void
usage(void)
{
	fprintf(stderr,
		"Usage: ff-channel <file> <file> <?file?> <?file?>                             \n"
		"                                                                              \n"
		"This filter combines channels from four farbfeld pictures.                    \n"
		"                                                                              \n"
		"The filter takes four pictures as command-line arguments, all of which must be\n"
		"the same size. No data is read from stdin.                                    \n"
		"                                                                              \n"
		"The output picture takes:                                                     \n"
		"   - the red channel from the first input                                     \n"
		"   - the green channel from the second input                                  \n"
		"   - the blue channel from the third input and                                \n"
		"   - the alpha channel from the fourth input                                  \n"
		"                                                                              \n"
		"An alternative invocation is to provide only two pictures as command-line     \n"
		"arguments. In this case, the red and green and blue channels come from the    \n"
		"first picture, and the alpha channel comes from the second picture.           \n"
		"                                                                              \n"
		"Example usage:                                                                \n"
		"   $ ff-channel red.ff green.ff blue.ff alpha.ff > channels-combined.ff       \n"
		"   $ ff-channel rgb.ff alpha.ff > channels-combined.ff                        \n"
		"\n"
	);
	exit(1);
}

int main(int argc,char**argv) {
	int i;
	if (argc>1 && (!strcmp(argv[1],"-h") || !strcmp(argv[1],"--help"))) {
		usage();
	}
	if(argc<5 && argc!=3) {
		fprintf(stderr,"Incorrect number of arguments\n");
		return 1;
	}
	for(i=0;i<4;i++) {
		if(argc==3 && i==2) break;
		file[i]=fopen(argv[i+1],"r");
		if(!file[i]) {
			perror(argv[i+1]);
			return 1;
		}
	}
	fread(buf+0,1,16,file[0]);
	fread(buf+16,1,16,file[1]);
	if(argc==3) {
		memcpy(buf+32,buf,32);
	} else {
		fread(buf+32,1,16,file[2]);
		fread(buf+48,1,16,file[3]);
	}
	if(memcmp(buf,buf+16,16) || memcmp(buf,buf+32,16) || memcmp(buf,buf+48,16)) {
		fprintf(stderr,"Header mismatch\n");
		return 1;
	}
	fwrite(buf,1,16,stdout);
	if(argc==3) {
		while(fread(buf+0,1,8,file[0]) && fread(buf+8,1,8,file[1])) {
			fwrite(buf+0,1,6,stdout);
			fwrite(buf+14,1,2,stdout);
		}
	} else {
		while(fread(buf+0,1,8,file[0]) && fread(buf+16,1,8,file[1]) && fread(buf+32,1,8,file[2]) && fread(buf+48,1,8,file[3])) {
			fwrite(buf+0,1,2,stdout);
			fwrite(buf+18,1,2,stdout);
			fwrite(buf+36,1,2,stdout);
			fwrite(buf+54,1,2,stdout);
		}
	}
	return 0;
}
