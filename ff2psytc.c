#if 0
gcc -s -O2 -o ~/bin/ff2psytc -Wno-unused-result ff2psytc.c
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
	unsigned long n[16];
	char i[16];
	char h[16];
} Matrix;

static unsigned char buf[16];
static Color pal[16];
static int width,height,npal,ntile,tilesize;
static Matrix matrix[16];
static int opt[128];
static char*pic;
static char*info;
static int bitp,bitv;
static int curmat;

#define theight opt['h']

static void
usage(void)
{
	fprintf(stderr,
		"Usage: ff2psytc <?opt?> ... <?opt?> + <?palette?>                             \n"
		"                                                                              \n"
		"Converts farbfeld to Psycopathicteen Tile Compressor graphics. ffp2sytc reads \n"
		"a farbfeld file from stdin, converts it to psytc and writes the result to     \n"
		"stdout.                                                                       \n"
		"                                                                              \n"
		"Psycopathicteen Tile Compressor is a experimental format for compressing 4    \n"
		"bits per pixel indexed colours picture (such as using on Super Nintendo).     \n"
		"                                                                              \n"
		"This encoder expects the input as a vertical strip; use ff-strip to change it \n"
		"if this is not the case.                                                      \n"
		"                                                                              \n"
		"Optional arguments are given a sequence of options, and also optionally the   \n"
		"palette; the palette is preceded by + as an argument by itself, and then the  \n"
		"remaining arguments are each colours for the palette.                         \n"
		"                                                                              \n"
		"Options:                                                                      \n"
		"   a -- Force the compression mode; 1 means horizontal compression, 2         \n"
		"        means vertical compression, and 3 means uncompressed                  \n"
		"   h -- Tile height; 8 by default (tile width is equal to the width of the    \n"
		"        input picture)                                                        \n"
		"   p -- Set to 1 to include the palette in the output file                    \n"
		"   r -- Number of times to repeat trying to figure out the best compression   \n"
		"        mode. The default is zero, meaning after the initial attempt, do not  \n"
		"        try again.                                                            \n"
		"   v -- If nonzero, display diagnostic messages on stderr. This is a bit field\n"
		"        and different bits display different diagnostic messages.             \n"
		"   x -- Weight for preferring horizontal compression (default 1)              \n"
		"   y -- Weight for preferring vertical compression (default 1)                \n"
		"   + -- Set palette next (must be the last argument)                          \n"
		"                                                                              \n"
		"The palette consists of 16 colours, where each colour can have one of these   \n"
		"patterns:                                                                     \n"
		"   rrggbb            three pairs of 8-bit hex values                          \n"
		"   rrggbbaa          four pairs of 8-bit hex values                           \n"
		"   rrrrggggbbbb      three pairs of 16-bit hex values                         \n"
		"   rrrrggggbbbbaaaa  four pairs of 16-bit hex values                          \n"
		"                                                                              \n"
		"Example usage:                                                                \n"
		"   $ ffp2sytc a3 h8 p1 + 9A4040 448023 ...  < image.ff > image.psytc          \n"
		"   $ ffp2sytc r3 < image.ff > image.psytc                                     \n"
		"\n"
	);
	exit(1);
}

static void set_palette(char**argv,int argc) {
	int i,j;
	if((npal=argc)>16) {
		fprintf(stderr,"Too many colors\n");
		exit(1);
	}
	for(i=0;i<argc;i++) {
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
		} else {
			fprintf(stderr,"Invalid color\n");
			exit(1);
		}
	}
}

static void read_picture(void) {
	int i,j;
	int s=width*height;
	for(i=0;i<s;i++) {
		fread(buf,1,8,stdin);
		for(j=0;j<npal;j++) if(!memcmp(pal[j].d,buf,8)) goto found;
		if(npal==16) {
			fprintf(stderr,"Too many colors\n");
			exit(1);
		}
		memcpy(pal[j=npal++].d,buf,8);
		found:
		pic[i]=j;
	}
}

static int matrix_compare(const void*a,const void*b) {
	const char*x=a;
	const char*y=b;
	signed long i;
	if(*x==curmat) return -1;
	if(*y==curmat) return 1;
	i=matrix[curmat].n[*x]-matrix[curmat].n[*y];
	return i>0?-1:i<0?1:0;
}

static int plain_compare(const void*a,const void*b) {
	const char*x=a;
	const char*y=b;
	return *y-*x;
}

static void calc_adjacent_1(void) {
	int i,x,y,z;
	unsigned char*p;
	for(i=0,p=pic;i<ntile;i++,p+=tilesize) {
		for(y=0;y<theight;y++) for(x=0;x<width;x++) {
			if(x || y) matrix[z].n[p[y*width+x]]+=opt['x'];
			z=p[y*width+x];
		}
		for(x=0;x<width;x++) for(y=0;y<theight;y++) {
			if(x || y) matrix[z].n[p[y*width+x]]+=opt['y'];
			z=p[y*width+x];
		}
	}
	for(x=0;x<16;x++) {
		for(y=0;y<16;y++) matrix[x].i[y]=y;
		curmat=x;
		qsort(matrix[x].i,16,1,matrix_compare);
		for(y=0;y<16;y++) matrix[x].h[matrix[x].i[y]]=y;
		if(opt['v']&2) {
			fprintf(stderr,"%2d - ",x);
			for(y=0;y<16;y++) fprintf(stderr,"%2d ",matrix[x].h[y]);
			fputc('\n',stderr);
		}
	}
}

static void calc_adjacent_2(void) {
	int i,x,y,z;
	unsigned char*p;
	for(x=0;x<16;x++) for(y=0;y<16;y++) matrix[x].i[y]=0;
	for(i=0,p=pic;i<ntile;i++,p+=tilesize) {
		if(info[i]==1) for(y=0;y<theight;y++) for(x=0;x<width;x++) {
			if(x || y) matrix[z].n[p[y*width+x]]+=opt['x'];
			z=p[y*width+x];
		}
		if(info[i]==2) for(x=0;x<width;x++) for(y=0;y<theight;y++) {
			if(x || y) matrix[z].n[p[y*width+x]]+=opt['y'];
			z=p[y*width+x];
		}
	}
	for(x=0;x<16;x++) {
		for(y=0;y<16;y++) matrix[x].i[y]=y;
		curmat=x;
		qsort(matrix[x].i,16,1,matrix_compare);
		qsort(matrix[x].i+6,10,1,plain_compare);
		for(y=0;y<16;y++) matrix[x].h[matrix[x].i[y]]=y;
		if(opt['v']&2) {
			fprintf(stderr,"%2d - ",x);
			for(y=0;y<16;y++) fprintf(stderr,"%2d ",matrix[x].h[y]);
			fputc('\n',stderr);
		}
	}
}

static void figure_compression_mode(void) {
	int i,x,y,z,n;
	// Uncompressed size: (4*width*theight+2) bits
	unsigned long total=0;
	int ch,cv;
	unsigned char*p;
	for(i=0,p=pic;i<ntile;i++,p+=tilesize) {
		if(opt['a']) {
			info[i]=opt['a'];
			continue;
		}
		ch=cv=6;
		for(y=0;y<theight;y++) for(x=0;x<width;x++) {
			if(x || y) {
				if(p[y*width+x]==z) {
					n++;
				} else {
					z=matrix[z].h[p[y*width+x]];
					ch+=z>5?5:z>2?4:2;
					if(n>2) {
						ch+=6; n-=3;
						while(n>=15) ch+=4,n-=15;
					} else {
						ch+=n;
					}
					z=p[y*width+x];
					n=1;
				}
			} else {
				n=1;
				z=p[y*width+x];
			}
		}
		if(n>2) {
			ch+=6; n-=3;
			while(n>=15) ch+=4,n-=15;
		} else {
			ch+=n;
		}
		for(x=0;x<width;x++) for(y=0;y<theight;y++) {
			if(x || y) {
				if(p[y*width+x]==z) {
					n++;
				} else {
					z=matrix[z].h[p[y*width+x]];
					cv+=z>5?5:z>2?4:2;
					if(n>2) {
						cv+=6; n-=3;
						while(n>=15) cv+=4,n-=15;
					} else {
						cv+=n;
					}
					z=p[y*width+x];
					n=1;
				}
			} else {
				n=1;
				z=p[y*width+x];
			}
		}
		if(n>2) {
			cv+=6; n-=3;
			while(n>=15) cv+=4,n-=15;
		} else {
			cv+=n;
		}
		n=4*width*theight+2;
		if(opt['v']&1) fprintf(stderr,"[%d] %d %d %d\n",i,ch,cv,n);
		info[i]=n<ch?(cv<n?2:3):(ch<cv?1:2);
		total+=info[i]==1?ch:info[i]==2?cv:n;
		if(opt['v']&8) fputc(".HVu"[info[i]],stderr);
	}
	if(opt['v']&4) fprintf(stderr,"** %lu **\n",total);
}

static void emitbit(int n) {
	bitv|=n<<bitp;
	if(++bitp==8) {
		putchar(bitv);
		bitp=bitv=0;
	}
}

static void make_header(void) {
	int i,j;
	putchar(width|(opt['p']?128:0));
	putchar(theight);
	if(opt['p']) {
		for(j=0;j<16;j++) if(!pal[j].d[6]) break;
		putchar(j);
		for(i=0;i<16;i++) if(i!=j) {
			putchar(pal[i].d[0]);
			putchar(pal[i].d[2]);
			putchar(pal[i].d[4]);
		}
	}
	for(i=0;i<16;i++) for(j=1;j<6;j++) {
		emitbit((matrix[i].i[j]>>0)&1);
		emitbit((matrix[i].i[j]>>1)&1);
		emitbit((matrix[i].i[j]>>2)&1);
		emitbit((matrix[i].i[j]>>3)&1);
	}
}

static void output_h_compressed_picture(void) {
	int n,x,y,z;
	z=*pic;
	n=1;
	emitbit((z>>0)&1); emitbit((z>>1)&1);
	emitbit((z>>2)&1); emitbit((z>>3)&1);
	for(y=0;y<theight;y++) for(x=0;x<width;x++) if(x || y) {
		if(pic[y*width+x]==z) {
			n++;
		} else {
			z=matrix[z].h[pic[y*width+x]];
			if(n>2) {
				emitbit(1); emitbit(1);
				n-=3;
				while(n>=15) {
					emitbit(1); emitbit(1); emitbit(1); emitbit(1);
					n-=15;
				}
				emitbit((n>>0)&1); emitbit((n>>1)&1); emitbit((n>>2)&1); emitbit((n>>3)&1);
			} else {
				if(n==2) emitbit(1);
				emitbit(0);
			}
			switch(z) {
				case 1 ... 2: emitbit(0); emitbit(z-1); break;
				case 3 ... 5: emitbit(1); emitbit(0); emitbit(((z-3)>>1)&1); emitbit((z-3)&1); break;
				default: emitbit(1); emitbit((z>>3)&1); emitbit((z>>2)&1); emitbit((z>>1)&1); emitbit(z&1);
			}
			z=pic[y*width+x];
			n=1;
		}
	}
	if(n>2) {
		emitbit(1); emitbit(1);
		n-=3;
		while(n>=15) {
			emitbit(1); emitbit(1); emitbit(1); emitbit(1);
			n-=15;
		}
		emitbit((n>>0)&1); emitbit((n>>1)&1); emitbit((n>>2)&1); emitbit((n>>3)&1);
	} else {
		if(n==2) emitbit(1);
		emitbit(0);
	}
}

static void output_v_compressed_picture(void) {
	int n,x,y,z;
	z=*pic;
	n=1;
	emitbit((z>>0)&1); emitbit((z>>1)&1);
	emitbit((z>>2)&1); emitbit((z>>3)&1);
	for(x=0;x<width;x++) for(y=0;y<theight;y++) if(x || y) {
		if(pic[y*width+x]==z) {
			n++;
		} else {
			z=matrix[z].h[pic[y*width+x]];
			if(n>2) {
				emitbit(1); emitbit(1);
				n-=3;
				while(n>=15) {
					emitbit(1); emitbit(1); emitbit(1); emitbit(1);
					n-=15;
				}
				emitbit((n>>0)&1); emitbit((n>>1)&1); emitbit((n>>2)&1); emitbit((n>>3)&1);
			} else {
				if(n==2) emitbit(1);
				emitbit(0);
			}
			switch(z) {
				case 1 ... 2: emitbit(0); emitbit(z-1); break;
				case 3 ... 5: emitbit(1); emitbit(0); emitbit(((z-3)>>1)&1); emitbit((z-3)&1); break;
				default: emitbit(1); emitbit((z>>3)&1); emitbit((z>>2)&1); emitbit((z>>1)&1); emitbit(z&1);
			}
			z=pic[y*width+x];
			n=1;
		}
	}
	if(n>2) {
		emitbit(1); emitbit(1);
		n-=3;
		while(n>=15) {
			emitbit(1); emitbit(1); emitbit(1); emitbit(1);
			n-=15;
		}
		emitbit((n>>0)&1); emitbit((n>>1)&1); emitbit((n>>2)&1); emitbit((n>>3)&1);
	} else {
		if(n==2) emitbit(1);
		emitbit(0);
	}
}

static void output_uncompressed_picture(void) {
	int i,z;
	for(i=0;i<tilesize;i++) {
		z=pic[i];
		emitbit((z>>0)&1); emitbit((z>>1)&1);
		emitbit((z>>2)&1); emitbit((z>>3)&1);
	}
}

int main(int argc, char **argv) {
	int i;
	theight=8;
	opt['x']=opt['y']=1;
	for(i=1;i<argc;i++) {
		if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help")) {
			usage();
		}
		if(argv[i][0]=='+') {
			set_palette(argv+i+1,argc-i-1);
			break;
		} else if(argv[i][0]) {
			opt[argv[i][0]&127]=strtol(argv[i]+1,0,0);
		}
	}
	fread(buf,1,16,stdin);
	if(memcmp(buf,"farbfeld",8)) {
		fprintf(stderr,"Not farbfeld\n");
		return 1;
	}
	width=(buf[8]<<24)|(buf[9]<<16)|(buf[10]<<8)|buf[11];
	height=(buf[12]<<24)|(buf[13]<<16)|(buf[14]<<8)|buf[15];
	if(!width || !theight || (width&~127) || (theight&~127)) {
		fprintf(stderr,"Invalid tile size\n");
		return 1;
	}
	if(height%theight) {
		fprintf(stderr,"Invalid height\n");
		return 1;
	}
	ntile=height/theight;
	pic=malloc(width*height);
	info=malloc(ntile);
	if(!pic || !info) {
		fprintf(stderr,"Allocation failed\n");
		return 1;
	}
	tilesize=width*theight;
	read_picture();
	calc_adjacent_1();
	figure_compression_mode();
	opt['v']>>=8;
	calc_adjacent_2();
	figure_compression_mode();
	while(opt['r']--) {
		calc_adjacent_2();
		figure_compression_mode();
	}
	make_header();
	for(i=0;i<ntile;i++) {
		emitbit(info[i]&1); emitbit(info[i]>>1);
		switch(info[i]) {
			case 1: output_h_compressed_picture(); break;
			case 2: output_v_compressed_picture(); break;
			case 3: output_uncompressed_picture(); break;
		}
		pic+=tilesize;
	}
	emitbit(0); emitbit(0);
	if(bitp) putchar(bitv);
	return 0;
}
