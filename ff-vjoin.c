#if 0
gcc -s -O2 -o ~/bin/ff-vjoin -Wno-unused-result ff-vjoin.c
exit
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
	int w;
	int h;
	FILE*f;
} Item;

static int count;
static Item*item;
static int width;
static int height;

static void
usage(void)
{
	fprintf(stderr,
		"Usage: ff-vjoin <image.ff> <?image.ff?> ... <?image.ff?>                      \n"
		"                                                                              \n"
		"This farbfeld program joins multiple pictures together vertically.            \n"
		"                                                                              \n"
		"If the pictures do not all have the same width, the resulting width will be   \n"
		"the largest input width and the pictures will be aligned at the centre.       \n"
		"                                                                              \n"
		"The arguments are the pictures to be joined; the first picture is at the top. \n"
		"                                                                              \n"
		"This program does not read from standard in.                                  \n"
		"                                                                              \n"
		"Example usage:                                                                \n"
		"   $ ff-vjoin < image1.ff image2.ff image3.ff > images-joined-vertically.ff   \n"
		"\n"
	);
	exit(1);
}

static inline int get32(FILE*fp) {
	int x=fgetc(fp)<<24;
	x|=fgetc(fp)<<16;
	x|=fgetc(fp)<<8;
	return x|fgetc(fp);
}

static inline void put32(int x) {
	putchar(x>>24);
	putchar(x>>16);
	putchar(x>>8);
	putchar(x);
}

static inline void load_headers(char**name) {
	int i,j;
	for(i=0;i<count;i++) {
		item[i].f=fopen(name[i],"rb");
		if(!item[i].f) {
			perror(name[i]);
			exit(1);
		}
		for(j=0;j<8;j++) fgetc(item[i].f);
		item[i].w=get32(item[i].f);
		height+=item[i].h=get32(item[i].f);
		if(width<item[i].w) width=item[i].w;
	}
}

static inline void copy_pictures(void) {
	int i,j,k;
	for(i=0;i<count;i++) {
		for(j=0;j<item[i].h;j++) {
			k=((width-item[i].w)&~1)<<2;
			while(k--) putchar(0);
			k=item[i].w<<3;
			while(k--) putchar(fgetc(item[i].f));
			k=((width+1-item[i].w)&~1)<<2;
			while(k--) putchar(0);
		}
	}
}

int main(int argc,char**argv) {
	if(argc<2 || !strcmp(argv[1],"-h") || !strcmp(argv[1],"--help")) {
		usage();
	}
	item=calloc(sizeof(Item),count=argc-1);
	if(!item) {
		fprintf(stderr,"Allocation failed\n");
		return 1;
	}
	load_headers(argv+1);
	fwrite("farbfeld",1,8,stdout);
	put32(width);
	put32(height);
	copy_pictures();
	return 0;
}
