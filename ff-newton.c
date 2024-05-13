#if 0
gcc -s -O2 -o ~/bin/ff-newton -Wno-unused-result ff-newton.c -lm
exit
#endif

// This program is make Newton fractal.

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
	unsigned char c[8];
} Color;

static int width,height,maxiter,nroots;
static double tolerance,mul_r,mul_i;
static double root_r[32];
static double root_i[32];
static Color colors[32];
static double coef_r[32];
static double coef_i[32];
static double dcoef_r[32];
static double dcoef_i[32];

static void
usage(void)
{
	fprintf(stderr,
		"Usage: ff-newton <width> <height> <iterations> <tolerance> <root> <?root?> ...\n"
		"                                                                              \n"
		"This farbfeld program generates a Newton fractal.                             \n"
		"                                                                              \n"
		"This program does not read from standard in.                                  \n"
		"                                                                              \n"
		"The program starts with four arguments:                                       \n"
		"   width                                                                      \n"
		"   height                                                                     \n"
		"   number of iterations                                                       \n"
		"   tolerance - a floating number, which should be positive (cannot be 0 and   \n"
		"               works a bit different if negative)                             \n"
		"                                                                              \n"
		"After those four comes three more for each root:                              \n"
		"   real part                                                                  \n"
		"   imaginary part                                                             \n"
		"   colour (the alpha is ignored if the tolerance is positive)                 \n"
		"                                                                              \n"
		"The coordinates are scaled so that it ranges -1 to +1 for both real and       \n"
		"imaginary parts.                                                              \n"
		"                                                                              \n"
		"Using only one root is technically valid, but generates a blank page - use two\n"
		"or more roots to see the fractal.                                             \n"
		"                                                                              \n"
		"If the tolerance is positive then the alpha channel of the output specifies   \n"
		"how many iterations are needed to reach close enough to the root; it is more  \n"
		"opaque if less iterations are needed and less opaque if more iterations are   \n"
		"needed.                                                                       \n"
		"                                                                              \n"
		"If it is not reached in that many iterations, then it is transparent black.   \n"
		"                                                                              \n"
		"The colour can be defined through one of the following patterns:              \n"
		"   rrggbb            three pairs of 8-bit hex values                          \n"
		"   rrggbbaa          four pairs of 8-bit hex values                           \n"
		"   rrrrggggbbbb      three pairs of 16-bit hex values                         \n"
		"   rrrrggggbbbbaaaa  four pairs of 16-bit hex values                          \n"
		"                                                                              \n"
		"Example usage:                                                                \n"
		"   $ ff-newton 640 480 97 0.0192  0.072  0.0439 900060 \\                     \n"
		"                                 -0.93  -0.23   609020 \\                     \n"
		"                                  0.18  -0.72   602290 \\                     \n"
		"                                 -0.32  -0.031  AA2244 > newton.ff            \n"
		"\n"
	);
	exit(1);
}

static void read_color(const char*s,int n) {
	unsigned char*c=colors[n].c;
	switch(strlen(s)) {
		case 6:
			sscanf(s,"%2hhx%2hhx%2hhx",c+0,c+2,c+4);
			c[1]=c[0];
			c[3]=c[2];
			c[5]=c[4];
			c[7]=c[6]=255;
			break;
		case 8:
			sscanf(s,"%2hhx%2hhx%2hhx%2hhx",c+0,c+2,c+4,c+6);
			c[1]=c[0];
			c[3]=c[2];
			c[5]=c[4];
			c[7]=c[6];
			break;
		case 12:
			sscanf(s,"%2hhx%2hhx%2hhx%2hhx%2hhx%2hhx",c+0,c+1,c+2,c+3,c+4,c+5);
			c[7]=c[6]=255;
			break;
		case 16:
			sscanf(s,"%2hhx%2hhx%2hhx%2hhx%2hhx%2hhx%2hhx%2hhx",c+0,c+1,c+2,c+3,c+4,c+5,c+6,c+7);
			break;
		default:
			fprintf(stderr,"Invalid color format (%d)\n",(int)strlen(s));
			exit(1);
	}
}

static void make_coefficients(void) {
	int n,m;
	double v_r,v_i;
	coef_r[0]=1.0;
	for(n=0;n<nroots;n++) {
		for(m=n+1;m>=0;m--) {
			v_r=coef_r[m]*root_r[n]-coef_i[m]*root_i[n];
			v_i=coef_r[m]*root_i[n]+coef_i[m]*root_r[n];
			coef_r[m]=(m?coef_r[m-1]:0.0)-v_r;
			coef_i[m]=(m?coef_i[m-1]:0.0)-v_i;
		}
	}
	for(m=0;m<31;m++) {
		dcoef_r[m]=(m+1)*coef_r[m+1];
		dcoef_i[m]=(m+1)*coef_i[m+1];
	}
}

static void do_average(double x,double y) {
	int n;
	double c[4]={0.0,0.0,0.0,0.0};
	double d=0.0;
	double z;
	for(n=0;n<nroots;n++) {
		d+=z=1.0/(hypot(x-root_r[n],y-root_i[n])-tolerance);
		c[0]+=z*(colors[n].c[0]*256+colors[n].c[1])/65535.001;
		c[1]+=z*(colors[n].c[2]*256+colors[n].c[3])/65535.001;
		c[2]+=z*(colors[n].c[4]*256+colors[n].c[5])/65535.001;
		c[3]+=z*(colors[n].c[6]*256+colors[n].c[7])/65535.001;
	}
	n=65535.01*c[0]/d;
	if(n<0) n=0; else if(n>65535) n=65535;
	putchar(n>>8); putchar(n);
	n=65535.01*c[1]/d;
	if(n<0) n=0; else if(n>65535) n=65535;
	putchar(n>>8); putchar(n);
	n=65535.01*c[2]/d;
	if(n<0) n=0; else if(n>65535) n=65535;
	putchar(n>>8); putchar(n);
	n=65535.01*c[3]/d;
	if(n<0) n=0; else if(n>65535) n=65535;
	putchar(n>>8); putchar(n);
}

static void do_pixel(int x,int y) {
	double z_r=(x*2.0/width-1.0);
	double z_i=(y*2.0/height-1.0);
	double v_r,v_i,t;
	double num_r,num_i,den_r,den_i;
	int i,m;
	i=maxiter;
	while(i) {
		num_r=num_i=den_r=den_i=0.0;
		v_r=1.0;
		v_i=0.0;
		for(m=0;m<32;m++) {
			num_r+=v_r*coef_r[m]-v_i*coef_i[m];
			num_i+=v_i*coef_r[m]+v_r*coef_i[m];
			den_r+=v_r*dcoef_r[m]-v_i*dcoef_i[m];
			den_i+=v_i*dcoef_r[m]+v_r*dcoef_i[m];
			t=v_r;
			v_r=t*z_r-v_i*z_i;
			v_i=t*z_i+v_i*z_r;
		}
		t=den_r*den_r+den_i*den_i;
		if(t==0.0) break;
		z_r-=(num_r*den_r+num_i*den_i)/t;
		z_i-=(num_i*den_r-num_r*den_i)/t;
		for(m=0;m<nroots;m++) {
			if(hypot(z_r-root_r[m],z_i-root_i[m])<=tolerance) {
				fwrite(colors[m].c,1,6,stdout);
				i=(colors[m].c[6]*256+colors[m].c[7])*(i/(double)maxiter);
				putchar(i>>8); putchar(i);
				return;
			}
		}
		i--;
	}
	if(tolerance<0.0 && isfinite(z_r) && isfinite(z_i)) {
		do_average(z_r,z_i);
	} else {
		putchar(0); putchar(0);
		putchar(0); putchar(0);
		putchar(0); putchar(0);
		putchar(0); putchar(0);
	}
}

int main(int argc,char**argv) {
	int i,j;
	if (argc>1 && (!strcmp(argv[1],"-h") || !strcmp(argv[1],"--help"))) {
		usage();
	}
	if(argc<6) {
		improper_args:
		fprintf(stderr,"Incorrect number of arguments\n");
		return 1;
	}
	width=strtol(argv[1],0,10);
	height=strtol(argv[2],0,10);
	maxiter=strtol(argv[3],0,10);
	tolerance=strtod(argv[4],0);
	nroots=(argc-5)/3;
	if(nroots<1 || nroots>30 || (argc-5)%3) goto improper_args;
	for(i=0;i<nroots;i++) {
		root_r[i]=strtod(argv[3*i+5],0);
		root_i[i]=-strtod(argv[3*i+6],0);
		read_color(argv[3*i+7],i);
	}
	make_coefficients();
	fwrite("farbfeld",1,8,stdout);
	putchar(width>>24); putchar(width>>16); putchar(width>>8); putchar(width);
	putchar(height>>24); putchar(height>>16); putchar(height>>8); putchar(height);
	for(i=0;i<height;i++) for(j=0;j<width;j++) do_pixel(j,i);
	return 0;
}
