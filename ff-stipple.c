#if 0
gcc -s -O2 -o ~/bin/ff-stipple ff-stipple.c
exit
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned int xsize,ysize,xtsize,ytsize,x,y,xx,yy;
static char stipple[4103];
static unsigned char color[32];

static const char base64[128]={
	['A']=0,['B']=1,['C']=2,['D']=3,['E']=4,['F']=5,['G']=6,['H']=7,['I']=8,['J']=9,['K']=10,['L']=11,['M']=12,['N']=13,
	['O']=14,['P']=15,['Q']=16,['R']=17,['S']=18,['T']=19,['U']=20,['V']=21,['W']=22,['X']=23,['Y']=24,['Z']=25,['a']=26,
	['b']=27,['c']=28,['d']=29,['e']=30,['f']=31,['g']=32,['h']=33,['i']=34,['j']=35,['k']=36,['l']=37,['m']=38,['n']=39,
	['o']=40,['p']=41,['q']=42,['r']=43,['s']=44,['t']=45,['u']=46,['v']=47,['w']=48,['x']=49,['y']=50,['z']=51,['0']=52,
	['1']=53,['2']=54,['3']=55,['4']=56,['5']=57,['6']=58,['7']=59,['8']=60,['9']=61,['+']=62,['/']=63,['-']=62,['_']=63,
};

static void
usage(void)
{
	fprintf(stderr,
		"Usage: ff-stipple <num horz> <num vert> <pattern> <bg> <fg> <?bg? <?fg?>      \n"
		"                                                                              \n"
		"This farbfeld program is used to generate a stipple pattern.                  \n"
		"                                                                              \n"
		"Makes a tiled stipple pattern with two colours. ff-stipple does not read from \n"
		"standard in.                                                                  \n"
		"                                                                              \n"
		"The filter takes five arguments:                                              \n"
		"   - how many repetitions of the pattern to occur horizontally                \n"
		"   - how many repetitions of the pattern to occur vertically                  \n"
		"   - the pattern                                                              \n"
		"   - background colour                                                        \n"
		"   - foreground colour                                                        \n"
		"   - alternating background colour (optional)                                 \n"
		"   - alternating foreground colour (optional)                                 \n"
		"                                                                              \n"
		"Optionally two more arguments can be provided, which are additional background\n"
		"and foreground colour that are used to generate a chess stipple.              \n"
		"                                                                              \n"
		"The colours can be defined through one of the following patterns:             \n"
		"   rrggbb            three pairs of 8-bit hex values                          \n"
		"   rrggbbaa          four pairs of 8-bit hex values                           \n"
		"   rrrrggggbbbb      three pairs of 16-bit hex values                         \n"
		"   rrrrggggbbbbaaaa  four pairs of 16-bit hex values                          \n"
		"                                                                              \n"
		"The pattern is encoded as base64, where the first character gives one less    \n"
		"than the pattern width, and the picture is encoded using the rest of the      \n"
		"base64 characters, each one encoding six pixels, with the high bit on the     \n"
		"left.                                                                         \n"
		"                                                                              \n"
		"Some example patterns:                                                        \n"
		"   - bats: PERAJIC1obWzsb07l3ve+++7v9t/Sl5KTE5CxGiEIQQQ                       \n"
		"   - diagonal: FBCEIQg                                                        \n"
		"   - grid: Hqv/+//7//v8                                                       \n"
		"   - tiles: PAAAAgKjqVNWo6lTVqOpU1ajqVNWo6lTVqOpU1fz//v8                      \n"
		"   - weave: FQXQC6C                                                           \n"
		"                                                                              \n"
		"Example usage:                                                                \n"
		"   $ ff-stipple 7 5 FBCEIQg 906060 409040 > stipple.ff                        \n"
		"   $ ff-stipple 7 5 FQXQC6C 906060 409040 606090 404090 > stipple.ff          \n"
		"\n"
	);
	exit(1);
}

static void parse_color(unsigned char*out,const char*in) {
	switch(strlen(in)) {
		case 6:
			sscanf(in,"%02hhX%02hhX%02hhX",out+0,out+2,out+4);
			out[1]=out[0];
			out[3]=out[2];
			out[5]=out[4];
			out[6]=out[7]=255;
			break;
		case 8:
			sscanf(in,"%02hhX%02hhX%02hhX%02hhX",out+0,out+2,out+4,out+6);
			out[1]=out[0];
			out[3]=out[2];
			out[5]=out[4];
			out[7]=out[6];
			break;
		case 12:
			sscanf(in,"%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX",out+0,out+1,out+2,out+3,out+4,out+5);
			out[6]=out[7]=255;
			break;
		case 16:
			sscanf(in,"%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX",out+0,out+1,out+2,out+3,out+4,out+5,out+6,out+7);
			break;
		default:
			fprintf(stderr,"Invalid color format (%d)\n",(int)strlen(in));
			exit(1);
	}
}

static void parse_stipple(const char*in) {
	int i,j;
	if(*in=='=') {
		xtsize=(base64[in[1]&127]&7)+1;
		ytsize=in[1]>='a'?-1:-2;
		in+=2;
	} else {
		xtsize=base64[*in++&127]+1;
		ytsize=0;
	}
	i=0;
	while(*in && *in!='=' && i<4096) {
		j=base64[*in++&127];
		stipple[i++]=j&32?8:0;
		stipple[i++]=j&16?8:0;
		stipple[i++]=j&8?8:0;
		stipple[i++]=j&4?8:0;
		stipple[i++]=j&2?8:0;
		stipple[i++]=j&1?8:0;
	}
	if(i<2) i=2;
	ytsize+=i/xtsize?:1;
	if(ytsize>64) ytsize=64;
}

int main(int argc,char**argv) {
	if (argc>1 && (!strcmp(argv[1],"-h") || !strcmp(argv[1],"--help"))) {
		usage();
	}
	if(argc!=6 && argc!=8) {
		fprintf(stderr,"Too %s arguments\n",argc<6?"few":"many");
		return 1;
	}
	xsize=strtol(argv[1],0,0);
	ysize=strtol(argv[2],0,0);
	parse_color(color,argv[4]);
	parse_color(color+8,argv[5]);
	parse_stipple(argv[3]);
	if(argc==8) {
		parse_color(color+16,argv[6]);
		parse_color(color+24,argv[7]);
	}
	fwrite("farbfeld",1,8,stdout);
	putchar((xsize*xtsize)>>24);
	putchar((xsize*xtsize)>>16);
	putchar((xsize*xtsize)>>8);
	putchar((xsize*xtsize)>>0);
	putchar((ysize*ytsize)>>24);
	putchar((ysize*ytsize)>>16);
	putchar((ysize*ytsize)>>8);
	putchar((ysize*ytsize)>>0);
	if(argc==8) {
		for(y=0;y<ysize;y++) for(yy=0;yy<ytsize;yy++) for(x=0;x<xsize;x++) for(xx=0;xx<xtsize;xx++) fwrite(color+stipple[yy*xtsize+xx]+(((x^y)&1)<<4),1,8,stdout);
	} else {
		for(y=0;y<ysize;y++) for(yy=0;yy<ytsize;yy++) for(x=0;x<xsize;x++) for(xx=0;xx<xtsize;xx++) fwrite(color+stipple[yy*xtsize+xx],1,8,stdout);
	}
	return 0;
}
