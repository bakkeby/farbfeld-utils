#if 0
gcc -s -O2 -o ~/bin/ff-hopalong ff-hopalong.c -lm
exit
#endif

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char*picture;
static int width,height,count;
static double xmin,xmax,ymin,ymax;
static double A,B,C,X,Y,Z;

static void
usage(void)
{
	fprintf(stderr,
		"Usage: ff-hopalong <width> <height> <count> <xmin> <xmax> <ymin> <ymax>       \n"
		"                   <A> <B> <C> <X> <Y>                                        \n"
		"                                                                              \n"
		"This farbfeld program draws a picture using HOPALONG attractors.              \n"
		"                                                                              \n"
		"This program does not read from standard in.                                  \n"
		"                                                                              \n"
		"The filter takes exactly 12 arguments:                                        \n"
		"   width                                                                      \n"
		"   height                                                                     \n"
		"   count - how many times to run the algorithm (how many dots to produce)     \n"
		"   xmin                                                                       \n"
		"   xmax                                                                       \n"
		"   ymin                                                                       \n"
		"   ymax                                                                       \n"
		"   A                                                                          \n"
		"   B                                                                          \n"
		"   C                                                                          \n"
		"   X                                                                          \n"
		"   Y                                                                          \n"
		"                                                                              \n"
		"The above arguments affects the algorithm and have no clear description. All  \n"
		"of the arguments from xmin to Y can be float values.                          \n"
		"                                                                              \n"
		"Example usage:                                                                \n"
		"   $ ff-hopalong 640 480 33360 0 640 0 480 43 12 19 320 240 > image.ff        \n"
		"\n"
	);
	exit(1);
}

int main(int argc,char**argv) {
	if (argc>1 && (!strcmp(argv[1],"-h") || !strcmp(argv[1],"--help"))) {
		usage();
	}
	if(argc!=13) {
		fprintf(stderr,"Improper number of arguments\n");
		return 1;
	}
	width=strtol(argv[1],0,0);
	height=strtol(argv[2],0,0);
	count=strtol(argv[3],0,0)+1;
	xmin=strtod(argv[4],0);
	xmax=strtod(argv[5],0);
	ymin=strtod(argv[6],0);
	ymax=strtod(argv[7],0);
	A=strtod(argv[8],0);
	B=strtod(argv[9],0);
	C=strtod(argv[10],0);
	X=strtod(argv[11],0);
	Y=strtod(argv[12],0);
	if(width<=0 || height<=0 || count<=0 || xmax<=xmin || ymax<=ymin) {
		fprintf(stderr,"Argument value out of range\n");
		return 1;
	}
	picture=calloc(width,height);
	if(!picture) {
		fprintf(stderr,"Memory allocation failed\n");
		return 1;
	}
	while(--count) {
		Z=Y-(X<0.0?-1.0:1.0)*sqrt(fabs(B*X-C));
		Y=A-X;
		X=Z;
		if(X>=xmin && X<=xmax && Y>=ymin && Y<=ymax) picture[((int)((X-xmin)*width/(xmax-xmin)))+width*((int)((Y-ymin)*height/(ymax-ymin)))]=-1;
	}
	fwrite("farbfeld",1,8,stdout);
	putchar(width>>24);
	putchar(width>>16);
	putchar(width>>8);
	putchar(width);
	putchar(height>>24);
	putchar(height>>16);
	putchar(height>>8);
	putchar(height);
	for(;count<width*height;count++) {
		putchar(picture[count]);
		putchar(picture[count]);
		putchar(picture[count]);
		putchar(picture[count]);
		putchar(picture[count]);
		putchar(picture[count]);
		putchar(255);
		putchar(255);
	}
	return 0;
}
