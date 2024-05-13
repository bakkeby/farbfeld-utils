#if 0
gcc -s -O2 -o ~/bin/ff2pbm -Wno-unused-result ff2pbm.c
exit
#endif
/*
	Convert farbfeld to PBM/PGM/PPM
	This and all other programs at http://zzo38computer.org/prog/farbfeld.zip are public domain, except LodePNG.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char format='4';
static int depth=16;
static unsigned char buf[8];
static int width;
static int pos=0;

static void
usage(void)
{
	fprintf(stderr,
		"Usage: ff2pbm <format> <?depth?>                                              \n"
		"                                                                              \n"
		"Convert farbfeld image to a Netpbm formatted image. ff2pbm reads a farbfeld   \n"
		"image from stdin, converts it to PBM format and writes the result to stdout.  \n"
		"                                                                              \n"
		"The Netpbm formats are a family of simple uncompressed raster image file      \n"
		"formats, associated with the Netpbm library and suite of image processing     \n"
		"utilities.                                                                    \n"
		"                                                                              \n"
		"There are five Netpbm formats:                                                \n"
		"   - PBM (portable bitmap) - a bi-level image format                          \n"
		"   - PGM (portable graymap) - a grayscale image format                        \n"
		"   - PPM (portable pixmap) - a colour image format                            \n"
		"   - PNM (portable anymap) - a collective name for PBM, PGM, and PPM          \n"
		"     A .pnm file may use any of these formats.                                \n"
		"   - PAM (portable array map) - a colour image format with transparency (RGBA)\n"
		"                                                                              \n"
		"This encoder takes one or two arguments.                                      \n"
		"                                                                              \n"
		"First argument is the format, from 1 to 7:                                    \n"
		"   1 = Mono text                                                              \n"
		"   2 = Greyscale text                                                         \n"
		"   3 = RGB text                                                               \n"
		"   4 = Mono binary                                                            \n"
		"   5 = Greyscale binary                                                       \n"
		"   6 = RGB binary                                                             \n"
		"   7 = Portable Arbitrary Map (RGBA binary)                                   \n"
		"                                                                              \n"
		"The second argument is the number of bits per channel (depth), from 1 to 16   \n"
		"(the default is 16).                                                          \n"
		"                                                                              \n"
		"Example usage:                                                                \n"
		"   $ ff2pbm 2 < image.ff > image.pgm                                          \n"
		"   $ ff2pbm 6 < image.ff > image.ppm                                          \n"
		"   $ bunzip2 < image.ff.bz2 | ff2pbm 4 > image.pbm                            \n"
		"\n"
	);
	exit(1);
}

int
main(int argc, char **argv)
{
	int i;
	if (argc > 1 && (!strcmp(argv[1],"-h") || !strcmp(argv[1],"--help"))) {
		usage();
	}
	if (argc>1) format=argv[1][0];
	if (argc>2) depth=strtol(argv[2],0,0);
	fread(buf,1,8,stdin);
	if (memcmp("farbfeld",buf,8)) {
		fprintf(stderr,"Not farbfeld\n");
		return 1;
	}
	if (depth<1 || depth>16 || format<'1' || format>'7') {
		fprintf(stderr,"Invalid output format (%c,%d)\n",format,depth);
		return 1;
	}
	putchar('P');
	putchar(format);
	if (format!='7') {
		i=fgetc(stdin)<<24;
		i|=fgetc(stdin)<<16;
		i|=fgetc(stdin)<<8;
		i|=fgetc(stdin);
		printf("\n%d",width=i);
		i=fgetc(stdin)<<24;
		i|=fgetc(stdin)<<16;
		i|=fgetc(stdin)<<8;
		i|=fgetc(stdin);
		printf("\n%d\n",i);
		if (format!='1' && format!='4') printf("%d\n",(1<<depth)-1);
	}
	--depth;
	switch(format) {
		case '1':
			while (fread(buf,8,1,stdin)) {
				putchar('1'^(*buf>>7));
				putchar('\n');
			}
			break;
		case '2':
			// This program outputs linear PGM/PPM.
			// To gamma correct it, use pnmgamma or ff-bright
			while (fread(buf,8,1,stdin)) {
				printf("%d\n",((buf[0]<<8)|buf[1])>>(15-depth));
			}
			break;
		case '3':
			while (fread(buf,8,1,stdin)) {
				printf("%d %d %d\n",((buf[0]<<8)|buf[1])>>(15-depth),((buf[2]<<8)|buf[3])>>(15-depth),((buf[4]<<8)|buf[5])>>(15-depth));
			}
			break;
		case '4':
			while (fread(buf,8,1,stdin)) {
				if (!(pos&7)) i=255;
				i^=(*buf>>7)<<(7&~pos);
				if ((pos&7)==7 || pos==width-1) putchar(i);
				pos=(pos+1)%width;
			}
			break;
		case '5':
			while (fread(buf,8,1,stdin)) {
				putchar(*buf>>(7&~depth));
				if (depth&8) putchar(((buf[0]<<8)|buf[1])>>(15-depth));
			}
			break;
		case '6':
			while (fread(buf,8,1,stdin)) {
				putchar(buf[0]>>(7&~depth));
				if (depth&8) putchar(((buf[0]<<8)|buf[1])>>(15-depth));
				putchar(buf[2]>>(7&~depth));
				if (depth&8) putchar(((buf[2]<<8)|buf[3])>>(15-depth));
				putchar(buf[4]>>(7&~depth));
				if (depth&8) putchar(((buf[4]<<8)|buf[5])>>(15-depth));
			}
			break;
		case '7':
			i=fgetc(stdin)<<24;
			i|=fgetc(stdin)<<16;
			i|=fgetc(stdin)<<8;
			i|=fgetc(stdin);
			printf("\nWIDTH %d\n",i);
			i=fgetc(stdin)<<24;
			i|=fgetc(stdin)<<16;
			i|=fgetc(stdin)<<8;
			i|=fgetc(stdin);
			printf("HEIGHT %d\n",i);
			printf("MAXVAL %d\nDEPTH 4\nTUPLTYPE RGB_ALPHA\nENDHDR\n",(2<<depth)-1);
			while (fread(buf,8,1,stdin)) {
				putchar(buf[0]>>(7&~depth));
				if (depth&8) putchar(((buf[0]<<8)|buf[1])>>(15-depth));
				putchar(buf[2]>>(7&~depth));
				if (depth&8) putchar(((buf[2]<<8)|buf[3])>>(15-depth));
				putchar(buf[4]>>(7&~depth));
				if (depth&8) putchar(((buf[4]<<8)|buf[5])>>(15-depth));
				putchar(buf[6]>>(7&~depth));
				if (depth&8) putchar(((buf[6]<<8)|buf[7])>>(15-depth));
			}
			break;
	}
	return 0;
}
