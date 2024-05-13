#if 0
gcc -s -O2 -o ~/bin/grob2ff -Wno-unused-result grob2ff.c
exit
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned char header[18];
static int width,height;
static const unsigned char pixels[16]={255,255,255,255,255,255,255,255,0,0,0,0,0,0,255,255};

static void
usage(void)
{
	fprintf(stderr,
		"Usage: grob2ff                                                                \n"
		"                                                                              \n"
		"Converts HP-48/HP-49 graphics object to farbfeld. grob2ff reads a graphics    \n"
		"object from stdin, converts it to farbfeld and writes the result to stdout.   \n"
		"                                                                              \n"
		"HP-48 and HP-49 are programmable graphing calculators designed and produced by\n"
		"Hewlett-Packard.                                                              \n"
		"                                                                              \n"
		"This decoder can read both ASCII and binary files, although some files may not\n"
		"work.                                                                         \n"
		"                                                                              \n"
		"This decoder takes no arguments.                                              \n"
		"                                                                              \n"
		"Example usage:                                                                \n"
		"   $ grob2ff < image.grob > image.ff                                          \n"
		"\n"
	);
	exit(1);
}

static inline void write_header(void) {
	fwrite("farbfeld",1,8,stdout);
	putchar(width>>24); putchar(width>>16); putchar(width>>8); putchar(width);
	putchar(height>>24); putchar(height>>16); putchar(height>>8); putchar(height);
}

static inline void grob_ascii(int h) {
	int c,x,y;
	if(h) while((c=fgetc(stdin))!=EOF && c!='\n' && c!='\r');
	c=fgetc(stdin);
	if(c!='\n') ungetc(c,stdin);
	if(scanf(h?"GROB %d %d%*1[\r\n ]":"%d %d%*1[\r\n ]",&width,&height)<2) {
		fprintf(stderr,"Unknown file format\n");
		exit(1);
	}
	write_header();
	for(y=0;y<height;y++) {
		for(x=0;x<width;x++) {
			if(!(x&3)) {
				c=fgetc(stdin);
				if(c=='\r' || c=='\n') c=fgetc(stdin);
				if(c>='A') c+=10-'A'; else c-='0';
			}
			fwrite(pixels+(c&1?8:0),1,8,stdout);
			c>>=1;
		}
		if("\0!!!!\0\0\0"[width&7]) fgetc(stdin);
	}
}

static inline void grob_binary(void) {
	int c,x,y;
	fread(header+5,1,13,stdin);
	if((header[5]!='8' && header[5]!='9') || header[8]!=0x1E || header[9]!=0x2B || (header[10]&15)) {
		fprintf(stderr,"Unknown file format\n");
		exit(1);
	}
	// Header value interleaving is: 1E 2B z0 zz zz yy yy xy xx xx
	width=(header[17]<<12)|(header[16]<<4)|(header[15]>>4);
	height=((header[15]&15)<<16)|(header[14]<<8)|header[13];
	write_header();
	for(y=0;y<height;y++) {
		for(x=0;x<width;x++) {
			if(!(x&7)) c=fgetc(stdin);
			fwrite(pixels+(c&1?8:0),1,8,stdout);
			c>>=1;
		}
	}
}

int main(int argc,char**argv) {
	if (argc > 1) {
		usage();
	}
	fread(header,1,5,stdin);
	if(!memcmp(header,"%%HP:",5)) {
		grob_ascii(1);
	} else if(!memcmp(header,"GROB ",5)) {
		grob_ascii(0);
	} else if(!memcmp(header,"HPHP4",5)) {
		grob_binary();
	} else {
		fprintf(stderr,"Unknown file format\n");
		return 1;
	}
	return 0;
}
