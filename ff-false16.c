#if 0
gcc -s -O2 -o ~/bin/ff-false16 -Wno-unused-result ff-false16.c
exit
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
	unsigned short r;
	unsigned short g;
	unsigned short b;
	unsigned short a;
} Color;

static Color shade[0x10000];

static void
usage(void)
{
	fprintf(stderr,
		"Usage: ff-false16 <0> <colour> <#> <colour> ... <65535>                       \n"
		"                                                                              \n"
		"This farbfeld filter is used to change colours in the picture to a specified  \n"
		"gradient.                                                                     \n"
		"                                                                              \n"
		"This is the 16-bit version of the false filter, for 8-bit gradients refer to  \n"
		"ff-false.                                                                     \n"
		"                                                                              \n"
		"The filter takes at a minimum five arguments.                                 \n"
		"                                                                              \n"
		"Specify a gradient by a sequence alternating 16-bit numbers and colours, with \n"
		"a 16-bit number as the first and last argument; these should be 0 and 65535,  \n"
		"although it is not required. It represents a gradient with several components,\n"
		"and interpolated in between. Changes colours of the picture to the            \n"
		"corresponding value of the channel of the gradient.                           \n"
		"                                                                              \n"
		"The colour can be defined through one of the following patterns:              \n"
		"   hh                one pair of 8-bit hex value (applies to all channels)    \n"
		"   hhhh              one pair of 16-bit hex value (applies to all channels)   \n"
		"   rrggbb            three pairs of 8-bit hex values                          \n"
		"   rrggbbaa          four pairs of 8-bit hex values                           \n"
		"   rrrrggggbbbb      three pairs of 16-bit hex values                         \n"
		"   rrrrggggbbbbaaaa  four pairs of 16-bit hex values                          \n"
		"                                                                              \n"
		"Example gradient:                                                             \n"
		"   0 000000 20000 FF0000 40000 00FF00 60000 0000FF 65535                      \n"
		"                                                                              \n"
		"Example usage:                                                                \n"
		"   $ ff-false16 0 000000 20000 0000FF 65535 < image.ff > false16.ff           \n"
		"\n"
	);
	exit(1);
}

static Color parse_color(const char*s) {
	Color c;
	switch(strlen(s)) {
		case 2:
			sscanf(s,"%2hx",&c.r);
			c.r*=0x101;
			c.a=c.g=c.b=c.r;
			break;
		case 4:
			sscanf(s,"%4hx",&c.r);
			c.a=c.g=c.b=c.r;
			break;
		case 6:
			sscanf(s,"%2hx%2hx%2hx",&c.r,&c.g,&c.b);
			c.r*=0x101;
			c.g*=0x101;
			c.b*=0x101;
			c.a=0xFFFF;
			break;
		case 8:
			sscanf(s,"%2hx%2hx%2hx%2hx",&c.r,&c.g,&c.b,&c.a);
			c.r*=0x101;
			c.g*=0x101;
			c.b*=0x101;
			c.a*=0x101;
			break;
		case 12:
			sscanf(s,"%4hx%4hx%4hx",&c.r,&c.g,&c.b);
			c.a=0xFFFF;
			break;
		case 16:
			sscanf(s,"%4hx%4hx%4hx%4hx",&c.r,&c.g,&c.b,&c.a);
			break;
	}
	return c;
}

static void calc_gradient(Color s1,Color s2,int i1,int i2) {
	Color c;
	int i;
	double g;
	for(i=i1;i<=i2;i++) {
		g=(i-i1)/(double)(i2-i1);
		if(s1.r==s2.r) c.r=s1.r; else c.r=g*s1.r+(1.0-g)*s2.r;
		if(s1.g==s2.g) c.g=s1.g; else c.g=g*s1.g+(1.0-g)*s2.g;
		if(s1.b==s2.b) c.b=s1.b; else c.b=g*s1.b+(1.0-g)*s2.b;
		if(s1.a==s2.a) c.a=s1.a; else c.a=g*s1.a+(1.0-g)*s2.a;
		if(c.r>shade[i].r) shade[i].r=c.r;
		if(c.g>shade[i].g) shade[i].g=c.g;
		if(c.b>shade[i].b) shade[i].b=c.b;
		if(c.a>shade[i].a) shade[i].a=c.a;
	}
}

int main(int argc,char**argv) {
	int i;
	if (argc>1 && (!strcmp(argv[1],"-h") || !strcmp(argv[1],"--help"))) {
		usage();
	}
	if(argc<5 || !argc%2) {
		fprintf(stderr,"Improper number of arguments\n");
		return 1;
	}
	for(i=1;i<argc-3;i+=2) calc_gradient(parse_color(argv[i+3]),parse_color(argv[i+1]),strtol(argv[i+0],0,0),strtol(argv[i+2],0,0));
	for(i=0;i<16;i++) putchar(fgetc(stdin));
	for(;;) {
		i=fgetc(stdin);
		if(i==EOF) return 0;
		i=(i<<8)|fgetc(stdin);
		putchar(shade[i].r>>8);
		putchar(shade[i].r);
		i=fgetc(stdin);
		if(i==EOF) return 0;
		i=(i<<8)|fgetc(stdin);
		putchar(shade[i].g>>8);
		putchar(shade[i].g);
		i=fgetc(stdin);
		if(i==EOF) return 0;
		i=(i<<8)|fgetc(stdin);
		putchar(shade[i].b>>8);
		putchar(shade[i].b);
		i=fgetc(stdin);
		if(i==EOF) return 0;
		i=(i<<8)|fgetc(stdin);
		putchar(shade[i].a>>8);
		putchar(shade[i].a);
	}
}
