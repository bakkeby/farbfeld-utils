#if 0
gcc -s -O2 -o ~/bin/fhm2ff -Wno-unused-result fhm2ff.c
exit
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
	unsigned char x[8];
} Color;

static unsigned char buf[32];
static int size,num,meth;
static Color pal[256];
static unsigned char*pic;

static void
usage(void)
{
	fprintf(stderr,
		"Usage: fhm2ff <picture number>                                                \n"
		"                                                                              \n"
		"Reads a .IMG lump from a Free Hero Mesh file. fhm2ff reads an .IMG file from  \n"
		"stdin, extracts the picture at the given index, converts that to farbfeld and \n"
		"writes the result to stdout.                                                  \n"
		"                                                                              \n"
		"Pass only the contents of the .IMG lump on stdin; not the entire .xclass file.\n"
		"You can use har to extract the lump you want.                                 \n"
		"                                                                              \n"
		"The .DEP lumps cannot be read by this program.                                \n"
		"                                                                              \n"
		"The command-line argument is the picture number from 0 to 15; the different   \n"
		"pictures are alternative sizes of the same thing.                             \n"
		"                                                                              \n"
		"Example usage:                                                                \n"
		"   $ fhm2ff 3 < lump.IMG > image.ff                                           \n"
		"   $ fhm2ff 0 < lump.IMG > image.ff                                           \n"
		"\n"
	);
	exit(1);
}

static const char default_palette[]=
	"C020FF "
	"000000 222222 333333 444444 555555 666666 777777 888888 999999 AAAAAA BBBBBB CCCCCC DDDDDD EEEEEE FFFFFF "
	"281400 412300 5F3200 842100 A05000 C35F14 E1731E FF8232 FF9141 FFA050 FFAF5F FFBE73 FFD282 FFE191 FFF0A0 "
	"321E1E 412220 5F2830 823040 A03A4C BE4658 E15464 FF6670 FF7F7B FF8E7F FF9F7F FFAF7F FFBF7F FFCF7F FFDF7F "
	"280D0D 401515 602020 802A2A A03535 C04040 E04A4A FF5555 FF6764 FF6F64 FF7584 FF849D FF94B7 FF9FD1 FFAEEA "
	"901400 A02000 B03000 C04000 D05000 E06000 F07000 FF8000 FF9000 FFA000 FFB000 FFC000 FFD000 FFE000 FFF000 "
	"280000 400000 600000 800000 A00000 C00000 E00000 FF0000 FF2828 FF4040 FF6060 FF8080 FFA0A0 FFC0C0 FFE0E0 "
	"280028 400040 600060 800080 A000A0 C000C0 E000E0 FF00FF FF28FF FF40FF FF60FF FF80FF FFA0FF FFC0FF FFE0FF "
	"281428 402040 603060 804080 A050A0 C060C0 E070E0 FF7CFF FF8CFF FF9CFF FFACFF FFBCFF FFCCFF FFDCFF FFECFF "
	"280050 350566 420A7C 4F0F92 5C14A8 6919BE 761ED4 8323EA 9028FF A040FF B060FF C080FF D0A0FF E0C0FF F0E0FF "
	"000028 000040 000060 000080 0000A0 0000C0 0000E0 0000FF 0A28FF 284AFF 466AFF 678AFF 87AAFF A7CAFF C7EBFF "
	"0F1E1E 142323 193232 1E4141 285050 325F5F 377373 418282 469191 50A0A0 5AAFAF 5FC3C3 69D2D2 73E1E1 78F0F0 "
	"002828 004040 006060 008080 00A0A0 00C0C0 00E0E0 00FFFF 28FFFF 40FFFF 60FFFF 80FFFF A0FFFF C0FFFF E0FFFF "
	"002800 004000 006000 008000 00A000 00C000 00E000 00FF00 28FF28 40FF40 60FF60 80FF80 A0FFA0 C0FFC0 E0FFE0 "
	"002110 234123 325F32 418241 50A050 5FC35F 73E173 85FF7A 91FF6E A0FF5F B4FF50 C3FF41 D2FF32 E1FF23 F0FF0F "
	"282800 404000 606000 808000 A0A000 C0C000 E0E000 FFFF00 FFFF28 FFFF40 FFFF60 FFFF80 FFFFA0 FFFFC0 FFFFE0 "
	//
	"442100 00FF55 0055FF FF5500 55FF00 FF0055 5500FF CA8B25 F078F0 F0F078 FF7F00 DD6D01 7AFF00 111111 "
	//
	"000000 0000AA 00AA00 00AAAA AA0000 AA00AA AAAA00 AAAAAA "
	"555555 5555FF 55FF55 55FFFF FF5555 FF55FF FFFF55 FFFFFF "
;

static void load_palette(void) {
	int i;
	for(i=0;i<256;i++) {
		sscanf(default_palette+i*7,"%2hhX%2hhX%2hhX ",pal[i].x+0,pal[i].x+2,pal[i].x+4);
		pal[i].x[1]=pal[i].x[0];
		pal[i].x[3]=pal[i].x[2];
		pal[i].x[5]=pal[i].x[4];
		pal[i].x[7]=pal[i].x[6]=i?255:0;
	}
}

static void load_picture(int id) {
	int c,n,t,x,y;
	unsigned char*p;
	meth=(id?buf[(*buf&15)+1+((id-1)>>1)]>>(id&1?0:4):*buf>>4)&15;
	size=buf[id+1];
	pic=realloc(pic,size*size);
	if(!pic) {
		fprintf(stderr,"Allocation failed\n");
		exit(1);
	}
	if(meth==15) {
		fread(pic,size,size,stdin);
		return;
	}
	p=pic;
	n=t=0;
	y=size*size;
	while(y--) {
		if(!n) {
			n=fgetc(stdin);
			if(n<85) {
				// Homogeneous run
				n++;
				x=fgetc(stdin);
				if(t==1 && x==c) n*=85; else n++;
				c=x;
				t=1;
			} else if(n<170) {
				// Heterogeneous run
				n-=84;
				t=2;
			} else {
				// Copy-above run
				n-=169;
				if(t==3) n*=85;
				t=3;
			}
		}
		n--;
		if(t==2) c=fgetc(stdin);
		if(t==3) c=p-pic>=size?p[-size]:0;
		*p++=c;
	}
}

static inline void out_picture(void) {
	int x,y;
	if(meth==5 || meth==6) meth^=3;
	for(y=0;y<size;y++) for(x=0;x<size;x++) {
		if(meth&1) x=size-x-1;
		if(meth&2) y=size-y-1;
		fwrite(pal[pic[meth&4?x*size+y:y*size+x]].x,1,8,stdout);
		if(meth&1) x=size-x-1;
		if(meth&2) y=size-y-1;
	}
}

int main(int argc,char**argv) {
	int i;
	if (argc!=2 || !strcmp(argv[1],"-h") || !strcmp(argv[1],"--help")) {
		usage();
	}
	load_palette();
	num=strtol(argv[1],0,0);
	*buf=fgetc(stdin);
	i=*buf&15;
	fread(buf+1,1,i+(i>>1),stdin);
	for(i=0;i<=num;i++) load_picture(i);
	fwrite("farbfeld",1,8,stdout);
	putchar(0); putchar(0);
	putchar(size>>8); putchar(size);
	putchar(0); putchar(0);
	putchar(size>>8); putchar(size);
	out_picture();
	return 0;
}
