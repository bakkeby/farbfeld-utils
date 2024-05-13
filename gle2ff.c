#if 0
gcc -s -O2 -o ~/bin/gle2ff -Wno-unused-result gle2ff.c
exit
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const unsigned long palette[16] = {
	0x000000, 0x0000AA, 0x00AA00, 0x00AAAA,
	0xAA0000, 0xAA00AA, 0xAA5500, 0xAAAAAA,
	0x555555, 0x5555FF, 0x55FF55, 0x55FFFF,
	0xFF5555, 0xFF55FF, 0xFFFF55, 0xFFFFFF,
};

static unsigned char buf[16];
static unsigned char*data;

static void
usage(void)
{
	fprintf(stderr,
		"Usage: gle2ff                                                                 \n"
		"                                                                              \n"
		"Convert AMI BIOS Energy Star logo image format to farbfeld. gle2ff reads a GLE\n"
		"image from stdin, converts it to farbfeld and writes the result to stdout.    \n"
		"                                                                              \n"
		"GLE is a simple monochrome graphics format used for storing the Energy Star   \n"
		"logo in AMI BIOS images.                                                      \n"
		"                                                                              \n"
		"This decoder takes no arguments and the implementation is experimental and    \n"
		"remains untested.                                                             \n"
		"                                                                              \n"
		"Example usage:                                                                \n"
		"   $ gle2ff < image.gle > image.ff                                            \n"
		"   $ gle2ff < image.gle | bzip2 > image.ff.bz2                                \n"
		"\n"
	);
	exit(1);
}

int
main(int argc, char **argv)
{
	int x,y,z;
	char*p;

	if (argc > 1) {
		usage();
	}

	fread(buf,1,8,stdin);
	if (memcmp(buf,"GLE\1",4)) {
		fprintf(stderr,"Bad magic\n");
		return 1;
	}
	p=data=malloc(112*buf[4]*buf[5]);
	if (!data) {
		fprintf(stderr,"Allocation failed\n");
		return 1;
	}
	for (y=0; y<buf[5]; y++) {
		for (x=0; x<buf[4]; x++) {
			for (z=0; z<14; z++) p[z*buf[4]]=fgetc(stdin);
			p++;
		}
		p+=13*buf[4];
	}
	fwrite("farbfeld",1,8,stdout);
	putchar(0); putchar(0); putchar(buf[4]>>5); putchar(buf[4]<<3);
	putchar(0); putchar(0); putchar((buf[5]*14)>>8); putchar(buf[5]*14);
	x=buf[4]*buf[5]*14;
	y=buf[6];
	z=buf[7];
	buf[0]=buf[1]=palette[z]>>16;
	buf[2]=buf[3]=palette[z]>>8;
	buf[4]=buf[5]=palette[z];
	buf[7]=buf[8]=buf[14]=buf[15]=255;
	buf[8]=buf[9]=palette[y]>>16;
	buf[10]=buf[11]=palette[y]>>8;
	buf[12]=buf[13]=palette[y];
	p=data;
	while (x--) {
		fwrite(buf+(*p&128?8:0),1,8,stdout);
		fwrite(buf+(*p&64?8:0),1,8,stdout);
		fwrite(buf+(*p&32?8:0),1,8,stdout);
		fwrite(buf+(*p&16?8:0),1,8,stdout);
		fwrite(buf+(*p&8?8:0),1,8,stdout);
		fwrite(buf+(*p&4?8:0),1,8,stdout);
		fwrite(buf+(*p&2?8:0),1,8,stdout);
		fwrite(buf+(*p&1?8:0),1,8,stdout);
		p++;
	}
	return 0;
}
