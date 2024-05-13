#if 0
gcc -s -O2 -o ~/bin/fuun2ff -Wno-unused-result fuun2ff.c
exit
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
	unsigned char r[10];
	unsigned char g[10];
	unsigned char b[10];
	unsigned char a[10];
} Pixel;

#define East 0
#define North 1
#define West 2
#define South 3

static int width,height;
static Pixel*pic;
static char command[7];
static int curx,cury,markx,marky,indx,dir;
static int rgb,trans,red,green,blue,alpha;
static unsigned char pxr,pxg,pxb,pxa,hr,hg,hb,ha;

#define Here(z) pic[cury*width+curx].z[indx]
#define At(x,y,z) pic[(y)*width+(x)].z[indx]

static void
usage(void)
{
	fprintf(stderr,
		"Usage: fuun2ff <?width?> <?height?>                                           \n"
		"                                                                              \n"
		"Convert Fuun RNA description to farbfeld. fuun2ff reads Fuun RNA description  \n"
		"from stdin, converts it to farbfeld and writes the result to stdout.          \n"
		"                                                                              \n"
		"Fuun RNA is a graphic description language. It was defined for the task of the\n"
		"ICFP contest 2007 (“Morph Endo”).                                             \n"
		"                                                                              \n"
		"The RNA of the extraterrestrial Fuun species (program) is a string of RNA     \n"
		"bases, where a base is one of the following four letters: I, C, F, P. RNA is  \n"
		"normally generated from Fuun DNA. The RNA is always read as a sequence of     \n"
		"chunks of 7 bases each (and the DNA actually writes it in the same 7 chunks). \n"
		"Every 7 byte chunk encodes an operation, and all the operations of the program\n"
		"are executed sequentially, without any forms of flow control. There are 21    \n"
		"operations, 20 of which each have a single encoding to chunks, and every other\n"
		"code is interpreted as a no-op.                                               \n"
		"                                                                              \n"
		"The output of an RNA program is an 8 bit deep RGB bitmap image of size 600x600\n"
		"pixels.                                                                       \n"
		"                                                                              \n"
		"Optionally the image size can be specified with one or two command line       \n"
		"arguments:                                                                    \n"
		"   fuun2ff 800       - will output a bitmap image of size 800x800             \n"
		"   fuun2ff 600 440   - will output a bitmap image of size 600x448             \n"
		"                                                                              \n"
		"Example usage:                                                                \n"
		"   $ fuun2ff 1000 < RNA.txt > image.ff                                        \n"
		"\n"
	);
	exit(1);
}

static inline void
make_pixel(void)
{
	if (rgb) {
		pxr=(255*red)/rgb;
		pxg=(255*green)/rgb;
		pxb=(255*blue)/rgb;
	} else {
		pxr=pxg=pxb=0;
	}
	pxa=trans?(255*alpha)/trans:255;
	// Premultiply alpha
	pxr=(pxa*pxr)/255;
	pxg=(pxa*pxg)/255;
	pxb=(pxa*pxb)/255;
}

static void
do_flood(int x,int y)
{
	int s,e;
	if (At(x,y,r)!=hr || At(x,y,g)!=hg || At(x,y,b)!=hb || At(x,y,a)!=ha) return;
	while (x && At(x-1,y,r)==hr && At(x-1,y,g)==hg && At(x-1,y,b)==hb && At(x-1,y,a)==ha) x--;
	s=x;
	for (;;) {
		At(x,y,r)=pxr; At(x,y,g)=pxg; At(x,y,b)=pxb; At(x,y,a)=pxa;
		x++;
		if (x==width || At(x,y,r)!=hr || At(x,y,g)!=hg || At(x,y,b)!=hb || At(x,y,a)!=ha) break;
	}
	e=x;
	for (x=s;x<e;x++) {
		if (y) do_flood(x,y-1);
		if (y<height-1) do_flood(x,y+1);
	}
}

static void
do_clip(void)
{
	int p=width*height;
	while (p--) {
		pic[p].r[indx-1]=(pic[p].r[indx-1]*pic[p].a[indx])/255;
		pic[p].g[indx-1]=(pic[p].g[indx-1]*pic[p].a[indx])/255;
		pic[p].b[indx-1]=(pic[p].b[indx-1]*pic[p].a[indx])/255;
		pic[p].a[indx-1]=(pic[p].a[indx-1]*pic[p].a[indx])/255;
	}
	indx--;
}

static void
do_compose(void)
{
	int p=width*height;
	int a;
	while (p--) {
		a=255-pic[p].a[indx];
		pic[p].r[indx-1]=pic[p].r[indx]+(pic[p].r[indx-1]*a)/255;
		pic[p].g[indx-1]=pic[p].g[indx]+(pic[p].g[indx-1]*a)/255;
		pic[p].b[indx-1]=pic[p].b[indx]+(pic[p].b[indx-1]*a)/255;
		pic[p].a[indx-1]=pic[p].a[indx]+(pic[p].a[indx-1]*a)/255;
	}
	indx--;
}

static void
draw_line(void)
{
	int dx=markx-curx;
	int dy=marky-cury;
	int d=abs(abs(dx)>abs(dy)?dx:dy);
	int c=dx*dy>0?0:1;
	int x=curx*d+((d-c)>>1);
	int y=cury*d+((d-c)>>1);
	int n=d;
	At(curx,cury,r)=pxr; At(curx,cury,g)=pxg; At(curx,cury,b)=pxb; At(curx,cury,a)=pxa;
	At(markx,marky,r)=pxr; At(markx,marky,g)=pxg; At(markx,marky,b)=pxb; At(markx,marky,a)=pxa;
	while(n--) {
		At(x/d,y/d,r)=pxr; At(x/d,y/d,g)=pxg; At(x/d,y/d,b)=pxb; At(x/d,y/d,a)=pxa;
		x+=dx; y+=dy;
	}
}

#define Case(x,y) case (x*4+y)
static inline void
do_command(void)
{
	if (command[1]=='I' && command[2]=='P') {
		if (command[3]!='I' || command[4]!='I') return;
		switch(command[5]*4+command[6]) {
			Case('I','C'): rgb++; break;
			Case('I','P'): rgb++; red++; break;
			Case('C','C'): rgb++; green++; break;
			Case('C','F'): rgb++; red++; green++; break;
			Case('C','P'): rgb++; blue++; break;
			Case('F','C'): rgb++; red++; blue++; break;
			Case('F','F'): rgb++; green++; blue++; break;
			Case('P','C'): rgb++; red++; green++; blue++; break;
			Case('P','F'): trans++; break;
			Case('P','P'): trans++; alpha++; break;
		}
	} else if (command[1]!=command[2]) {
		return;
	} else if (command[1]=='I' && command[4]=='I' && command[6]=='P') {
		if (command[3]=='P' && command[5]=='C') {
			rgb=trans=red=green=blue=alpha=0;
		} else if (command[3]=='I' && command[5]=='I') {
			switch(dir) {
				case East: curx=(curx+1)%width; break;
				case North: cury=(cury+height-1)%height; break;
				case West: curx=(curx+width-1)%width; break;
				case South: cury=(cury+1)%height; break;
			}
		} else if (command[3]=='P' && command[5]=='I') {
			make_pixel();
			ha=Here(r); hg=Here(g); hb=Here(b); ha=Here(a);
			if(pxr!=hr || pxg!=hg || pxb!=hb || pxa!=ha) do_flood(curx,cury);
		}
	} else if (command[4]!=command[5]) {
		return;
	} else if (command[1]=='F' && command[3]=='I' && command[4]=='C' && command[6]=='F' && indx) {
		do_clip();
	} else if (command[6]!='P') {
		return;
	} else if (command[1]=='C') {
		if (command[3]=='C' && command[4]=='C') {
			dir=(dir+1)&3;
		} else if (command[3]=='I' && command[4]=='F') {
			markx=curx; marky=cury;
		} else if (command[3]=='P' && command[4]=='F' && indx!=9) {
			int i=width*height-1;
			indx++;
			while (i) pic[i].r[indx]=pic[i].g[indx]=pic[i].b[indx]=pic[i].a[indx]=0,--i;
		}
	} else if (command[1]=='F') {
		if (command[3]=='F' && command[4]=='F') {
			dir=(dir+3)&3;
		} else if (command[3]=='I' && command[4]=='C') {
			make_pixel();
			draw_line();
		} else if (command[3]=='P' && command[4]=='C' && indx) {
			do_compose();
		}
	}
}

int
main(int argc, char **argv)
{
	int x,y;
	width=height=600;
	if (argc>1) {
		if (!strcmp(argv[1],"-h") || !strcmp(argv[1],"--help"))
			usage();
		width=height=strtol(argv[1],0,0);
	}
	if (argc>2) height=strtol(argv[2],0,0);
	pic=calloc(width*height,sizeof(Pixel));
	if (!pic) {
		fprintf(stderr,"Allocation failed\n");
		return 1;
	}
	x=0;
	for (;;) {
		y=fgetc(stdin);
		if (y=='I' || y=='C' || y=='F' || y=='P') {
			command[x++]=y;
			if (x==7) {
				x=0;
				if (*command=='P') do_command();
			}
		} else if (y==EOF) {
			break;
		}
	}
	fwrite("farbfeld",1,8,stdout);
	putchar(width>>24); putchar(width>>16); putchar(width>>8); putchar(width);
	putchar(height>>24); putchar(height>>16); putchar(height>>8); putchar(height);
	for (x=0; x<width*height; x++) {
		putchar(pic[x].r[indx]); putchar(pic[x].r[indx]);
		putchar(pic[x].g[indx]); putchar(pic[x].g[indx]);
		putchar(pic[x].b[indx]); putchar(pic[x].b[indx]);
		putchar(pic[x].a[indx]); putchar(pic[x].a[indx]);
	}
	return 0;
}
