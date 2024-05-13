#if 0
gcc -s -O2 -o ~/bin/maki2ff -Wno-unused-result maki2ff.c
exit
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
	unsigned char data[8];
} Color;

static char format;
static char dblh;
static unsigned char buf[32];
static int width,height;
static unsigned char*flaga;
static unsigned char*pic;
static Color pal[256];

#define errx(A,...) do { fprintf(stderr,__VA_ARGS__); fputc('\n',stderr); exit(A); } while(0)
#define allocerr(A) if (!(A)) errx(1,"Allocation failed");

static void
usage(void)
{
	fprintf(stderr,
		"Usage: maki2ff                                                                \n"
		"                                                                              \n"
		"Converts Maki-chan graphics to farbfeld. maki2ff reads an MKI file from stdin,\n"
		"converts it to farbfeld and writes the result to stdout.                      \n"
		"                                                                              \n"
		"Maki-chan is a family of raster graphics formats that were mainly used in     \n"
		"Japan and it may be associated with MSX computers.                            \n"
		"                                                                              \n"
		"Maki-chan covers three formats: MKI (MAKI), MAG and MAX. This decoder only    \n"
		"supports the MAKI format.                                                     \n"
		"                                                                              \n"
		"This decoder does not take any arguments.                                     \n"
		"                                                                              \n"
		"Example usage:                                                                \n"
		"   $ maki2ff < image.mki > image.ff                                           \n"
		"   $ maki2ff < image.mki | bzip2 > image.ff.bz2                               \n"
		"\n"
	);
	exit(1);
}

static void do_version_1(void) {
	int a,b,x,y,z;
	width=640;
	height=400;
	fread(buf,1,32,stdin);
	fread(buf,1,4,stdin); // unused part of header
	dblh=buf[26]&1;
	// Read palette
	for(x=0;x<16;x++) {
		pal[x].data[6]=pal[x].data[7]=255;
		pal[x].data[2]=pal[x].data[3]=fgetc(stdin);
		pal[x].data[0]=pal[x].data[1]=fgetc(stdin);
		pal[x].data[4]=pal[x].data[5]=fgetc(stdin);
	}
	// Read flag A section
	allocerr(flaga=malloc(1000));
	fread(flaga,1,1000,stdin);
	allocerr(pic=calloc(320,400));
	// Read flag B section
	for(a=128,z=y=0;y<400;y+=4) {
		for(x=0;x<320;x+=4) {
			if(!a) z++,a=128;
			if(flaga[z]&a) {
				b=fgetc(stdin);
				if(b&0x80) pic[y*320+x+0]=1;
				if(b&0x40) pic[y*320+x+1]=1;
				if(b&0x20) pic[y*320+x+2]=1;
				if(b&0x10) pic[y*320+x+3]=1;
				if(b&0x08) pic[y*320+x+320]=1;
				if(b&0x04) pic[y*320+x+321]=1;
				if(b&0x02) pic[y*320+x+322]=1;
				if(b&0x01) pic[y*320+x+323]=1;
				b=fgetc(stdin);
				if(b&0x80) pic[y*320+x+640]=1;
				if(b&0x40) pic[y*320+x+641]=1;
				if(b&0x20) pic[y*320+x+642]=1;
				if(b&0x10) pic[y*320+x+643]=1;
				if(b&0x08) pic[y*320+x+960]=1;
				if(b&0x04) pic[y*320+x+961]=1;
				if(b&0x02) pic[y*320+x+962]=1;
				if(b&0x01) pic[y*320+x+963]=1;
			}
			a>>=1;
		}
	}
	free(flaga);
	// Read pixel section
	for(a=0;a<320*400;a++) if(pic[a]) pic[a]=fgetc(stdin);
	// XOR filter
	for(a=0,b=(format=='A'?640:1280);b<320*400;) pic[b++]^=pic[a++];
	// Output
	fwrite("farbfeld\x00\x00\x02\x80\x00\x00\x01\x90",1,16,stdout);
	for(a=0;a<320*400;a++) {
		b=pic[a];
		fwrite(pal+(b>>4),1,8,stdout);
		fwrite(pal+(b&15),1,8,stdout);
	}
}

static void do_version_2(void) {
	int dh,pb,pw,pl;
	int i,x,y,z;
	unsigned char*a;
	unsigned char*b;
	unsigned char*o;
	unsigned char*act;
	signed int q[16];
	for(;;) {
		i=fgetc(stdin);
		if(i==EOF) errx(1,"Unexpected end of file");
		if(i==0x1A) break;
	}
	for(;;) {
		i=fgetc(stdin);
		if(i==EOF) errx(1,"Unexpected end of file");
		if(!i) break;
	}
	fread(buf+1,1,31,stdin);
	width=256*(buf[9]-buf[5])+1+buf[8]-buf[4];
	height=256*(buf[11]-buf[7])+1+buf[10]-buf[6];
	if(buf[1]==3) buf[3]=(buf[2]==4?1:buf[2]==0x54||!buf[2]?0:128);
	dh=(pb=buf[3]&128?0:1)&buf[3];
	//pl=((buf[4]|(buf[5]<<8))>>pb)&0xFFFC;
	//pw=((((buf[8]|(buf[9]<<8))>>pb)+4)&0xFFFC)-pl;
	//pl=buf[4]&(pb?7:3);
	pw=pb?(width+1)>>1:width;
	x=buf[24]|(buf[25]<<8)|(buf[26]<<16)|(buf[27]<<24); x-=32;
	y=buf[12]|(buf[13]<<8)|(buf[14]<<16)|(buf[15]<<24); y-=32;
	z=buf[16]|(buf[17]<<8)|(buf[18]<<16)|(buf[19]<<24); z-=32;
	allocerr(flaga=malloc(x+0x300+pw*height));
	pic=flaga+x;
	fread(flaga,1,x,stdin);
	a=flaga;
	for(i=0;i<256;i++) {
		pal[i].data[6]=pal[i].data[7]=255;
		pal[i].data[2]=pal[i].data[3]=*a++;
		pal[i].data[0]=pal[i].data[1]=*a++;
		pal[i].data[4]=pal[i].data[5]=*a++;
	}
	a=flaga+y;
	b=flaga+z;
	o=pic;
	allocerr(act=calloc(pw,1));
	q[1]=-2; q[2]=-4; q[3]=-8;
	q[4]=-pw; q[5]=-pw-2;
	q[6]=-2*pw; q[7]=-2*pw-2; q[8]=-2*pw-4;
	q[9]=-4*pw; q[10]=-4*pw-2; q[11]=-4*pw-4;
	q[12]=-8*pw; q[13]=-8*pw-2; q[14]=-8*pw-4;
	q[15]=-16*pw;
	for(z=y=0;y<height;y++) for(x=0;x<pw/4;x++) {
		if(*a&(128>>z)) act[x]^=*b++;
		*o=(i=act[x]>>4)?o[q[i]]:fgetc(stdin); o++;
		*o=(i=act[x]>>4)?o[q[i]]:fgetc(stdin); o++;
		*o=(i=act[x]&15)?o[q[i]]:fgetc(stdin); o++;
		*o=(i=act[x]&15)?o[q[i]]:fgetc(stdin); o++;
		if(++z==8) z=0,a++;
	}
	// Output
	fwrite("farbfeld",1,8,stdout);
	z=height<<dh;
	putchar(width>>24); putchar(width>>16); putchar(width>>8); putchar(width);
	putchar(z>>24); putchar(z>>16); putchar(z>>8); putchar(z);
	if(pb) {
		for(y=0;y<height;y++) {
			for(z=0;z<=dh;z++) for(x=0;x<pw;x++) {
				fwrite(pal[pic[y*pw+x]>>4].data,1,8,stdout);
				fwrite(pal[pic[y*pw+x]&15].data,1,8,stdout);
			}
		}
	} else {
		for(y=0;y<height;y++) {
			for(z=0;z<=dh;z++) for(x=0;x<width;x++) fwrite(pal[pic[y*pw+x]].data,1,8,stdout);
		}
	}
}

int main(int argc, char **argv) {
	if (argc > 1)
		usage();
	fread(buf,1,12,stdin);
	if(memcmp(buf,"MAKI0",5)) errx(1,"Not a Maki-chan file");
	format=buf[buf[5]=='1'?6:5];
	if(format=='2') do_version_2(); else do_version_1();
	return 0;
}
