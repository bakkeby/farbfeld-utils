#if 0
gcc -s -O2 -o ~/bin/mc2ff -Wno-unused-result mc2ff.c
exit
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define fatal(...) do { fprintf(stderr,__VA_ARGS__); fputc('\n',stderr); exit(1); } while(0)

typedef struct {
	signed short size;
	union {
		unsigned char data[8];
		unsigned short next[4];
	};
} Node;

static unsigned char buf[8];
static Node*node;
static int nnode,nallocnode;

static void
usage(void)
{
	fprintf(stderr,
		"Usage: mc2ff                                                                  \n"
		"                                                                              \n"
		"Convert Macrocell image to farbfeld. mc2ff reads a Macrocell image from stdin,\n"
		"converts it to farbfeld and writes the result to stdout.                      \n"
		"                                                                              \n"
		"The Macrocell format is used for storing cellular automata. Its structure     \n"
		"mirrors that of the HashLife algorithm, and is accordingly efficient for      \n"
		"storing large, complex patterns.                                              \n"
		"                                                                              \n"
		"The format is used by the open source cross-platform application Golly for    \n"
		"exploring Conway's Game of Life and many other types of cellular automata.    \n"
		"                                                                              \n"
		"Example usage:                                                                \n"
		"   $ mc2ff < image.mc > image.ff                                              \n"
		"   $ mc2ff < image.mc | bzip2 > image.ff.bz2                                  \n"
		"\n"
	);
	exit(1);
}

static Node *
new_node(void)
{
	Node*p;
	if (nallocnode<++nnode) {
		nallocnode+=32;
		node=realloc(node,nallocnode*sizeof(Node));
		if (!node) fatal("Allocation failed (%d nodes)",nallocnode);
	}
	if (nnode>=0x10000) fatal("Too many nodes");
	p=node+nnode-1;
	p->data[0]=p->data[1]=p->data[2]=p->data[3]=0;
	p->data[4]=p->data[5]=p->data[6]=p->data[7]=0;
	return p;
}

static int
input_line(void)
{
	Node*n;
	int i=fgetc(stdin);
	int x,y;
	if (i==EOF) {
		return 0;
	} else if (i=='#' || i=='\r' || i=='\n') {
		while (i!=EOF && i!='\n') i=fgetc(stdin);
		return 1;
	} else if (i=='$' || i=='.' || i=='*') {
		n=new_node();
		n->size=3;
		for (x=y=0; ; i=fgetc(stdin)) {
			if (i=='$') {
				x=0,++y;
			} else if (i=='.' || i=='*') {
				if (y&~7) fatal("Improper leaf node");
				if (i=='*') n->data[y]|=1<<x;
				x++;
			} else {
				break;
			}
		}
		while (i==' ') i=fgetc(stdin);
		if (i=='\r' || i=='\n' || i==EOF) return 1;
		fatal("Invalid character");
	} else {
		n=new_node();
		ungetc(i,stdin);
		if (scanf("%hu %hu %hu %hu %hu ",&n->size,n->next+0,n->next+1,n->next+2,n->next+3)!=5) fatal("Invalid input");
		return 1;
	}
}

static inline void
make_header(void)
{
	int n=1<<node[nnode-1].size;
	fwrite("farbfeld",1,8,stdout);
	putchar(n>>24); putchar(n>>16); putchar(n>>8); putchar(n);
	putchar(n>>24); putchar(n>>16); putchar(n>>8); putchar(n);
}

static void
out_node(int y,int n,int z)
{
	if (z==8) {
		int x,v;
		v=node[n].data[y&7];
		for (x=0; x<8; x++) {
			buf[0]=buf[1]=buf[2]=buf[3]=buf[4]=buf[5]=v&(1<<x)?255:0;
			fwrite(buf,1,8,stdout);
		}
	} else {
		int i;
		z>>=1;
		i=y&z?2:0;
		out_node(y,node[n].next[i],z);
		out_node(y,node[n].next[i+1],z);
	}
}

static inline void
make_picture(void)
{
	int z=1<<node[nnode-1].size;
	int y;
	for(y=0;y<z;y++) out_node(y,nnode-1,z);
}

int
main(int argc, char **argv)
{
	if (argc > 1)
		usage();
	fread(buf,1,4,stdin);
	if (memcmp(buf,"[M2]",4)) fatal("Improper file format");
	node=malloc((nallocnode=32)*sizeof(Node));
	if (!node) fatal("Allocation failed");
	do {
		if (fread(buf,1,1,stdin)<=0) fatal("Cannot read input");
	} while(*buf!='\n');
	new_node(); // The empty node
	while (input_line());
	make_header();
	buf[6]=buf[7]=255;
	make_picture();
	return 0;
}
