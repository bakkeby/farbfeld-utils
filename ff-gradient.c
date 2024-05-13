#if 0
gcc -s -O2 -o ~/bin/ff-gradient -D_DEFAULT_SOURCE -Wno-unused-result ff-gradient.c -lm
exit
#endif

#define _BSD_SOURCE 1
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
	unsigned short r;
	unsigned short g;
	unsigned short b;
	unsigned short a;
} Color;

static int width;
static int height;
static Color shade[18];
static int count;
static int iparam;
static double dparam;

static void
usage(void)
{
	fprintf(stderr,
		"Usage: ff-gradient <width> <height> <mode> <colour> <?colour?> ...            \n"
		"                                                                              \n"
		"This farbfeld program generates a gradient picture.                           \n"
		"                                                                              \n"
		"The filter takes four or more arguments:                                      \n"
		"   - width                                                                    \n"
		"   - height                                                                   \n"
		"   - mode                                                                     \n"
		"   - one or more colours that make up the gradient                            \n"
		"                                                                              \n"
		"The colours can be defined through one of the following patterns:             \n"
		"   rrggbb            three pairs of 8-bit hex values                          \n"
		"   rrggbbaa          four pairs of 8-bit hex values                           \n"
		"   rrrrggggbbbb      three pairs of 16-bit hex values                         \n"
		"   rrrrggggbbbbaaaa  four pairs of 16-bit hex values                          \n"
		"                                                                              \n"
		"The modes available are:                                                      \n"
		"   a  = determine the colour to use according to the angle relative to the    \n"
		"        centre                                                                \n"
		"   d  = diagonal gradient                                                     \n"
		"   f  = a combination horizontal and vertical gradient - the last colour forms\n"
		"        a gradient with the first colour in the leftmost column, and then that\n"
		"        forms a horizontal gradient with the rest of the colours              \n"
		"   h  = horizontal gradient                                                   \n"
		"   m* = the gradient is based on the product of the X and Y coordinates - the \n"
		"        optional number is a bias amount                                      \n"
		"   n* = random (uniform) noise - the optional number is the random number seed\n"
		"   q  = make a horziontal gradient using red and green channels, and a        \n"
		"        vertical gradient using blue and alpha channels                       \n"
		"        (to mix the channels differently, follow this by ff-matrix)           \n"
		"   r  = the gradient is based on the distance from the centre                 \n"
		"        (forming concentric circles)                                          \n"
		"   v  = vertical gradient                                                     \n"
		"   y* = swirly lines emanating from the middle of the picture - the optional  \n"
		"        number specifies how swirly it is (and need not be an integer)        \n"
		"                                                                              \n"
		"(* means you can optionally add a number after the mode letter, with no space \n"
		" or other delimiter)                                                          \n"
		"                                                                              \n"
		"Example usage:                                                                \n"
		"   $ ff-gradient 640 480 y9.23 906060 609060 606090 > gradient-swirl.ff       \n"
		"\n"
	);
	exit(1);
}

static Color parse_color(const char*s) {
	Color c;
	switch(strlen(s)) {
		case 6:
			sscanf(s,"%2hx%2hx%2hx",&c.r,&c.g,&c.b);
			c.r*=0x101;
			c.g*=0x101;
			c.b*=0x101;
			c.a=0xFFFF;
			break;
		case 8:
			sscanf(s,"%2hx%2hx%2hx%2hx",&c.r,&c.g,&c.b,&c.a);
			c.r*=0x101;
			c.g*=0x101;
			c.b*=0x101;
			c.a*=0x101;
			break;
		case 12:
			sscanf(s,"%4hx%4hx%4hx",&c.r,&c.g,&c.b);
			c.a=0xFFFF;
			break;
		case 16:
			sscanf(s,"%4hx%4hx%4hx%4hx",&c.r,&c.g,&c.b,&c.a);
			break;
	}
	return c;
}

static void calc_gradient(double g) {
	Color c,s1,s2;
	if(count) {
		g*=(double)count;
		s1=shade[1+(int)g];
		s2=shade[(int)g];
		if(s1.r==s2.r) c.r=s1.r; else c.r=(g-floor(g))*s1.r+(floor(g)+1.0-g)*s2.r;
		if(s1.g==s2.g) c.g=s1.g; else c.g=(g-floor(g))*s1.g+(floor(g)+1.0-g)*s2.g;
		if(s1.b==s2.b) c.b=s1.b; else c.b=(g-floor(g))*s1.b+(floor(g)+1.0-g)*s2.b;
		if(s1.a==s2.a) c.a=s1.a; else c.a=(g-floor(g))*s1.a+(floor(g)+1.0-g)*s2.a;
	} else {
		c=*shade;
	}
	putchar(c.r>>8);
	putchar(c.r);
	putchar(c.g>>8);
	putchar(c.g);
	putchar(c.b>>8);
	putchar(c.b);
	putchar(c.a>>8);
	putchar(c.a);
}

static void calc_gradient_2(double g1,double g2) {
	Color c,s1,s2;
	if(count) {
		g1*=(double)count;
		s1=shade[1+(int)g1];
		s2=shade[(int)g1];
		if(s1.r==s2.r) c.r=s1.r; else c.r=(g1-floor(g1))*s1.r+(floor(g1)+1.0-g1)*s2.r;
		if(s1.g==s2.g) c.g=s1.g; else c.g=(g1-floor(g1))*s1.g+(floor(g1)+1.0-g1)*s2.g;
		g2*=(double)count;
		s1=shade[1+(int)g2];
		s2=shade[(int)g2];
		if(s1.b==s2.b) c.b=s1.b; else c.b=(g2-floor(g2))*s1.b+(floor(g2)+1.0-g2)*s2.b;
		if(s1.a==s2.a) c.a=s1.a; else c.a=(g2-floor(g2))*s1.a+(floor(g2)+1.0-g2)*s2.a;
	} else {
		c=*shade;
	}
	putchar(c.r>>8);
	putchar(c.r);
	putchar(c.g>>8);
	putchar(c.g);
	putchar(c.b>>8);
	putchar(c.b);
	putchar(c.a>>8);
	putchar(c.a);
}

static void do_a_gradient(void) {
	int x,y;
	shade[++count]=*shade;
	for(y=0;y<height;y++) for(x=0;x<width;x++) calc_gradient((atan2(height/2-y,x-width/2)+M_PI)/(2.0*M_PI));
}

static void do_d_gradient(void) {
	int x,y;
	for(y=0;y<height;y++) for(x=0;x<width;x++) calc_gradient(((double)(x+y))/(double)(width+height-1));
}

static void do_f_gradient(void) {
	int x,y;
	double f;
	Color c=*shade;
	if(count) --count;
	for(y=0;y<height;y++) {
		f=((double)y)/(double)(height-1);
		if(c.r!=shade[count+1].r) shade->r=f*c.r+(1.0-f)*shade[count+1].r;
		if(c.g!=shade[count+1].g) shade->g=f*c.g+(1.0-f)*shade[count+1].g;
		if(c.b!=shade[count+1].b) shade->b=f*c.b+(1.0-f)*shade[count+1].b;
		if(c.a!=shade[count+1].a) shade->a=f*c.a+(1.0-f)*shade[count+1].a;
		for(x=0;x<width;x++) calc_gradient(((double)x)/(double)(width-1));
	}
}

static void do_h_gradient(void) {
	int x,y;
	for(y=0;y<height;y++) for(x=0;x<width;x++) calc_gradient(((double)x)/(double)(width-1));
}

static void do_m_gradient(void) {
	int x,y;
	for(y=1;y<=height;y++) for(x=1;x<=width;x++) calc_gradient(((double)(x*y+iparam))/(double)(width*height+iparam));
}

static void do_n_gradient(void) {
	int x,y;
	for(y=1;y<=height;y++) for(x=1;x<=width;x++) calc_gradient(((double)random())/(double)RAND_MAX);
}

static void do_q_gradient(void) {
	int x,y;
	for(y=0;y<height;y++) for(x=0;x<width;x++) calc_gradient_2(((double)x)/(double)(width-1),((double)y)/(double)(height-1));
}

static void do_r_gradient(void) {
	int x,y;
	for(y=0;y<height;y++) for(x=0;x<width;x++) calc_gradient(hypot(x-0.5*width,y-0.5*height)/hypot(0.5*width,0.5*height));
}

static void do_v_gradient(void) {
	int x,y;
	for(y=0;y<height;y++) for(x=0;x<width;x++) calc_gradient(((double)y)/(double)(height-1));
}

static void do_y_gradient(void) {
	int x,y;
	shade[++count]=*shade;
	for(y=0;y<height;y++) for(x=0;x<width;x++) calc_gradient(fmod((atan2(height/2-y,x-width/2)+M_PI)/(2.0*M_PI)+(dparam*hypot(x-0.5*width,y-0.5*height))/hypot(width,height),1.0));
}

int main(int argc,char**argv) {
	int i;
	if(argc<5 || !strcmp(argv[1],"-h") || !strcmp(argv[1],"--help")) {
		usage();
	} else if(argc>20) {
		fprintf(stderr,"Too many arguments\n");
		return 1;
	}
	width=strtol(argv[1],0,0);
	height=strtol(argv[2],0,0);
	count=argc-4;
	for(i=0;i<count;i++) shade[i]=parse_color(argv[i+4]);
	fwrite("farbfeld",1,8,stdout);
	putchar(width>>24);
	putchar(width>>16);
	putchar(width>>8);
	putchar(width);
	putchar(height>>24);
	putchar(height>>16);
	putchar(height>>8);
	putchar(height);
	--count;
	switch(argv[3][0]) {
		case 'a':
			do_a_gradient();
			break;
		case 'd':
			do_d_gradient();
			break;
		case 'f':
			do_f_gradient();
			break;
		case 'h':
			do_h_gradient();
			break;
		case 'm':
			if(argv[3][1]) iparam=strtol(argv[3]+1,0,0);
			do_m_gradient();
			break;
		case 'n':
			if(argv[3][1]) srandom(strtol(argv[3]+1,0,0));
			do_n_gradient();
			break;
		case 'q':
			do_q_gradient();
			break;
		case 'r':
			do_r_gradient();
			break;
		case 'v':
			do_v_gradient();
			break;
		case 'y':
			if(argv[3][1]) dparam=strtod(argv[3]+1,0); else dparam=1.0;
			do_y_gradient();
			break;
		default:
			fprintf(stderr,"Unknown mode: %c\n",argv[3][0]);
			return 1;
	}
	return 0;
}
