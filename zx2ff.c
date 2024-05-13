#if 0
gcc -s -O2 -o ~/bin/zx2ff -Wno-unused-result zx2ff.c
exit
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned char mem[0x3000];
static int mode;

static void
usage(void)
{
	fprintf(stderr,
		"Usage: zx2ff                                                                  \n"
		"                                                                              \n"
		"Converts ZX Spectrum graphics to farbfeld. zx2ff reads ZX Spectrum graphics   \n"
		"from stdin, converts that to farbfeld and writes the result to stdout.        \n"
		"                                                                              \n"
		"The original ZX Spectrum computer produces a one bit per pixel, bitmapped     \n"
		"colour graphics video output. A composite video signal is generated through an\n"
		"RF modulator, and was designed for use with contemporary 1980s television     \n"
		"sets.                                                                         \n"
		"                                                                              \n"
		"The image size of the framebuffer is 256x192 pixels, with a palette of 15     \n"
		"non-modifiable colours, where the entire colour palette is extremely          \n"
		"saturated. The resolution of the colour output is 64 times lower than the     \n"
		"resolution of the pixel bitmap. The extremely low colour resolution was used  \n"
		"to conserve memory, totaling just 768 bytes for colour attributes. Colour is  \n"
		"stored separate from the pixel bitmap, as a 32x24 cell grid, using one byte   \n"
		"per each of the character cells. One character cell is composed of 8x8 pixels.\n"
		"In practice, this means any character cell can only use two selected colours  \n"
		"for colouring the contained 64 pixels.                                        \n"
		"                                                                              \n"
		"This decoder takes no arguments.                                              \n"
		"                                                                              \n"
		"zx2ff can decode three ZX Spectrum graphics modes and will auto-detect which  \n"
		"mode is used. It will write an error if the detected graphics mode is not     \n"
		"supported.                                                                    \n"
		"                                                                              \n"
		"Example usage:                                                                \n"
		"   $ zx2ff < zx_spectrum.source > image.ff                                    \n"
		"\n"
	);
	exit(1);
}

static inline void
outpix(int a, int p)
{
	int i;
	int b=a&0x40?0xFF:0xD7;
	if (!p) a>>=3;
	putchar(i=a&2?b:0); putchar(i);
	putchar(i=a&4?b:0); putchar(i);
	putchar(i=a&1?b:0); putchar(i);
	putchar(255); putchar(255);
}

static void
out_picture_0(unsigned char *pix, unsigned char *att)
{
	int x,y,z;
	for (y=0; y<8; y++) {
		for (z=0; z<8; z++) {
			for (x=0; x<32; x++) {
				outpix(att[x],pix[x+(z<<8)]&0x80);
				outpix(att[x],pix[x+(z<<8)]&0x40);
				outpix(att[x],pix[x+(z<<8)]&0x20);
				outpix(att[x],pix[x+(z<<8)]&0x10);
				outpix(att[x],pix[x+(z<<8)]&0x08);
				outpix(att[x],pix[x+(z<<8)]&0x04);
				outpix(att[x],pix[x+(z<<8)]&0x02);
				outpix(att[x],pix[x+(z<<8)]&0x01);
			}
		}
		pix+=0x20;
		att+=0x20;
	}
}

static void
out_picture_2(unsigned char *pix)
{
	int x,y,z;
	for (y=0; y<8; y++) {
		for (z=0; z<8; z++) {
			for (x=0; x<32; x++) {
				outpix(pix[x+(z<<8)+0x1800],pix[x+(z<<8)]&0x80);
				outpix(pix[x+(z<<8)+0x1800],pix[x+(z<<8)]&0x40);
				outpix(pix[x+(z<<8)+0x1800],pix[x+(z<<8)]&0x20);
				outpix(pix[x+(z<<8)+0x1800],pix[x+(z<<8)]&0x10);
				outpix(pix[x+(z<<8)+0x1800],pix[x+(z<<8)]&0x08);
				outpix(pix[x+(z<<8)+0x1800],pix[x+(z<<8)]&0x04);
				outpix(pix[x+(z<<8)+0x1800],pix[x+(z<<8)]&0x02);
				outpix(pix[x+(z<<8)+0x1800],pix[x+(z<<8)]&0x01);
			}
		}
		pix+=0x20;
	}
}

static void
out_picture_6(unsigned char *pix)
{
	int x,y,z;
	for (y=0; y<8; y++) {
		for (z=0; z<8; z++) {
			for (x=0; x<32; x++) {
				outpix(mode,pix[x+(z<<8)]&0x80);
				outpix(mode,pix[x+(z<<8)]&0x40);
				outpix(mode,pix[x+(z<<8)]&0x20);
				outpix(mode,pix[x+(z<<8)]&0x10);
				outpix(mode,pix[x+(z<<8)]&0x08);
				outpix(mode,pix[x+(z<<8)]&0x04);
				outpix(mode,pix[x+(z<<8)]&0x02);
				outpix(mode,pix[x+(z<<8)]&0x01);
				outpix(mode,pix[x+(z<<8)+0x1800]&0x80);
				outpix(mode,pix[x+(z<<8)+0x1800]&0x40);
				outpix(mode,pix[x+(z<<8)+0x1800]&0x20);
				outpix(mode,pix[x+(z<<8)+0x1800]&0x10);
				outpix(mode,pix[x+(z<<8)+0x1800]&0x08);
				outpix(mode,pix[x+(z<<8)+0x1800]&0x04);
				outpix(mode,pix[x+(z<<8)+0x1800]&0x02);
				outpix(mode,pix[x+(z<<8)+0x1800]&0x01);
			}
		}
		pix+=0x20;
	}
}

int
main(int argc, char **argv)
{
	if (argc > 1) {
		usage();
	}
	fread(mem,1,0x1B00,stdin);
	if (fread(mem+0x1B00,1,0x1500,stdin)>0) {
		mode=fgetc(stdin);
		if(mode==EOF) mode=2;
	} else {
		mode=0;
	}
	switch(mode&7) {
		case 0:
			fwrite("farbfeld\0\0\x01\x00\0\0\x00\xC0",1,16,stdout);
			out_picture_0(mem+0x0000,mem+0x1800);
			out_picture_0(mem+0x0800,mem+0x1900);
			out_picture_0(mem+0x1000,mem+0x1A00);
			return 0;
		case 2:
			fwrite("farbfeld\0\0\x01\x00\0\0\x00\xC0",1,16,stdout);
			out_picture_2(mem+0x0000);
			out_picture_2(mem+0x0800);
			out_picture_2(mem+0x1000);
			return 0;
		case 6:
			fwrite("farbfeld\0\0\x02\x00\0\0\x00\xC0",1,16,stdout);
			mode=(((mode>>3)&7)*9)^0170;
			out_picture_6(mem+0x0000);
			out_picture_6(mem+0x0800);
			out_picture_6(mem+0x1000);
			return 0;
		default:
			fprintf(stderr,"Currently not implemented\n");
			return 1;
	}
}
