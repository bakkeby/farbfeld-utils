#if 0
gcc -s -O2 -o ~/bin/ff-vccapture -Wno-unused-result ff-vccapture.c
exit
#endif

#ifndef __linux__
#error "This program is only for Linux."
#endif

#include <err.h>
#include <fcntl.h>
#include <linux/kd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

typedef struct {
	unsigned char r,g,b;
} Color;
typedef struct {
	unsigned char d[8];
} Pixel;

static int lines,columns;
static int tty,vcsa;
static unsigned char buf[16];
static unsigned short*screen;
static struct console_font_op fontop;
static struct consolefontdesc fontdesc;
static unsigned char fontdata[0x10000];
static Color colors[16];
static Pixel pal[16];

static void
usage(void)
{
	fprintf(stderr,
		"Usage: ff-vccapture <virtual console screen number>                           \n"
		"                                                                              \n"
		"This farbfeld program takes a screenshot of a Linux virtual console screen.   \n"
		"                                                                              \n"
		"This program does not read from standard in and only works on Linux.          \n"
		"                                                                              \n"
		"This program takes a single argument which is a virtual console screen number.\n"
		"                                                                              \n"
		"To test this you can open a new TTY by hitting e.g. Ctrl+Alt+F5 and log in,   \n"
		"then go back to your current session and pass 5 to the program. This may      \n"
		"require sudo depending on permissions.                                        \n"
		"                                                                              \n"
		"Example usage:                                                                \n"
		"   $ ff-vccapture 5 > vccapture.ff                                            \n"
		"\n"
	);
	exit(1);
}

static void do_line(void) {
	int a,c,x,y,z;
	for(y=0;y<fontdesc.charheight;y++) {
		for(x=0;x<columns;x++) {
			c=screen[x];
			if(fontdesc.charcount==256) a=c>>8,c&=0xFF; else a=c>>9,c&=0x1FF;
			for(z=7;z>=0;z--) fwrite(pal[fontdata[(c<<5)+y]&(1<<z)?a&15:(a>>3)&7].d,1,8,stdout);
		}
	}
}

int main(int argc,char**argv) {
	int x;
	static const char m[16]={0,4,2,6,1,5,3,7,8,12,10,14,9,13,11,15};
	if (argc>1 && (!strcmp(argv[1],"-h") || !strcmp(argv[1],"--help"))) {
		usage();
	}
	if(argc!=2) errx(1,"Incorrect number of arguments");
	snprintf(buf,15,"/dev/tty%d",255&(int)(strtol(argv[1],0,10)));
	tty=open(buf,O_RDONLY|O_NOCTTY);
	if(tty==-1) err(1,"Cannot open tty device");
	snprintf(buf,15,"/dev/vcsa%d",255&(int)(strtol(argv[1],0,10)));
	vcsa=open(buf,O_RDONLY|O_NOCTTY);
	if(vcsa==-1) err(1,"Cannot open vcsa device");
	read(vcsa,buf,4);
	lines=buf[0];
	columns=buf[1];
	screen=malloc(lines*columns*sizeof(unsigned short));
	if(!screen) errx(1,"Allocation failed");
	read(vcsa,screen,lines*columns*sizeof(unsigned short));
	ioctl(tty,GIO_CMAP,(void*)colors);
	fontdesc.charcount=512;
	fontdesc.charheight=0;
	fontdesc.chardata=(void*)fontdata;
	ioctl(tty,GIO_FONTX,(void*)&fontdesc);
	if(!fontdesc.charheight) {
		fontop.op=KD_FONT_OP_GET;
		fontop.charcount=512;
		fontop.width=8;
		fontop.height=32;
		fontop.data=(void*)fontdata;
		ioctl(tty,KDFONTOP,(void*)&fontop);
		fontdesc.charcount=fontop.charcount;
		fontdesc.charheight=fontop.height;
		//err(1,"Unable to load font");
	}
	close(tty);
	close(vcsa);
	fwrite("farbfeld",1,8,stdout);
	putchar(columns>>21); putchar(columns>>13); putchar(columns>>5); putchar(columns<<3);
	x=lines*fontdesc.charheight;
	putchar(x>>24); putchar(x>>16); putchar(x>>8); putchar(x);
	for(x=0;x<16;x++) {
		pal[x].d[0]=pal[x].d[1]=colors[m[x]].r;
		pal[x].d[2]=pal[x].d[3]=colors[m[x]].g;
		pal[x].d[4]=pal[x].d[5]=colors[m[x]].b;
		pal[x].d[6]=pal[x].d[7]=255;
	}
	for(x=0;x<lines;x++) {
		do_line();
		screen+=columns;
	}
	return 0;
}
