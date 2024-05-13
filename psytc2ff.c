#if 0
gcc -s -O2 -o ~/bin/psytc2ff -Wno-unused-result psytc2ff.c
exit
#endif

/* "psytc" = Psycopathicteen Tile Compressor */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
	unsigned char d[8];
} Color;

typedef struct {
	char i[16];
	char h[16];
} Matrix;

static Color pal[16];
static int width,height,theight,ntile,tilesize;
static char*pic;
static int bitp,bitv;
static Matrix matrix[16];
static int textmode;

static void
usage(void)
{
	fprintf(stderr,
		"Usage: psytc2ff <?palette?>                                                   \n"
		"                                                                              \n"
		"Converts Psycopathicteen Tile Compressor graphics to farbfeld. psytc2ff reads \n"
		"a psytc file from stdin, converts it to farbfeld and writes the result to     \n"
		"stdout.                                                                       \n"
		"                                                                              \n"
		"Psycopathicteen Tile Compressor is a experimental format for compressing 4    \n"
		"bits per pixel indexed colours picture (such as using on Super Nintendo).     \n"
		"                                                                              \n"
		"The arguments are the colours to use for the palette.                         \n"
		"                                                                              \n"
		"If the palette is stored in the picture file then no arguments are allowed,   \n"
		"otherwise the palette must be specified.                                      \n"
		"                                                                              \n"
		"The palette consists of 16 colours, where each colour can have one of these   \n"
		"patterns:                                                                     \n"
		"   rrggbb            three pairs of 8-bit hex values                          \n"
		"   rrggbbaa          four pairs of 8-bit hex values                           \n"
		"   rrrrggggbbbb      three pairs of 16-bit hex values                         \n"
		"   rrrrggggbbbbaaaa  four pairs of 16-bit hex values                          \n"
		"                                                                              \n"
		"If the only argument is - then it will output text (with ANSI colours) instead\n"
		"of the picture; this text is an explanation of the compression used.          \n"
		"                                                                              \n"
		"Example usage:                                                                \n"
		"   $ psytc2ff < image.bin > image.ff                                          \n"
		"   $ psytc2ff - < image.bin > image.ff                                        \n"
		"\n"
	);
	exit(1);
}

static int getbit(void) {
	bitp&=7;
	if(!bitp) bitv=fgetc(stdin);
	return (bitv>>bitp++)&1;
}

static void load_adjacency_matrix(void) {
	int i,j,k;
	for(i=0;i<16;i++) {
		matrix[i].h[i]=16;
		for(j=1;j<6;j++) {
			k=getbit(); k|=getbit()<<1; k|=getbit()<<2; k|=getbit()<<3;
			matrix[i].i[j]=k;
			matrix[i].h[k]=j;
		}
		for(j=6,k=15;j<16;j++) {
			while(matrix[i].h[k]) k--;
			matrix[i].i[j]=k--;
		}
	}
}

static int read_length(void) {
	int i=getbit();
	int n=2;
	if(!i) return 0;
	i=getbit();
	if(!i) return 1;
	for(;;) {
		i=getbit(); i|=getbit()<<1; i|=getbit()<<2; i|=getbit()<<3;
		n+=i;
		if(i!=15) return n;
	}
}

static int read_color(int v) {
	int n=getbit();
	if(n) {
		n=getbit()<<3; n|=getbit()<<2; n|=getbit()<<1;
		if(n<6) n=(n>>1)+3; else n|=getbit();
		if(textmode) printf(" \e[1;3%cm%d",n<6?'3':'1',matrix[v].i[n]);
	} else {
		n=getbit()+1;
		if(textmode) printf(" \e[1;32m%d",matrix[v].i[n]);
	}
	return matrix[v].i[n];
}

static void input_compressed_picture(char*p,int v) {
	int i,n,x,z;
	z=getbit(); z|=getbit()<<1; z|=getbit()<<2; z|=getbit()<<3;
	*p=z;
	n=read_length();
	if(textmode) printf(" \e[1;36m%d\e[0;3%cm(%d)",z,n>1?'5':n?'1':'4',n+1);
	x=0;
	for(i=1;i<tilesize;i++) {
		if(v) {
			p+=width;
			if(++x==theight) x=0,p+=1-tilesize;
		} else {
			p++;
		}
		if(n--) {
			*p=z;
		} else {
			*p=z=read_color(z);
			n=read_length();
			if(textmode) printf("\e[0;3%cm(%d)",n>1?'5':n?'1':'4',n+1);
		}
	}
}

static void input_uncompressed_picture(char*p) {
	int i,z;
	for(i=0;i<tilesize;i++) {
		z=getbit(); z|=getbit()<<1; z|=getbit()<<2; z|=getbit()<<3;
		p[i]=z;
		if(textmode) printf(" \e[1;36m%d",z);
	}
}

int main(int argc, char **argv) {
	int i,j;
	if (argc > 1 && (!strcmp(argv[1],"-h") || !strcmp(argv[1],"--help"))) {
		usage();
	}
	argc--; argv++;
	for(i=0;i<argc && i<16;i++) {
		j=strlen(argv[i]);
		if(j==6) {
			sscanf(argv[i],"%2hhX%2hhX%2hhX",pal[i].d+0,pal[i].d+2,pal[i].d+4);
			pal[i].d[1]=pal[i].d[0];
			pal[i].d[3]=pal[i].d[2];
			pal[i].d[5]=pal[i].d[4];
			pal[i].d[7]=pal[i].d[6]=255;
		} else if(j==8) {
			sscanf(argv[i],"%2hhX%2hhX%2hhX%2hhX",pal[i].d+0,pal[i].d+2,pal[i].d+4,pal[i].d+6);
			pal[i].d[1]=pal[i].d[0];
			pal[i].d[3]=pal[i].d[2];
			pal[i].d[5]=pal[i].d[4];
			pal[i].d[7]=pal[i].d[6];
		} else if(j==12) {
			sscanf(argv[i],"%2hhX%2hhX%2hhX%2hhX%2hhX%2hhX",pal[i].d+0,pal[i].d+1,pal[i].d+2,pal[i].d+3,pal[i].d+4,pal[i].d+5);
			pal[i].d[7]=pal[i].d[6]=255;
		} else if(j==16) {
			sscanf(argv[i],"%2hhX%2hhX%2hhX%2hhX%2hhX%2hhX%2hhX%2hhX",pal[i].d+0,pal[i].d+1,pal[i].d+2,pal[i].d+3,pal[i].d+4,pal[i].d+5,pal[i].d+6,pal[i].d+7);
		} else if(j==1 && argv[i][0]=='-') {
			textmode=1;
			break;
		} else {
			fprintf(stderr,"Invalid color\n");
			return 1;
		}
	}
	width=fgetc(stdin);
	theight=fgetc(stdin);
	if(width&128) {
		width&=127;
		for(i=0;i<16;i++) pal[i].d[6]=pal[i].d[7]=255;
		i=fgetc(stdin);
		if(!(i&~15)) pal[i].d[6]=pal[i].d[7]=0;
		for(i=0;i<16;i++) if(pal[i].d[6]) {
			if(argc<2) {
				pal[i].d[0]=pal[i].d[1]=fgetc(stdin);
				pal[i].d[2]=pal[i].d[3]=fgetc(stdin);
				pal[i].d[4]=pal[i].d[5]=fgetc(stdin);
			} else {
				fgetc(stdin); fgetc(stdin); fgetc(stdin);
			}
		}
	}
	tilesize=width*theight;
	load_adjacency_matrix();
	if(textmode) {
		printf("\e[mTile size: \e[1m%d\e[m x \e[1m%d\e[m = \e[1m%d\e[m\nFrequent adjacent colors:\n",width,theight,tilesize);
		for(i=0;i<16;i++) {
			printf(" \e[1;34m%2d\e[m -\e[32m",i);
			for(j=1;j<6;j++) printf(" %2d",matrix[i].i[j]);
			printf("\e[m\n");
		}
	}
	for(j=0;;) {
		i=getbit(); i|=getbit()<<1;
		if(!i) break;
		if(textmode) printf("Tile #\e[1m%d\e[m (\e[33m%s compression\e[m): ",ntile,i==1?"horizontal":i==2?"vertical":"no");
		ntile++;
		height+=theight;
		pic=realloc(pic,ntile*tilesize);
		if(!pic) {
			fprintf(stderr,"Allocation failed\n");
			return 1;
		}
		if(i==3) input_uncompressed_picture(pic+j);
		else input_compressed_picture(pic+j,i-1);
		j+=tilesize;
		if(textmode) printf("\e[m\n");
	}
	if(textmode) {
		puts("EOF");
		return 0;
	}
	fwrite("farbfeld",1,8,stdout);
	putchar(width>>24); putchar(width>>16); putchar(width>>8); putchar(width);
	putchar(height>>24); putchar(height>>16); putchar(height>>8); putchar(height);
	for(i=0;i<ntile*tilesize;i++) fwrite(pal[pic[i]].d,1,8,stdout);
	return 0;
}
