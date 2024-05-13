#if 0
gcc -s -O2 -o ~/bin/ff-probgen -Wno-unused-result ff-probgen.c
exit
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
	int opt[128];
} Config;

static int width,height;
static unsigned short*row;
static Config config[5];

#define fatal(...) do{ fprintf(stderr,__VA_ARGS__); fputc('\n',stderr); exit(1); }while(0)
#define wsize config->opt['w']

static void
usage(void)
{
	fprintf(stderr,
		"Usage: ff-probgen <width> <height> <global conf> <conf> <conf> <conf> <?conf?>\n"
		"                                                                              \n"
		"This farbfeld program that generates an image using a probabilistic random    \n"
		"generator.                                                                    \n"
		"                                                                              \n"
		"This program does not read from standard in.                                  \n"
		"                                                                              \n"
		"The filter takes six or seven arguments:                                      \n"
		"   - width of picture                                                         \n"
		"   - height of picture                                                        \n"
		"   - global configuration                                                     \n"
		"   - lowest configuration                                                     \n"
		"   - low configuration                                                        \n"
		"   - high configuration                                                       \n"
		"   - highest configuration (optional)                                         \n"
		"                                                                              \n"
		"Each configuration consists of alternating letters and the numeric value of   \n"
		"that field.                                                                   \n"
		"                                                                              \n"
		"Global configurations are:                                                    \n"
		"   s = random seed (default 42)                                               \n"
		"   w = window size (must be less than the width) (default 5)                  \n"
		"                                                                              \n"
		"Non-global configurations are:                                                \n"
		"   c = clip/wrap mode, where bit0 means negative wrapping, and bit1 means     \n"
		"       positive wrapping                                                      \n"
		"   d = division factor (fourths)                                              \n"
		"   j = minimum random value to add (out of 65535)                             \n"
		"   k = maximum random value to add (out of 65535)                             \n"
		"   m = multiplication factor (fourths)                                        \n"
		"   r = probability (out of 256) to make this pixel automatically blackened    \n"
		"                                                                              \n"
		"Note that many of these numbers can be negative or positive or zero, but some \n"
		"are not meaningful to be negative (and the division cannot be zero).          \n"
		"                                                                              \n"
		"Example usage:                                                                \n"
		"   $ ff-probgen 640 480 s91w5 c0d005j1000k10000m0.18r004 \\                   \n"
		"                              c0d010j1500k32000m0.21r010 \\                   \n"
		"                              c1d030j3000k54000m0.30r030 \\                   \n"
		"                              c1d050j4500k62000m0.45r080 > image-probgen.ff   \n"
		"\n"
	);
	exit(1);
}

static void load_config(int n,char*s) {
	int c,v;
	config[n].opt['c']=0;
	config[n].opt['d']=4;
	config[n].opt['h']=0;
	config[n].opt['j']=0;
	config[n].opt['k']=0;
	config[n].opt['m']=0;
	config[n].opt['r']=0;
	while(*s) {
		c=*s++;
		v=strtol(s,&s,10);
		config[n].opt[c&127]=v;
	}
}

static void process(void) {
	int x,i,v,c;
	int minv=65535;
	int maxv=0;
	int total=0;
	for(i=0;i<wsize;i++) {
		if(row[i]<minv) minv=row[i];
		if(row[i]>maxv) maxv=row[i];
		total+=row[i];
	}
	for(x=0;x<width;x++) {
		if(row[x]<=minv) c=1;
		else if(row[x]>=maxv) c=4;
		else if(row[x]<total/(2*wsize)) c=2;
		else c=3;
		v=row[(x+wsize)%width];
		total+=v-row[x];
		minv=65535;
		maxv=0;
		for(i=0;i<wsize;i++) {
			if(row[(i+x+wsize)%width]<minv) minv=row[(i+x+wsize)%width];
			if(row[(i+x+wsize)%width]>maxv) maxv=row[(i+x+wsize)%width];
		}
		v=(4*row[x])/config[c].opt['d']+random()%(config[c].opt['k']+1-config[c].opt['j'])+config[c].opt['j'];
		if(config[c].opt['m']) v=((config[c].opt['m']*total)/wsize+v*4)/(config[c].opt['m']+4);
		if(config[c].opt['r'] && (random()&255)<config[c].opt['r']) v=0;
		if(v<0) v=config[c].opt['c']&1?v&65535:0;
		if(v>65535) v=config[c].opt['c']&2?v&65535:65535;
		putchar(v>>8); putchar(v);
		putchar(v>>8); putchar(v);
		putchar(v>>8); putchar(v);
		putchar(255); putchar(255);
		row[x]=v;
	}
}

int main(int argc,char**argv) {
	if (argc>1 && (!strcmp(argv[1],"-h") || !strcmp(argv[1],"--help"))) {
		usage();
	}
	if(argc!=7 && argc!=8) fatal("Incorrect number of arguments");
	config->opt['s']=42;
	config->opt['w']=5;
	width=strtol(argv[1],0,0);
	height=strtol(argv[2],0,0);
	load_config(0,argv[3]);
	if(wsize>=width) fatal("Invalid window size");
	srandom(config->opt['s']);
	row=calloc(sizeof(unsigned short),width);
	if(!row) fatal("Allocation failed");
	while(random()&5) row[random()%width]++;
	load_config(1,argv[4]);
	load_config(2,argv[5]);
	if(argc==7) {
		load_config(3,argv[6]);
		load_config(4,argv[6]);
	} else {
		load_config(3,argv[6]);
		load_config(4,argv[7]);
	}
	fwrite("farbfeld",1,8,stdout);
	putchar(width>>24); putchar(width>>16); putchar(width>>8); putchar(width);
	putchar(height>>24); putchar(height>>16); putchar(height>>8); putchar(height);
	while(height--) process();
	return 0;
}
