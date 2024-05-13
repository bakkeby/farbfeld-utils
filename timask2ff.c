#if 0
gcc -s -O2 -o ~/bin/timask2ff -Wno-unused-result timask2ff.c
exit
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned char buf[16];
static int width,height,total,bits,bitv;
static signed char*pic;
static const char pal[16]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,255,255};

static void
usage(void)
{
	fprintf(stderr,
		"Usage: timask2ff <?width?> <?height?>                                         \n"
		"                                                                              \n"
		"Convert TeXnicard image mask to farbfeld. ff2timask reads an image mask from  \n"
		"stdin, converts it to farbfeld and writes the result to stdout.               \n"
		"                                                                              \n"
		"TeXnicard image mask format is a file format for storing compressed monochrome\n"
		"transparency masks for pictures. It uses a variant of a combination of 2D RLE \n"
		"with Elias gamma.                                                             \n"
		"                                                                              \n"
		"All output is black, but the alpha channel is set according to the input. The \n"
		"X/Y offset in the input file is ignored.                                      \n"
		"                                                                              \n"
		"timask2ff takes two optional arguments that allows for the height and width of\n"
		"the output image to be overridden. By default the height and width are taken  \n"
		"from the farbfeld image itself.                                               \n"
		"                                                                              \n"
		"Example usage:                                                                \n"
		"   $ timask2ff < image.mask > image.ff                                        \n"
		"   $ timask2ff 200 180 < image.mask | bzip2 > image.ff.bz2                    \n"
		"\n"
	);
	exit(1);
}

static inline int
get_bit(void)
{
	int i;
	if (!bits) bitv=fgetc(stdin);
	if (bitv==EOF) {
		fprintf(stderr,"Unexpected end of file\n");
		exit(1);
	}
	i=(bitv>>bits++)&1;
	if (bits==8) bits=0;
	return i;
}

static int
get_number(void)
{
	int n=0;
	int i=0;
	while (get_bit()) n|=get_bit()<<i++;
	return n|(1<<i);
}

static int
figure_box(int start)
{
	int v=get_bit();
	int x1=get_number();
	int y1=get_number();
	int x,y,z;
	for (x=0; x<x1; x++) {
		for (y=0; y<y1; y++) if (pic[start+x+y*width]==-1) break;
		if (y==y1) x1++;
	}
	for (x=0; x<x1; x++) {
		for (y=0; y<y1; y++) if (pic[z=start+x+y*width]==-1 && z<total) pic[z]=v;
	}
	return x1;
}

int
main(int argc, char **argv)
{
	int i;

	if (argc > 1 && (!strcmp(argv[1],"-h") || !strcmp(argv[1],"--help"))) {
		usage();
	}

	width=fgetc(stdin)<<8;
	width|=fgetc(stdin);
	height=fgetc(stdin)<<8;
	height|=fgetc(stdin);
	fread(buf,1,4,stdin); // Ignore X/Y offset
	if (argc==3) width=strtol(argv[1],0,10),height=strtol(argv[2],0,10);
	pic=malloc(total=width*height);
	if (!pic) {
		fprintf(stderr,"Allocation failed\n");
		return 1;
	}
	memset(pic,-1,total);
	for (i=0;;) {
		i+=figure_box(i);
		while (i<total && pic[i]!=-1) i++;
		if (i==total) break;
	}
	fwrite("farbfeld\0\0",1,10,stdout);
	putchar(width>>8); putchar(width);
	putchar(0); putchar(0); putchar(height>>8); putchar(height);
	for (i=0; i<total; i++) fwrite(pal+8*pic[i],1,8,stdout);
	return 0;
}
