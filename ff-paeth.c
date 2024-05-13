#if 0
gcc -s -O2 -o ~/bin/ff-paeth -Wno-unused-result ff-paeth.c
exit
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned char buf[16];
static int width;
static int pos;
static unsigned char*row;
static int mode;

static void
usage(void)
{
	fprintf(stderr,
		"Usage: ff-paeth <mode>                                                        \n"
		"                                                                              \n"
		"This farbfeld filter perform Paeth prediction.                                \n"
		"                                                                              \n"
		"Paeth prediction is a technique for making image files smaller which can      \n"
		"improve compression.                                                          \n"
		"                                                                              \n"
		"The filter takes exactly one argument, which is a single letter mode.         \n"
		"                                                                              \n"
		"The available modes are:                                                      \n"
		"   d -- decode                                                                \n"
		"   e -- encode                                                                \n"
		"                                                                              \n"
		"Example usage:                                                                \n"
		"   $ ff-paeth e < image.ff > image-paeth.ff                                   \n"
		"   $ ff-paeth e < image.ff | ff-paeth d > same-image.ff                       \n"
		"\n"
	);
	exit(1);
}

static int paeth(int a,int b,int c) {
	int p=a+b-c;
	int pa=p>a?p-a:a-p;
	int pb=p>b?p-b:b-p;
	int pc=p>c?p-c:c-p;
	if(pa<=pb && pa<=pc) return a;
	if(pb<=pc) return b;
	return c;
}

static void encode(int idx) {
	int x=(buf[idx]-paeth(buf[idx+8],row[idx+(pos<<3)],pos?row[idx+(pos<<3)-8]:0))&255;
	putchar(x);
	row[idx+(pos<<3)]=buf[idx];
}

static void decode(int idx) {
	int x=(buf[idx]+paeth(buf[idx+8],row[idx+(pos<<3)],pos?row[idx+(pos<<3)-8]:0))&255;
	putchar(x);
	row[idx+(pos<<3)]=buf[idx]=x;
}

int main(int argc,char**argv) {
	int i;
	if (argc>1 && (!strcmp(argv[1],"-h") || !strcmp(argv[1],"--help"))) {
		usage();
	}
	if(argc<2 || (argv[1][0]!='e' && argv[1][0]!='d')) {
		fprintf(stderr,"Bad option\n");
		return 1;
	}
	mode=(argv[1][0]=='d');
	fread(buf,1,16,stdin);
	fwrite(buf,1,16,stdout);
	width=(buf[8]<<24)|(buf[9]<<16)|(buf[10]<<8)|buf[11];
	row=calloc(width,8);
	if(!row) {
		fprintf(stderr,"Out of memory\n");
		return 1;
	}
	memset(buf+8,0,8);
	while(fread(buf,8,1,stdin)) {
		for(i=0;i<8;i++) if(mode) decode(i); else encode(i);
		if(pos=(pos+1)%width) memcpy(buf+8,buf,8); else memset(buf+8,0,8);
	}
	return 0;
}
