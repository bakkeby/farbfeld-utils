#if 0
gcc -s -O2 -o ~/bin/ff-fern -Wno-unused-result ff-fern.c -lm
exit
#endif

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
	double a,b,c,d,e,f;
	int p;
	double x,y;
} Rule;

static int width,height;
static Rule rules[32];
static int nrules,step;
static double curx,cury;
static char opt[128];
static unsigned char*pic;
static double newx,newy;
static int ix,iy;
static int lastrule,lastrule2;
static char dist[32768];

static void
usage(void)
{
	fprintf(stderr,
		"Usage: ff-fern <width> <height> <x> <y> <steps> <options> <rule> ...          \n"
		"                                                                              \n"
		"This farbfeld program generates a picture of a fern or something else by chaos\n"
		"game.                                                                         \n"
		"                                                                              \n"
		"Normally there is no input picture, but it is possible to use input with the i\n"
		"option.                                                                       \n"
		"                                                                              \n"
		"This program uses coordinates where:                                          \n"
		"   - Y is -1 at the bottom and +1 at the top                                  \n"
		"   - X is -1 at the left and +1 at the right and                              \n"
		"   - the middle of the picture is zero                                        \n"
		"                                                                              \n"
		"The arguments are:                                                            \n"
		"   - width                                                                    \n"
		"   - height                                                                   \n"
		"   - initial X                                                                \n"
		"   - initial Y                                                                \n"
		"   - number of steps                                                          \n"
		"   - string of options; use - to indicate no options                          \n"
		"   - rules; one or more rules                                                 \n"
		"                                                                              \n"
		"Each rule consists of seven arguments: a b c d e f p                          \n"
		"                                                                              \n"
		"The first six are numbers that can include fractional parts, but p must be an \n"
		"integer from 0 to 32767, indicating probability weight.                       \n"
		"                                                                              \n"
		"The rules are based on the equations:                                         \n"
		"   x' = ax + by + c                                                           \n"
		"   y' = dx + ey + f                                                           \n"
		"                                                                              \n"
		"Valid options include:                                                        \n"
		"   b -- do not use a rule if it would result in moving to out of bounds       \n"
		"   c -- do not use a rule if the green channel is already at least 256        \n"
		"        (also causes any pixel that is hit to immediately be set to its       \n"
		"        maximum value, rather than increasing by one at a time)               \n"
		"   i -- read a input picture to use as the initial state; it must have the    \n"
		"        specified width and height; if not specified, all pixels are initially\n"
		"        opaque and black. When an input picture is used, it will not use any  \n"
		"        rule if it would cause it to land on a transparent pixel.             \n"
		"   k -- do not use the next rule as used previously to the previously used    \n"
		"        rule                                                                  \n"
		"   l -- do not use the same rule twice consecutively                          \n"
		"   m -- do not use the same rule as used previously to the previously used    \n"
		"        rule                                                                  \n"
		"   n -- do not use the next rule (considered the first after the last one)    \n"
		"        immediately after the one just used                                   \n"
		"   q -- if specified, decide which channels to affect based on which rule is  \n"
		"        being used                                                            \n"
		"   r -- if specified, affect only one channel; it affects red if it is not    \n"
		"        already maximum, or else green if it is not already maximum, or else  \n"
		"        blue if it is not already maximum, or else do nothing                 \n"
		"   s -- if specified, affect only one channel; which one is affected depends  \n"
		"        on which rule is selected                                             \n"
		"                                                                              \n"
		"If none if the q, r, s options are in use, then it affects red and green and  \n"
		"blue channels equally, resulting in a grey scale picture.                     \n"
		"                                                                              \n"
		"Example usage:                                                                \n"
		"   $ ff-fern 300 300 0 0 200 rbc \\                                           \n"
		"     -1.09 -0.01 2.5 -1.050 -0.0028 0.000000001 5000 > example.ff             \n"
		"\n"
	);
	exit(1);
}

static int increment(unsigned char*p) {
	if(opt['c']) {
		if(*p) return 0;
		*p=p[1]=255;
	} else if(p[1]==255) {
		if(*p==255) return 0;
		p[1]=0;
		++*p;
	} else {
		++p[1];
	}
	return 1;
}

static inline void do_step(void) {
	int i;
	int m=0;
	unsigned char*p;
	for(i=0;i<nrules;i++) {
		if(opt['l'] && i==lastrule) continue;
		if(opt['n'] && i==(lastrule+1)%nrules) continue;
		if(opt['m'] && i==lastrule2) continue;
		if(opt['k'] && i==(lastrule2+1)%nrules) continue;
		rules[i].x=newx=rules[i].a*curx+rules[i].b*cury+rules[i].e;
		rules[i].y=newy=rules[i].c*curx+rules[i].d*cury+rules[i].f;
		if(opt['b'] && (newx<=-1.0 || newx>=1.0 || newy<=-1.0 || newy>=1.0)) continue;
		ix=(1.0+newx)*0.5*width;
		iy=opt['y']?(1.0-newy)*height:(1.0-newy)*0.5*height;
		if(opt['b'] && (ix<0 || iy<0 || ix>=width || iy>=height)) continue;
		if(opt['i'] && ix>=0 && iy>=0 && ix<width && iy<height && !(pic[(ix+iy*width)*8+6]&128)) continue;
		if(opt['c'] && ix>=0 && iy>=0 && ix<width && iy<height && pic[(ix+iy*width)*8+2]) continue;
		if(rules[i].p+m>=32767) continue;
		memset(dist+m,i,rules[i].p);
		m+=rules[i].p;
	}
	if(!m) {
		step=0;
		return;
	}
	lastrule2=lastrule;
	lastrule=i=dist[random()%m];
	if(opt['u'] && lastrule2>=0) {
		newx=rules[lastrule2].e;
		newy=rules[lastrule2].f;
		rules[lastrule2].e=rules[i].e;
		rules[lastrule2].f=rules[i].f;
		rules[i].e=newx;
		rules[i].f=newy;
	}
	newx=curx=rules[i].x;
	newy=cury=rules[i].y;
	ix=(1.0+newx)*0.5*width;
	iy=opt['y']?(1.0-newy)*height:(1.0-newy)*0.5*height;
	if(ix<0 || iy<0 || ix>=width || iy>=height) return;
	p=pic+(ix+iy*width)*8;
	if(opt['r']) {
		increment(p) || increment(p+2) || increment(p+4);
	} else if(opt['s']) {
		increment(p+(i%3)*2);
	} else if(opt['q']) {
		i=(i&7)+1;
		if(i&4) increment(p);
		if(i&2) increment(p+2);
		if(i&1) increment(p+4);
	} else {
		increment(p);
		increment(p+2);
		increment(p+4);
	}
}

int main(int argc,char**argv) {
	int i;
	if (argc>1 && (!strcmp(argv[1],"-h") || !strcmp(argv[1],"--help"))) {
		usage();
	}
	if(argc<8 || argc%7 || argc>33*7) {
		fprintf(stderr,"Improper number of arguments (%d)\n", argc);
		return 1;
	}
	width=strtol(argv[1],0,0);
	height=strtol(argv[2],0,0);
	curx=strtod(argv[3],0);
	cury=strtod(argv[4],0);
	step=strtol(argv[5],0,0);
	srandom((width|height)+step);
	for(i=0;argv[6][i];i++) opt[argv[6][i]&127]=1;
	nrules=(argc-7)/7;
	for(i=0;i<nrules;i++) {
		rules[i].a=strtod(argv[i*7+7],0);
		rules[i].b=strtod(argv[i*7+8],0);
		rules[i].c=strtod(argv[i*7+9],0);
		rules[i].d=strtod(argv[i*7+10],0);
		rules[i].e=strtod(argv[i*7+11],0);
		rules[i].f=strtod(argv[i*7+12],0);
		rules[i].p=strtol(argv[i*7+13],0,0);
	}
	pic=calloc(8,width*height);
	if(!pic) {
		fprintf(stderr,"Allocation failed\n");
		return 1;
	}
	if(opt['i']) {
		fread(opt,1,16,stdin);
		fread(pic,8,width*height,stdin);
	} else {
		for(i=width*height-1;i>=0;i--) {
			pic[(i<<3)|6]=pic[(i<<3)|7]=255;
		}
	}
	lastrule=lastrule2=-1;
	while(step--) do_step();
	fwrite("farbfeld",1,8,stdout);
	putchar(width>>24);
	putchar(width>>16);
	putchar(width>>8);
	putchar(width);
	putchar(height>>24);
	putchar(height>>16);
	putchar(height>>8);
	putchar(height);
	fwrite(pic,8,width*height,stdout);
	return 0;
}
