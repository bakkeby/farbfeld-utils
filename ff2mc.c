#if 0
gcc -s -O2 -o ~/bin/ff2mc -Wno-unused-result ff2mc.c
exit
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define fatal(...) do { fprintf(stderr,__VA_ARGS__); fputc('\n',stderr); exit(1); } while(0)

typedef struct {
	unsigned char d[8];
} Tile;

typedef union {
	Tile*til;
	unsigned short q[4];
} Node;

static unsigned char buf[16];
static int size,size8,square8,nxt,lev,lst;
static Tile*pic;
static Node node[65536];
static unsigned short*mpic;
static Node fnod;

static void
usage(void)
{
	fprintf(stderr,
		"Usage: ff2mc                                                                  \n"
		"                                                                              \n"
		"Convert a farbfeld image to Macrocell. ff2mc reads a farbfeld image from      \n"
		"stdin, converts it to farbfeld and writes the result to stdout.               \n"
		"                                                                              \n"
		"The Macrocell format is used for storing cellular automata. Its structure     \n"
		"mirrors that of the HashLife algorithm, and is accordingly efficient for      \n"
		"storing large, complex patterns.                                              \n"
		"                                                                              \n"
		"The format is used by the open source cross-platform application Golly for    \n"
		"exploring Conway's Game of Life and many other types of cellular automata.    \n"
		"                                                                              \n"
		"This encoder takes no arguments. The input picture size must be square, must  \n"
		"be a power of two, and must be at least 8x8. Only the high bit of the red     \n"
		"channel is used.                                                              \n"
		"                                                                              \n"
		"Example usage:                                                                \n"
		"   $ ff2mc < image.ff > image.mc                                              \n"
		"   $ bunzip2 < image.ff.bz2 | ff2mc > image.mc                                \n"
		"\n"
	);
	exit(1);
}

static void
read_picture(void)
{
	int w,x,y,z;
	Tile*p=pic;
	for (y=0; y<size8; y++) {
		for (z=0; z<8; z++) {
			for (x=0; x<size8; x++) {
				for (w=1; w!=256; w+=w) {
					fread(buf,1,8,stdin);
					if (*buf&128) p[x].d[z]|=w;
				}
			}
		}
		p+=size8;
	}
}

static void
out_small_block(const unsigned char *d)
{
	int i,j,k;
	for (i=0; i<8; i++) if (d[i]) j=i;
	for (i=0; i<=j; i++) {
		k=d[i];
		while (k) {
			putchar(k&1?'*':'.');
			k>>=1;
		}
		putchar('$');
	}
	putchar('\n');
}

static void
small_block(void)
{
	int i,j;
	memset(buf,0,8);
	for (i=0; i<square8; i++) {
		if (memcmp(buf,pic[i].d,8)) {
			for (j=1; j<nxt; j++) if (!memcmp(node[j].til->d,pic[i].d,8)) goto found;
			if (nxt==65536) fatal("Too many lines of output");
			node[j=nxt++].til=pic+i;
			out_small_block(pic[i].d);
			found:
			mpic[i]=j;
		}
	}
}

static void
mega_block(void)
{
	int xk=1<<(lev-4);
	int yk=size8<<(lev-4);
	int x,y,i;
	for (x=0; x<size8; x+=xk+xk) for (y=0; y<square8; y+=yk+yk) {
		fnod.q[0]=mpic[x+y];
		fnod.q[1]=mpic[x+y+xk];
		fnod.q[2]=mpic[x+y+yk];
		fnod.q[3]=mpic[x+y+xk+yk];
		if (fnod.q[0] || fnod.q[1] || fnod.q[2] || fnod.q[3]) {
			for (i=lst; i<nxt; i++) if (!memcmp(node[i].q,fnod.q,4*sizeof(unsigned short))) goto found;
			if (nxt==65536) fatal("Too many lines of output");
			node[i=nxt++]=fnod;
			printf("%d %d %d %d %d\n",lev,fnod.q[0],fnod.q[1],fnod.q[2],fnod.q[3]);
			found:
			mpic[x+y]=i;
		}
	}
}

int
main(int argc, char **argv)
{
	if (argc > 1)
		usage();
	fread(buf,1,16,stdin);
	if (memcmp(buf,"farbfeld",8)) fatal("Not farbfeld");
	if (memcmp(buf+8,buf+12,4)) fatal("Incorrect size");
	size=(buf[8]<<24)|(buf[9]<<16)|(buf[10]<<8)|buf[11];
	if (size<8 || ((size-1)&size)) fatal("Incorrect size");
	pic=calloc(size,size8=size>>3);
	mpic=calloc(size8,size8*sizeof(unsigned short));
	square8=size8*size8;
	if (!pic || !mpic) fatal("Allocation failed");
	read_picture();
	puts("[M2] (Farbfeld Utilities)");
	nxt=lst=1;
	small_block();
	free(pic);
	lev=4;
	while ((1<<lev)<=size) {
		lst=nxt;
		mega_block();
		lev++;
	}
	return 0;
}
