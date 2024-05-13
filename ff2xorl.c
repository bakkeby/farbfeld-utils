#if 0
gcc -s -O2 -o ~/bin/ff2xorl -Wno-unused-result ff2xorl.c
exit
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
	unsigned char d[3];
	unsigned char v;
} Color;

static unsigned char buf[16];
static int width,height,total;
static unsigned char*pic;
static Color pal[16];
static int dif[16];
static unsigned short ring[16];
static int ring_index;

#define errx(A,...) do{ fprintf(stderr,__VA_ARGS__); fputc('\n',stderr); exit(A); }while(0)
#define allocerr(A) if(!(A)) errx(1,"Allocation failed");

static void
usage(void)
{
	fprintf(stderr,
		"Usage: ff2xorl <?palette?>                                                    \n"
		"                                                                              \n"
		"Converts farbfeld to XORLINES-2 format. ff2xorl reads farbfeld from stdin,    \n"
		"converts that to XORLINES-2 format and writes the result to stdout.           \n"
		"                                                                              \n"
		"The context around the XORLINES-2 format is not documented.                   \n"
		"                                                                              \n"
		"Restrictions:                                                                 \n"
		"   - only images with even width size are valid                               \n"
		"   - the picture can't be too big                                             \n"
		"   - the picture can't contain more than 16 colours                           \n"
		"                                                                              \n"
		"Optionally, specify up to sixteen triples of hex (RGB) to make the palette.   \n"
		"If you do not specify the full palette, it will automatically add the rest to \n"
		"make sixteen (or however many colours are in the picture). Their order can    \n"
		"affect how good the compression is.                                           \n"
		"                                                                              \n"
		"Example usage:                                                                \n"
		"   $ ff2xorl < image.ff > image.xorl                                          \n"
		"\n"
	);
	exit(1);
}

static int best_run(int d,int at,int z) {
	int n=0;
	int v=d?pic[at]^pic[at-d]:pic[at];
	int u;
	while(n<z && at+n<total) {
		u=d?pic[at+n]^pic[at+n-d]:pic[at+n];
		if(u!=v) break;
		n++;
	}
	return n;
}

static int encode_at(int at) {
	int i,j,x,y;
	if(at==total-1) {
		putchar(0x00);
		putchar(pic[at]);
		return total;
	}
	for(y=i=0;i<16;i++) if(at>=dif[i]) {
		j=best_run(dif[i],at,128);
		if(j>y) y=j,x=i;
	}
	if(y==128 && !x) {
		putchar(pic[at]|0xF0);
		return at+y;
	} else if(y>2) {
		j=(x<<4)|(x?pic[at]^pic[at-dif[x]]:pic[at]);
		for(i=0;i<16;i++) if(ring[i]==256*y+j) {
			putchar(i|0xE0);
			return at+y;
		}
		ring[ring_index]=256*y+j;
		ring_index=(ring_index+1)&15;
		putchar(y-1);
		putchar(j);
		return at+y;
	} else {
		for(i=2;i<192;i+=2) {
			y=(i<190?4:3);
			for(j=0;j<16;j++) if(at+i>=dif[j] && best_run(dif[j],at+i,y)==y) {
				if(i>2) i-=2;
				goto done;
			}
		}
		done:
		putchar(i/2+127);
		for(x=0;x<i;x+=2) putchar((pic[at+x]<<4)|pic[at+x+1]);
		return at+i;
	}
}

int main(int argc,char**argv) {
	int i,j;
	if (argc > 1 && (!strcmp(argv[1],"-h") || !strcmp(argv[1],"--help"))) {
		usage();
	}
	fread(buf,1,16,stdin);
	if(memcmp(buf,"farbfeld",8)) errx(1,"Not farbfeld");
	if(buf[8]|buf[9]|buf[12]|buf[13]) errx(1,"Picture is too big");
	width=(buf[10]<<8)|buf[11];
	height=(buf[14]<<8)|buf[15];
	if(width&1) errx(1,"Width is odd; only even widths are valid");
	allocerr(pic=malloc(1+(total=width*height)));
	for(i=0;i<16;i++) if(argc>i+1) {
		sscanf(argv[i+1],"%1hhX%1hhX%1hhX",pal[i].d+0,pal[i].d+1,pal[i].d+2);
		pal[i].d[0]*=0x11;
		pal[i].d[1]*=0x11;
		pal[i].d[2]*=0x11;
		pal[i].v=1;
	}
	for(i=0;i<total;i++) {
		fread(buf,1,8,stdin);
		for(j=0;j<16;j++) {
			if(!pal[j].v) {
				pal[j].d[0]=buf[0];
				pal[j].d[1]=buf[2];
				pal[j].d[2]=buf[4];
				pal[j].v=1;
				goto found;
			}
			if(buf[0]==pal[j].d[0] && buf[2]==pal[j].d[1] && buf[4]==pal[j].d[2]) goto found;
		}
		errx(1,"Too many colors");
		found: pic[i]=j;
	}
	putchar(width); putchar(width>>8);
	putchar(height); putchar(height>>8);
	for(i=0;i<3;i++) for(j=0;j<16;j+=2) putchar((pal[j].d[i]&0xF0)|(pal[j+1].d[i]>>4));
	for(i=0;i<16;i++) dif[i]="\0\1\2\4"[i&3]+"\0\1\2\4"[i>>2]*width;
	for(i=0;i<total;) i=encode_at(i);
	return 0;
}
