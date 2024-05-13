#if 0
gcc -s -O2 -o ~/bin/ff-uniq -Wno-unused-result ff-uniq.c
exit
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned char buf[16];
static int width,totalheight,tileheight,ntiles,tilesize,otiles;
static unsigned char*pic;
static unsigned char*mark;

static void
usage(void)
{
	fprintf(stderr,
		"Usage: ff-uniq <height>                                                       \n"
		"                                                                              \n"
		"Delete duplicate tiles from a vertical strip.                                 \n"
		"                                                                              \n"
		"ff-uniq takes a vertical strip as input and produces as output the vertical   \n"
		"strip with all duplicate tiles removed.                                       \n"
		"If the input is not a vertical strip then use ff-strip to make it so.         \n"
		"                                                                              \n"
		"Takes exactly one argument, which is the height of one tile.                  \n"
		"                                                                              \n"
		"Example usage:                                                                \n"
		"   $ ff-uniq 14 < vertical_strip.ff > vertical_strip_unique.ff                \n"
		"   $ ff-strip 14 14 1 < tile_set.ff | ff-uniq 14 > vertical_strip_unique.ff   \n"
		"\n"
	);
	exit(1);
}

static void
process(void)
{
	int i,j;
	unsigned char*p;
	unsigned char*q;
	otiles=ntiles;
	for (i=0,p=pic; i<ntiles; i++,p+=tilesize) if (!mark[i]) {
		for (j=i+1,q=p+tilesize; j<ntiles; j++,q+=tilesize) {
			if (!memcmp(p,q,tilesize)) mark[j]=1,otiles--;
		}
	}
}

static void
output(void)
{
	int i;
	unsigned char*p;
	for (i=0,p=pic; i<ntiles; i++,p+=tilesize) if (!mark[i]) fwrite(p,1,tilesize,stdout);
}

int
main(int argc, char **argv)
{
	tileheight=strtol(argv[1],0,0);
	if (argc!=2 || !tileheight)
		usage();
	fread(buf,1,16,stdin);
	width=(buf[8]<<24)|(buf[9]<<16)|(buf[10]<<8)|buf[11];
	totalheight=(buf[12]<<24)|(buf[13]<<16)|(buf[14]<<8)|buf[15];
	if (totalheight%tileheight) {
		fprintf(stderr,"Incorrect tile height\n");
		return 1;
	}
	ntiles=totalheight/tileheight;
	tilesize=width*tileheight<<3;
	mark=malloc(ntiles);
	pic=malloc(tilesize*ntiles);
	if (!mark || !pic) {
		fprintf(stderr,"Allocation failed\n");
		return 1;
	}
	memset(mark,0,ntiles);
	fwrite(buf,1,12,stdout);
	fread(pic,tilesize,ntiles,stdin);
	process();
	totalheight=otiles*tileheight;
	putchar(totalheight>>24);
	putchar(totalheight>>16);
	putchar(totalheight>>8);
	putchar(totalheight);
	output();
	return 0;
}
