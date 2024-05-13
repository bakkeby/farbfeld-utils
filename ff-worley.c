#if 0
gcc -s -O2 -o ~/bin/ff-worley -Wno-unused-result ff-worley.c -lm
exit
#endif

#define TAU (6.283185307179586476925286766559005768394338798750211641949889184615632812572417997256069650684234135964296173026564613294187689219101164463450718816256)

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum {
	P_metric,
	P_points,
	P_color,
	P_alpha,
	countP
};

typedef struct {
	int x,y;
} Point;

typedef struct {
	double dist;
	int id;
} Nearest;

static unsigned char buf[16];
static unsigned char*pic;
static int width,height;
static char mode[countP];
static int para[countP][16];
static int npara[countP];
static int dist1,dist2,isfuzz;
static double distfuzz;
static Point*points;
static int npoint;
static Nearest*near;
static int nnear;
static int*band;

static void
usage(void)
{
	fprintf(stderr,
		"Usage: ff-worley <seed> <points> <metric> <arrangement> <colour> <alpha>      \n"
		"                                                                              \n"
		"This farbfeld filter is used to apply partition by Worley/Voronoi noise.      \n"
		"                                                                              \n"
		"The filter takes exactly six arguments.                                       \n"
		"                                                                              \n"
		"The first argument is the random number seed.                                 \n"
		"                                                                              \n"
		"The second argument is one to three numbers with commas in between:           \n"
		"   - how many closest points to consider; the farthest of them is used        \n"
		"   - as above, but a secondary number (used in some cases)                    \n"
		"   - the amount of noise to add to the distance (fractions are allowed)       \n"
		"                                                                              \n"
		"The third argument is the metric to use:                                      \n"
		"   a --                                                                       \n"
		"   b -- biased manhattan; horizontal distances are multiplied by the parameter\n"
		"        to define the bias amount                                             \n"
		"   c -- chebyshev                                                             \n"
		"   d -- diagonal                                                              \n"
		"   e -- euclidean - optionally specify parameters which are the amount to add \n"
		"        to the horizontal distance, and the amount to add to the vertical     \n"
		"        distance, before making the calculation (e.g. e,4,5)                  \n"
		"   m -- manhattan                                                             \n"
		"   p -- product - the optional parameter is used for biasing                  \n"
		"   r -- random                                                                \n"
		"                                                                              \n"
		"The fourth argument is the method of arranging the Voronoi points:            \n"
		"   b -- make a grid, taking four parameters:                                  \n"
		"        - cell width (horizontal cell size)                                   \n"
		"        - cell height (vertical cell size)                                    \n"
		"        - the minimum number of points per cell                               \n"
		"        - the maximum number of points per cell                               \n"
		"        It then places that many points inside the cell at random.            \n"
		"   c -- arrange points in concentric circles, taking four parameters:         \n"
		"        - number of points per circle                                         \n"
		"        - number of circles                                                   \n"
		"        - third parameter is 1 to add a centre point or 0 otherwise           \n"
		"        - how far out to make the circles                                     \n"
		"   e -- put one random point per scanline                                     \n"
		"   g -- make a grid with one point in the middle of each cell, parameters:    \n"
		"        - cell width (horizontal cell size)                                   \n"
		"        - cell height (vertical cell size)                                    \n"
		"        - maximum amount to disturb the point position at random from the     \n"
		"          middle (horizontally)                                               \n"
		"        - maximum amount to disturb the point position at random from the     \n"
		"          middle (vertically)                                                 \n"
		"        - optional - can be used to make a staggered grid in order to emulate \n"
		"          a hex grid                                                          \n"
		"        - as above                                                            \n"
		"   r -- place points entirely at random, parameters:                          \n"
		"        - how many points to randomly place                                   \n"
		"                                                                              \n"
		"The fifth argument is the function to determine the colour of the pixel:      \n"
		"   a -- copy the colour of the point half way between the actual point and the\n"
		"        nearest Voronoi point                                                 \n"
		"   b --                                                                       \n"
		"   c --                                                                       \n"
		"   d --  average this point and the half way point                            \n"
		"   e --                                                                       \n"
		"   f --  use the alpha channel to determine what fraction of the distance from\n"
		"         the actual to the Voronoi point to go, to determine what pixel to    \n"
		"         copy, optional parameters:                                           \n"
		"         - amount of extra adding                                             \n"
		"         - take the actual distance into account                              \n"
		"   g --                                                                       \n"
		"   h --  copy the colour of the actual point                                  \n"
		"   m --  copy the colour of the Voronoi point, unless it is too far away in   \n"
		"         which case copy the colour of the actual point instead, parameters:  \n"
		"         - how much is too far away (by a factor of twenty)                   \n"
		"   n --  copy the colour of the Voronoi point                                 \n"
		"   o --  adjust the colour of the actual point by the distance, using the     \n"
		"         first parameter as a multiplier for the distance                     \n"
		"   q --                                                                       \n"
		"   s --                                                                       \n"
		"   u --                                                                       \n"
		"   v --                                                                       \n"
		"   w --                                                                       \n"
		"   x --  use the X coordinate of the nearest Voronoi point and the Y          \n"
		"         coordinate of the actual point                                       \n"
		"   y --  use the X coordinate of the actual point and the Y coordinate of the \n"
		"         nearest Voronoi point                                                \n"
		"   z --  one of the XY coordinates is taken from the actual point and the     \n"
		"         other from the Voronoi point, in order to decide which pixel to copy \n"
		"         (each pixel alternates between which X/Y is in use)                  \n"
		"                                                                              \n"
		"The sixth argument is the function to determine the alpha value of the pixel: \n"
		"   c --                                                                       \n"
		"   d --                                                                       \n"
		"   g --                                                                       \n"
		"   h --  do not change the alpha value                                        \n"
		"   i --  use the index number of the nearest point as the alpha value         \n"
		"         (this can be used with ff-outline and ff-back to outline each Voronoi\n"
		"          region)                                                             \n"
		"   j --                                                                       \n"
		"   k --                                                                       \n"
		"   n --  copy the alpha value from the nearest Voronoi point                  \n"
		"   x --                                                                       \n"
		"   y --                                                                       \n"
		"                                                                              \n"
		"For the third to sixth arguments, after the letter can specify the parameters \n"
		"to those modes by putting a comma and then a number (must be an integer); this\n"
		"can be uses multiple times for multiple parameters (e.g. a,4,2).              \n"
		"                                                                              \n"
		"Example usage:                                                                \n"
		"   $ ff-worley 3911 29,42,0.95 b r,500 d h < image.ff > image-worley.ff       \n"
		"\n"
	);
	exit(1);
}

static int point_compare(const void*aa,const void*bb) {
	const Point*a=aa;
	const Point*b=bb;
	return a->y-b->y?:a->x-b->x;
}

static inline void alloc_points(int n) {
	if(!n) {
		fprintf(stderr,"Need at least one point\n");
		exit(1);
	}
	points=calloc(npoint=n,sizeof(Point));
	if(!points) {
		fprintf(stderr,"Allocation failed\n");
		exit(1);
	}
}

static double metric(int x,int y) {
	switch(mode[P_metric]) {
		case 'a': return 0.5*(x+y+(x>y?x:y)/(double)(1+para[P_metric][0]));
		case 'b': return x*para[P_metric][0]+0.5*y;
		case 'c': return x>y?x:y;
		case 'd': return (x>y?x:y)+abs(x-y);
		case 'e': return hypot(x+para[P_metric][0],y+para[P_metric][1]);
		case 'm': return x+y;
		case 'p': return sqrt(x)*sqrt(y)+0.01*sqrt(x+y+para[P_metric][0]);
		case 'r': return sqrt(random()%(x+para[P_metric][0]))+sqrt(random()%(y+para[P_metric][1]));
	}
}

static void make_points(void) {
	int i,x,y;
	switch(mode[P_points]) {
		case 'b':
			alloc_points((width/para[P_points][0]+1)*(height/para[P_points][1]+1)*para[P_points][3]);
			npoint=0;
			for(x=0;x<width;x+=para[P_points][0]) {
				for(y=0;y<height;y+=para[P_points][1]) {
					npoint+=i=random()%(para[P_points][3]-para[P_points][2]+1)+para[P_points][2];
					while(i--) {
						points[npoint-i-1].x=x+random()%para[P_points][0];
						points[npoint-i-1].y=y+random()%para[P_points][1];
					}
				}
			}
			break;
		case 'c':
			alloc_points(para[P_points][0]*para[P_points][1]+para[P_points][2]);
			npoint=0;
			i=para[P_points][3]+1;
			for(x=0;x<para[P_points][0];x++) {
				for(y=0;y<para[P_points][1];y++) {
					points[npoint].x=0.5*width*(cos((TAU*x)/para[P_points][0])*((y+i)/(0.5+para[P_points][1]+i))+1.0);
					points[npoint].y=0.5*height*(sin((TAU*x)/para[P_points][0])*((y+i)/(0.5+para[P_points][1]+i))+1.0);
					npoint++;
				}
			}
			if(para[P_points][2]>0) {
				points[npoint].x=width>>1;
				points[npoint].y=height>>1;
				npoint++;
			}
			break;
		case 'e':
			alloc_points(height);
			for(i=0;i<height;i++) {
				points[i].x=random()%width;
				points[i].y=i;
			}
			break;
		case 'g':
			alloc_points((width/para[P_points][0]+1)*(height/para[P_points][1]+1));
			i=0;
			for(x=para[P_points][0]>>1;x<width;x+=para[P_points][0]) {
				for(y=para[P_points][1]>>1;y<height;y+=para[P_points][1]) {
					points[i].x=x+(y&1?para[P_points][4]:0)+random()%(2*para[P_points][2]+1)-para[P_points][2];
					points[i].y=y+(x&1?para[P_points][5]:0)+random()%(2*para[P_points][3]+1)-para[P_points][3];
					if(points[i].x>=0 && points[i].y>=0 && points[i].x<width && points[i].y<height) i++;
				}
			}
			npoint=i;
			break;
		case 'r':
			alloc_points(para[P_points][0]);
			for(i=0;i<npoint;i++) {
				points[i].x=random()%width;
				points[i].y=random()%height;
			}
			break;
		default:
			fprintf(stderr,"Invalid mode\n");
			exit(1);
	}
	qsort(points,npoint,sizeof(Point),point_compare);
	for(i=1;i<npoint;i++) if(points[i].y>0 && points[i].y<height) band[points[i].y]=i;
	for(i=1;i<height;i++) if(!band[i]) band[i]=band[i-1];
}

static inline unsigned char*getpoint(int x,int y) {
	if(x<0) x=0;
	if(x>width-1) x=width-1;
	if(y<0) y=0;
	if(y>height-1) y=height-1;
	return pic+x*8+y*width*8;
}

static inline void do_color(int x,int y,int i,double d) {
	static int a[3];
	int z;
	switch(mode[P_color]) {
		case 'a':
			x=(x+points[i].x)>>1;
			y=(y+points[i].y)>>1;
			memcpy(buf,getpoint(x,y),6);
			break;
		case 'b':
			z=para[P_color][0]*0.01*d;
			x=(z*x+points[i].x+z/2)/(z+1);
			y=(z*y+points[i].y+z/2)/(z+1);
			memcpy(buf,getpoint(x,y),6);
			break;
		case 'c':
			z=para[P_color][0]+para[P_color][1]+8;
			x=(x*8+points[i].x*para[P_color][0]+points[near[dist2-1].id].x*para[P_color][1])/z;
			y=(y*8+points[i].y*para[P_color][0]+points[near[dist2-1].id].y*para[P_color][1])/z;
			memcpy(buf,getpoint(x,y),6);
			break;
		case 'd':
			memcpy(buf,getpoint(x,y),6);
			x=(x+points[i].x)>>1;
			y=(y+points[i].y)>>1;
			buf[0]=(buf[0]+getpoint(x,y)[0])>>1;
			buf[2]=(buf[2]+getpoint(x,y)[2])>>1;
			buf[4]=(buf[4]+getpoint(x,y)[4])>>1;
			break;
		case 'e':
			memcpy(buf,getpoint(x,y),8);
			z=(((buf[6]<<8)|buf[7])/(double)(para[P_color][2]?:65535))*(d+para[P_color][0]);
			if(para[P_color][1]==-z) z++;
			x=(z*x+para[P_color][1]*points[i].x+z/2)/(z+para[P_color][1]);
			y=(z*y+para[P_color][1]*points[i].y+z/2)/(z+para[P_color][1]);
			memcpy(buf+8,getpoint(x,y),6);
			z=((buf[0]<<8)|buf[1])+((buf[8]<<8)|buf[9]);
			buf[0]=z>>9; buf[1]=z>>1;
			z=((buf[2]<<8)|buf[3])+((buf[10]<<8)|buf[11]);
			buf[2]=z>>9; buf[3]=z>>1;
			z=((buf[4]<<8)|buf[5])+((buf[12]<<8)|buf[13]);
			buf[4]=z>>9; buf[5]=z>>1;
			break;
		case 'f':
			memcpy(buf,getpoint(x,y),8);
			d=(((buf[6]<<8)|buf[7])+para[P_color][0]+d*para[P_color][1])/65535.0;
			x=(1.0-d)*x+d*points[i].x+0.5;
			y=(1.0-d)*y+d*points[i].y+0.5;
			memcpy(buf,getpoint(x,y),6);
			break;
		case 'g':
			if(para[P_color][2]) x+=(d*para[P_color][2])/100;
			memcpy(buf,getpoint(x+(x-points[i].x)*para[P_color][0]/8,y+(y-points[i].y)*para[P_color][1]/8),6);
			break;
		case 'h':
			memcpy(buf,getpoint(x,y),6);
			break;
		case 'm':
			memcpy(buf,d*20.0>para[P_color][0]?getpoint(x,y):getpoint(points[i].x,points[i].y),6);
			break;
		case 'n':
			memcpy(buf,getpoint(points[i].x,points[i].y),6);
			break;
		case 'o':
			memcpy(buf,getpoint(x,y),6);
			z=((buf[0]<<8)|buf[1])+d*0.25*para[P_color][0];
			if(para[P_color][1]) z=z<0?0:z>65535?65535:z;
			buf[0]=z>>8; buf[1]=z;
			z=((buf[2]<<8)|buf[3])+d*0.25*para[P_color][0];
			if(para[P_color][1]) z=z<0?0:z>65535?65535:z;
			buf[2]=z>>8; buf[3]=z;
			z=((buf[4]<<8)|buf[5])+d*0.25*para[P_color][0];
			if(para[P_color][1]) z=z<0?0:z>65535?65535:z;
			buf[4]=z>>8; buf[5]=z;
			break;
		case 'q':
			d=1.0/(d*0.01*(para[P_color][0]+0.5)+1.0);
			x=x*d+points[i].x*(1.0-d)/(para[P_color][1]+1);
			y=y*d+points[i].y*(1.0-d)/(para[P_color][2]+1);
			memcpy(buf,getpoint(x,y),6);
			break;
		case 's':
			memcpy(buf,getpoint(points[i].x,points[i].y),6);
			a[0]=(buf[0]<<8)|buf[1];
			a[1]=(buf[2]<<8)|buf[3];
			a[2]=(buf[4]<<8)|buf[5];
			memcpy(buf,getpoint(x,y),6);
			d=(a[0]+a[1]+a[2]+d*para[P_color][0])/3;
			a[0]=((buf[0]<<8)|buf[1])+50.25*(a[0]-d)/(para[P_color][1]+0.25);
			a[1]=((buf[2]<<8)|buf[3])+50.25*(a[1]-d)/(para[P_color][1]+0.25);
			a[2]=((buf[4]<<8)|buf[5])+50.25*(a[2]-d)/(para[P_color][1]+0.25);
			a[0]=a[0]<0?0:a[0]>65535?65535:a[0];
			a[1]=a[1]<0?0:a[1]>65535?65535:a[1];
			a[2]=a[2]<0?0:a[2]>65535?65535:a[2];
			buf[0]=a[0]>>8; buf[1]=a[0];
			buf[2]=a[1]>>8; buf[4]=a[1];
			buf[4]=a[2]>>8; buf[5]=a[2];
			break;
		case 'u':
			if(x) {
				memcpy(buf+8,(x^y)&para[P_color][0]?getpoint(x,y):getpoint(points[i].x,points[i].y),6);
				z=(((buf[0]<<8)|buf[1])*para[P_color][1]+((buf[8]<<8)|buf[9])*para[P_color][2])>>4;
				z=z<0?0:z>65535?65535:z; buf[0]=z>>8; buf[1]=z;
				z=(((buf[2]<<8)|buf[3])*para[P_color][1]+((buf[10]<<8)|buf[11])*para[P_color][2])>>4;
				z=z<0?0:z>65535?65535:z; buf[2]=z>>8; buf[3]=z;
				z=(((buf[4]<<8)|buf[5])*para[P_color][1]+((buf[12]<<8)|buf[13])*para[P_color][2])>>4;
				z=z<0?0:z>65535?65535:z; buf[4]=z>>8; buf[5]=z;
			} else {
				memcpy(buf,getpoint(x,y),6);
			}
			break;
		case 'v':
			memcpy(buf+8,getpoint(x,y),6);
			z=(buf[8]<<8)+buf[9]+para[P_color][0];
			z=z<0?0:z>65535?65535:z; buf[8]=z>>8; buf[9]=z;
			z=(buf[10]<<8)+buf[11]+para[P_color][1];
			z=z<0?0:z>65535?65535:z; buf[10]=z>>8; buf[11]=z;
			z=(buf[12]<<8)+buf[13]+para[P_color][2];
			z=z<0?0:z>65535?65535:z; buf[12]=z>>8; buf[13]=z;
			x=(points[i].x+para[P_color][3]*x)/(para[P_color][3]+1);
			y=(points[i].y+para[P_color][3]*y)/(para[P_color][3]+1);
			memcpy(buf,getpoint(x,y),6);
			memcpy(getpoint(x,y),buf+8,6);
			break;
		case 'w':
			memcpy(buf,getpoint(points[i].x,points[i].y),6);
			a[0]=(buf[0]<<8)|buf[1];
			a[1]=(buf[2]<<8)|buf[3];
			a[2]=(buf[4]<<8)|buf[5];
			memcpy(buf,getpoint(x-points[i].x+x,y-points[i].y+y),6);
			a[0]+=(buf[0]<<8)|buf[1];
			a[1]+=(buf[2]<<8)|buf[3];
			a[2]+=(buf[4]<<8)|buf[5];
			memcpy(buf,getpoint(x,y),6);
			a[0]-=(buf[0]<<8)|buf[1];
			a[1]-=(buf[2]<<8)|buf[3];
			a[2]-=(buf[4]<<8)|buf[5];
			a[0]=a[0]<0?0:a[0]>65535?65535:a[0];
			a[1]=a[1]<0?0:a[1]>65535?65535:a[1];
			a[2]=a[2]<0?0:a[2]>65535?65535:a[2];
			buf[0]=a[0]>>8; buf[1]=a[0];
			buf[2]=a[1]>>8; buf[4]=a[1];
			buf[4]=a[2]>>8; buf[5]=a[2];
			break;
		case 'x':
			memcpy(buf,getpoint(points[i].x,y),6);
			break;
		case 'y':
			memcpy(buf,getpoint(x,points[i].y),6);
			break;
		case 'z':
			memcpy(buf,(x^y)&1?getpoint(x,points[i].y):getpoint(points[i].x,y),6);
			break;
	}
}

static inline void do_alpha(int x,int y,int i,double d) {
	int z;
	switch(mode[P_alpha]) {
		case 'c':
			memcpy(buf+6,getpoint(x,y)+(para[P_alpha][d>para[P_alpha][0]?1:2]<<1),2);
			break;
		case 'd':
			if(para[P_alpha][3]) d*=0.1*para[P_alpha][3];
			d=fabs(d-near[dist2-1].dist*para[P_alpha][2]);
			z=(65535.0*(d+para[P_alpha][1]))/para[P_alpha][0];
			if(z<0) z=0;
			if(z>65535) z=65535;
			buf[6]=z>>8;
			buf[7]=z;
			break;
		case 'g':
			if(para[P_alpha][2]) x+=(d*para[P_alpha][2])/100;
			memcpy(buf+6,getpoint(x+(x-points[i].x)*para[P_alpha][0]/8,y+(y-points[i].y)*para[P_alpha][1]/8)+6,2);
			break;
		case 'h':
			memcpy(buf+6,getpoint(x,y)+6,2);
			break;
		case 'i':
			buf[6]=i>>8;
			buf[7]=i;
			break;
		case 'j':
			buf[6]=near[dist2-1].id>>8;
			buf[7]=near[dist2-1].id;
			break;
		case 'k':
			buf[6]=buf[7]=(x>points[i].x?255:0)^(y>=points[i].y?255:para[P_alpha][0]);
			break;
		case 'n':
			memcpy(buf+6,getpoint(points[i].x,points[i].y)+6,2);
			break;
		case 'x':
			z=((para[P_alpha][0]*(x-points[i].x))>>1)+32768;
			if(z<0) z=0;
			if(z>65535) z=65535;
			buf[6]=z>>8;
			buf[7]=z;
			break;
		case 'y':
			z=((para[P_alpha][0]*(y-points[i].y))>>1)+32768;
			if(z<0) z=0;
			if(z>65535) z=65535;
			buf[6]=z>>8;
			buf[7]=z;
			break;
	}
}

static void do_point(int x,int y) {
	int i,j;
	double d;
	for(i=0;i<nnear;i++) near[i].dist=HUGE_VAL;
	for(i=band[y];i<npoint;i++) {
		d=metric(abs(x-points[i].x),abs(y-points[i].y));
		if(isfuzz) d+=(random()/(double)RAND_MAX)*distfuzz;
		for(j=0;j<nnear;j++) {
			if(d<near[j].dist) {
				memmove(near+j+1,near+j,(nnear-j-1)*sizeof(Nearest));
				near[j].id=i;
				near[j].dist=d;
				break;
			}
		}
		if(points[i].y>y+near[nnear-1].dist) break;
	}
	for(i=band[y]-1;i>=0;i--) {
		d=metric(abs(x-points[i].x),abs(y-points[i].y));
		if(isfuzz) d+=(random()/(double)RAND_MAX)*distfuzz;
		for(j=0;j<nnear;j++) {
			if(d<near[j].dist) {
				memmove(near+j+1,near+j,(nnear-j-1)*sizeof(Nearest));
				near[j].id=i;
				near[j].dist=d;
				break;
			}
		}
		if(points[i].y<y-near[nnear-1].dist) break;
	}
	i=near[dist1-1].id;
	d=near[dist1-1].dist;
	do_color(x,y,i,d);
	do_alpha(x,y,i,d);
	fwrite(buf,1,8,stdout);
}

int main(int argc,char**argv) {
	int i,j;
	char*s;
	if (argc>1 && (!strcmp(argv[1],"-h") || !strcmp(argv[1],"--help"))) {
		usage();
	}
	if(argc!=countP+3) {
		fprintf(stderr,"Improper number of arguments\n");
		return 1;
	}
	srandom(strtol(argv[1],0,0));
	dist1=strtol(argv[2],&s,0);
	if(*s==',') dist2=strtol(s+1,&s,0); else dist2=dist1;
	if(*s==',') isfuzz=1,distfuzz=strtod(s+1,&s);
	for(i=0;i<countP;i++) {
		mode[i]=argv[i+3][0];
		for(s=argv[i+3]+1,j=0;*s==','&&j<16;j++) para[i][j]=strtol(s+1,&s,0);
		npara[i]=j;
	}
	fread(buf,1,16,stdin);
	fwrite(buf,1,16,stdout);
	width=(buf[8]<<24)|(buf[9]<<16)|(buf[10]<<8)|buf[11];
	height=(buf[12]<<24)|(buf[13]<<16)|(buf[14]<<8)|buf[15];
	pic=malloc(8*width*height);
	if(!pic) {
		fprintf(stderr,"Allocation failed\n");
		return 1;
	}
	fread(pic,8,width*height,stdin);
	near=calloc(nnear=dist1>dist2?dist1:dist2,sizeof(Nearest));
	band=calloc(height,sizeof(int));
	if(!near || !band) {
		fprintf(stderr,"Allocation failed\n");
		return 1;
	}
	make_points();
	for(i=0;i<height;i++) for(j=0;j<width;j++) do_point(j,i);
	return 0;
}

