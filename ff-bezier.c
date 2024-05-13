#if 0
gcc -s -O2 -o ~/bin/ff-bezier -Wno-unused-result ff-bezier.c -lm
exit
#endif

#define _POSIX_C_SOURCE 200112L
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned char buf[8];
static unsigned char color[8];
static int width,height,size;
static char*pic;
static double sol[3];

static void
usage(void)
{
	fprintf(stderr,
		"Usage: ff-bezier <colour> <x-pos> <y-pos> <x-pos> <y-pos> ...                 \n"
		"                                                                              \n"
		"This farbfeld filter draws one or more cubic Bezier curves.                   \n"
		"                                                                              \n"
		"ff-bezier draws one or more connected cubic Bezier curves onto an existing    \n"
		"picture.                                                                      \n"
		"                                                                              \n"
		"The first argument is the colour to draw. After that gives the coordinates of \n"
		"each point, with one argument for X coordinate, one argument for Y coordinate,\n"
		"and then again another X coordinate, another Y coordinate, etc.               \n"
		"                                                                              \n"
		"The coordinate can have decimal values.                                       \n"
		"                                                                              \n"
		"At least four points are needed. Any multiple of three points can be added    \n"
		"afterwards because the end points of each piece are shared, but the control   \n"
		"points are not shared.                                                        \n"
		"                                                                              \n"
		"The colour can be defined through one of the following patterns:              \n"
		"   rrggbb            three pairs of 8-bit hex values                          \n"
		"   rrggbbaa          four pairs of 8-bit hex values                           \n"
		"   rrrrggggbbbb      three pairs of 16-bit hex values                         \n"
		"   rrrrggggbbbbaaaa  four pairs of 16-bit hex values                          \n"
		"                                                                              \n"
		"Example usage:                                                                \n"
		"   $ ff-bezier FF9090 0 50 40 30 80 20 120 140 < image.ff > image-bezier.ff   \n"
		"\n"
	);
	exit(1);
}

static void solve(double a,double b,double c,double d) {
	// Hopefully there is a better way which doesn't require this
	sol[0]=sol[1]=sol[2]=1.0/0.0;
	if(a!=0.0) {
		double Delta=18.0*a*b*c*d-4.0*b*b*b*d+b*b*c*c-4.0*a*c*c*c-27.0*a*a*d*d;
		double Delta0=b*b-3.0*a*c;
		double Delta1=2.0*b*b*b-9.0*a*b*c+27.0*a*a*d;
		if(Delta==0.0 && Delta0==0.0) {
			sol[0]=b/(-3.0*a);
		} else if(Delta<=0.0) {
			double Cplus=cbrt(0.5*(Delta1+sqrt(-27.0*a*a*Delta)));
			double Cminus=cbrt(0.5*(Delta1-sqrt(-27.0*a*a*Delta)));
			sol[0]=(b+Cplus+Delta0/Cplus)/(-3.0*a);
			sol[1]=(b+Cminus+Delta0/Cminus)/(-3.0*a);
		} else {
			double p=Delta0/(-3.0*a*a);
			double q=Delta1/(27.0*a*a*a);
			double t0=2.0*sqrt(p/-3.0)*cos(acos((3.0*q*sqrt(-3.0/p))/(2.0*p))/3.0);
			double t2=-2.0*sqrt(p/-3.0)*cos(acos((-3.0*q*sqrt(-3.0/p))/(2.0*p))/3.0);
			double t1=-t0-t2;
			double tx=b/(-3.0*a);
			sol[0]=t0+tx;
			sol[1]=t1+tx;
			sol[2]=t2+tx;
		}
	} else if(b!=0.0) {
		double srd=sqrt(b*b-4.0*a*c);
		sol[0]=(-b+srd)/(2.0*a);
		sol[1]=(-b-srd)/(2.0*a);
	} else if(c==0.0 && d==0.0) {
		sol[0]=0.0;
	} else {
		sol[0]=-d/c;
	}
}

static inline void pset(int x,int y) {
	if(x<0 || y<0 || x>=width || y>=height) return;
	pic[y*width+x]=1;
}

static void do_curve(double x0,double y0,double x1,double y1,double x2,double y2,double x3,double y3) {
	int xmin=floor(fmin(fmin(x0,x1),fmin(x2,x3)));
	int xmax=ceil(fmax(fmax(x0,x1),fmax(x2,x3)));
	int ymin=floor(fmin(fmin(y0,y1),fmin(y2,y3)));
	int ymax=ceil(fmax(fmax(y0,y1),fmax(y2,y3)));
	int i,j;
	double a,b,c,t,u,z;
	if(xmin>=width || xmax<0 || ymin>=height || ymax<0) return;
	if(xmin<0) xmin=0;
	if(xmax>=width) xmax=width-1;
	if(ymin<0) ymin=0;
	if(ymax>=height) ymax=height-1;
	pset(x0,y0);
	pset(x3,y3);
	a=-x0+3.0*x1-3.0*x2+x3;
	b=3.0*(x0-2.0*x1+x2);
	c=3.0*(x1-x0);
	for(i=xmin;i<=xmax;i++) {
		solve(a,b,c,x0-i);
		for(j=0;j<3;j++) if(sol[j]>=0.0 && sol[j]<=1.0) {
			t=sol[j];
			u=1.0-sol[j];
			z=u*u*u*y0+3.0*t*u*u*y1+3.0*t*t*u*y2+t*t*t*y3;
			pset(i,floor(z));
			pset(i,ceil(z));
		}
	}
	a=-y0+3.0*y1-3.0*y2+y3;
	b=3.0*(y0-2.0*y1+y2);
	c=3.0*(y1-y0);
	for(i=ymin;i<=ymax;i++) {
		solve(a,b,c,y0-i);
		for(j=0;j<3;j++) if(sol[j]>=0.0 && sol[j]<=1.0) {
			t=sol[j];
			u=1.0-sol[j];
			z=u*u*u*x0+3.0*t*u*u*x1+3.0*t*t*u*x2+t*t*t*x3;
			pset(floor(z),i);
			pset(ceil(z),i);
		}
	}
}

int main(int argc,char**argv) {
	int i;
	if(argc%6!=4 || !strcmp(argv[1],"-h") || !strcmp(argv[1],"--help")) {
		usage();
	}
	i=strlen(argv[1]);
	if(i==6) {
		sscanf(argv[1],"%2hhX%2hhX%2hhX",color+0,color+2,color+4);
		color[1]=color[0];
		color[3]=color[2];
		color[5]=color[4];
		color[7]=color[6]=255;
	} else if(i==8) {
		sscanf(argv[1],"%2hhX%2hhX%2hhX%2hhX",color+0,color+2,color+4,color+6);
		color[1]=color[0];
		color[3]=color[2];
		color[5]=color[4];
		color[7]=color[6];
	} else if(i==12) {
		sscanf(argv[1],"%2hhX%2hhX%2hhX%2hhX%2hhX%2hhX",color+0,color+1,color+2,color+3,color+4,color+5);
		color[7]=color[6]=255;
	} else if(i==16) {
		sscanf(argv[1],"%2hhX%2hhX%2hhX%2hhX%2hhX%2hhX%2hhX%2hhX",color+0,color+1,color+2,color+3,color+4,color+5,color+6,color+7);
	} else {
		fprintf(stderr,"Improper color\n");
		return 1;
	}
	fread(buf,1,8,stdin);
	fwrite(buf,1,8,stdout);
	fread(buf,1,8,stdin);
	width=(buf[0]<<24)|(buf[1]<<16)|(buf[2]<<8)|buf[3];
	height=(buf[4]<<24)|(buf[5]<<16)|(buf[6]<<8)|buf[7];
	pic=calloc(width,height);
	if(!pic) {
		fprintf(stderr,"Allocation failed\n");
		return 1;
	}
	fwrite(buf,1,8,stdout);
	size=width*height;
	for(i=2;i<argc-3;i+=6) do_curve(
		strtod(argv[i+0],0),strtod(argv[i+1],0),strtod(argv[i+2],0),strtod(argv[i+3],0)
	 ,strtod(argv[i+4],0),strtod(argv[i+5],0),strtod(argv[i+6],0),strtod(argv[i+7],0)
	);
	for(i=0;i<size;i++) {
		fread(buf,1,8,stdin);
		fwrite(pic[i]?color:buf,1,8,stdout);
	}
	return 0;
}
