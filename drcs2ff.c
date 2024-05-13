#if 0
gcc -s -O2 -o ~/bin/drcs2ff -Wno-unused-result drcs2ff.c
exit
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define fatal(...) do{ fprintf(stderr,__VA_ARGS__); exit(1); }while (0)

static int pcn,pcmw,pcmh;
static unsigned char bgcolor[8];
static unsigned char fgcolor[8];
static unsigned char sixel[32];

static void
usage(void)
{
	fprintf(stderr,
		"Usage: drcs2ff <bg color> <fg color>                                          \n"
		"                                                                              \n"
		"Convert DEC VT320 font file to farbfeld. This is in the context of a DEC VT320\n"
		"terminal and the font file is referred to as a Dynamically Redefined Character\n"
		"Set (shortened to DRCS). drcs2ff reads a DRCS font file from stdin, converts  \n"
		"it to farbfeld and writes the result to stdout.                               \n"
		"                                                                              \n"
		"Note that the aspect ratio of the picture is different to what would be       \n"
		"visible on a actual DEC VT320 terminal.                                       \n"
		"                                                                              \n"
		"Takes two arguments; the background and foreground colour. You can use - to   \n"
		"make it transparent.                                                          \n"
		"                                                                              \n"
		"The colour arguments can follow these patterns:                               \n"
		"   -                 transparent                                              \n"
		"   rrggbb            three pairs of 8-bit hex values                          \n"
		"   rrggbbaa          four pairs of 8-bit hex values                           \n"
		"   rrrrggggbbbb      three pairs of 16-bit hex values                         \n"
		"   rrrrggggbbbbaaaa  four pairs of 16-bit hex values                          \n"
		"                                                                              \n"
		"The character set name from the input is ignored.                             \n"
		"                                                                              \n"
		"Example usage:                                                                \n"
		"   $ drcs2ff - A03811 < drcs.font > image.ff                                  \n"
		"\n"
	);
	exit(1);
}

static void
define_color(unsigned char *out, const char *in)
{
	switch(strlen(in)) {
		case 1:
			if (*in!='-') fatal("Invalid color specification\n");
			break;
		case 6:
			sscanf(in,"%2hhX%2hhX%2hhX",out+0,out+2,out+4);
			out[1]=out[0];
			out[3]=out[2];
			out[5]=out[4];
			out[7]=out[6]=255;
			break;
		case 8:
			sscanf(in,"%2hhX%2hhX%2hhX%2hhX",out+0,out+2,out+4,out+6);
			out[1]=out[0];
			out[3]=out[2];
			out[5]=out[4];
			out[7]=out[6];
			break;
		case 12:
			sscanf(in,"%2hhX%2hhX%2hhX%2hhX%2hhX%2hhX",out+0,out+1,out+2,out+3,out+4,out+5);
			out[7]=out[6]=255;
			break;
		case 16:
			sscanf(in,"%2hhX%2hhX%2hhX%2hhX%2hhX%2hhX%2hhX%2hhX",out+0,out+1,out+2,out+3,out+4,out+5,out+6,out+7);
			break;
		default:
			fatal("Invalid color specification\n");
	}
}

static int
read_number(int *p)
{
	int c;
	int n=0;
	for (;;) {
		c=fgetc(stdin);
		if (c>='0' && c<='9') {
			n=10*n+c-'0';
		} else if (c==';' || c=='{') {
			if (n<0) fatal("Number too long\n");
			*p=n;
			return c=='{';
		} else if (c && c!='\r' && c!='\n') {
			fatal("Invalid numeric character (%d)\n",c);
		}
	}
}

static void
read_header(void)
{
	int c=fgetc(stdin);
	int n;
	while (!c) c=fgetc(stdin);
	if ((c==27 && fgetc(stdin)!='P') || (c!=27 && c!='P'+64)) fatal("First character should be DCS or ESC P\n");
	if (read_number(&n)) goto eoh; // Pfn
	if (read_number(&pcn)) goto eoh; // Pcn
	if (read_number(&n)) goto eoh; // Pe
	if (read_number(&pcmw)) goto eoh; // Pcmw
	if (pcmw==1 || pcmw>15) fatal("Illegal character matrix width (%d)\n",pcmw);
	if (pcmw && pcmw<5) pcmw+=3,pcmh=10;
	if (read_number(&n)) goto eoh; // Pw
	if (!pcmw && n==2) pcmw=9;
	if (read_number(&n)) goto eoh; // Pt
	if (read_number(pcmh?&n:&pcmh)) goto eoh; // Pcmh
	if (pcmh>12) fatal("Invalid character matrix height (%d)\n",pcmh);
	c=read_number(&n); // Pcss
	if (n && !pcn) pcn=-1;
	while (!c) c=read_number(&n);
	eoh:
	if (!pcmw) pcmw=15;
	if (!pcmh) pcmh=12;
	if (pcn<=0) ++pcn;
	if (pcn>95) fatal("Illegal starting character (%d)\n",pcn);
	// Character set name
	for (;;) {
		c=fgetc(stdin);
		if (c==EOF || c==7 || c==27 || c>127) fatal("Invalid character set name\n");
		if (c>='0' && c<='~') break;
	}
}

static void
write_header(void)
{
	int n;
	fwrite("farbfeld",1,8,stdout);
	putchar(0); putchar(0); putchar(0); putchar(pcmw);
	n=pcmh*128;
	putchar(0); putchar(0); putchar(n>>8); putchar(n);
	n=pcmw*pcmh*(pcn+32);
	while (n--) fwrite(bgcolor,1,8,stdout);
}

static void
do_characters(void)
{
	int re=96-pcn;
	int c,si,x,y,z;
	while (re--) {
		si=0;
		memset(sixel,0,32);
		for (;;) {
			c=fgetc(stdin);
			if (c==EOF || c==7 || c==';' || c>128) break;
			if (c=='/') {
				si=16;
			} else if (c>='?' && c<='~' && (si&15)!=pcmw) {
				sixel[si++]=c-'?';
			}
		}
		for (y=0;y<pcmh;y++) for (x=0;x<pcmw;x++) fwrite(sixel[(y>5?16:0)+x]&(1<<(y%6))?fgcolor:bgcolor,1,8,stdout);
		if (c==7 || c>128) break;
	}
	if (re>0) {
		re*=pcmw*pcmh;
		while (re--) fwrite(bgcolor,1,8,stdout);
	}
}

int
main(int argc, char **argv)
{
	if (argc<3) usage();
	define_color(bgcolor,argv[1]);
	define_color(fgcolor,argv[2]);
	read_header();
	write_header();
	do_characters();
	return 0;
}
