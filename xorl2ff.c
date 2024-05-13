#if 0
gcc -s -O2 -o ~/bin/xorl2ff -Wno-unused-result xorl2ff.c
exit
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
	unsigned char d[8];
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
		"Usage: xorl2ff                                                                \n"
		"                                                                              \n"
		"Converts XORLINES-2 format to farbfeld. xorl2ff reads XORLINES-2 format from  \n"
		"stdin, converts that farbfeld and writes the result to stdout.                \n"
		"                                                                              \n"
		"The context around the XORLINES-2 format is not documented.                   \n"
		"                                                                              \n"
		"This decoder takes no arguments.                                              \n"
		"                                                                              \n"
		"Example usage:                                                                \n"
		"   $ xorl2ff < image.xorl > image.ff                                          \n"
		"\n"
	);
	exit(1);
}

static int process(int at,int cmd) {
	int i,j;
	if(cmd>=0xF0) {
		// Shortcut for homogeneous run of 128
		cmd+=0x7E10;
	} else if(cmd>=0xE0) {
		// Recall homogeneous run from ring
		cmd=ring[cmd&15];
	} else if(cmd>=0x80) {
		// Heterogeneous run
		while(cmd-->0x7F) {
			i=fgetc(stdin);
			pic[at++]=i>>4;
			pic[at++]=i&15;
		}
		return at;
	} else {
		// Homogeneous run; record in ring
		cmd=(cmd<<8)|fgetc(stdin);
		ring[ring_index++]=cmd;
		ring_index&=15;
	}
	if(cmd<0) return total; //errx(2,"Data error: %d",cmd);
	// Execute a homogeneous run
	i=dif[(cmd>>4)&15];
	j=cmd&15;
	while(cmd>=0) {
		pic[at]=j^pic[at-i];
		cmd-=0x100;
		at++;
	}
	return at;
}

int main(int argc,char**argv) {
	int i,j;
	if (argc > 1) {
		usage();
	}
	fread(buf,1,4,stdin);
	width=(buf[1]<<8)|buf[0];
	height=(buf[3]<<8)|buf[2];
	for(i=0;i<16;i++) dif[i]="\0\1\2\4"[i&3]+"\0\1\2\4"[i>>2]*width;
	allocerr(pic=malloc(total=width*height));
	memset(pic,0,total);
	fwrite("farbfeld\0",1,10,stdout);
	putchar(buf[1]); putchar(buf[0]); putchar(0); putchar(0); putchar(buf[3]); putchar(buf[2]);
	for(i=0;i<16;i++) pal[i].d[6]=pal[i].d[7]=255;
	for(i=0;i<3;i++) {
		fread(buf,1,8,stdin);
		for(j=0;j<8;j++) {
			pal[j+j].d[i+i]=pal[j+j].d[i+i+1]=0x11*(buf[j]>>4);
			pal[j+j+1].d[i+i]=pal[j+j+1].d[i+i+1]=0x11*(buf[j]&15);
		}
	}
	for(i=0;i<total;) i=process(i,fgetc(stdin));
	for(i=0;i<total;i++) fwrite(pal[pic[i]].d,1,8,stdout);
	return 0;
}
