#if 0
gcc -s -O2 -o ~/bin/ff-icosa ff-icosa.c -lm
exit
#endif

/*
	This program is based on David Madore's "psychedelic icosahedra generator" program, which is public domain.
	This modified version is also public domain.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifndef FUCKING_ARCHAIC_SYSTEM
#define FUCKING_ARCHAIC_SYSTEM 0
#endif

#if FUCKING_ARCHAIC_SYSTEM
#define I 1.0fi
#define complex _Complex
#else
#include <complex.h>
#endif

#define IMAGE_HEIGHT imgheight
#define IMAGE_WIDTH imgwidth

#define IMAGE_CENTER_X (0.5*imgwidth)
#define IMAGE_CENTER_Y (0.5*imgheight)
#define IMAGE_SCALE_X imgscale
#define IMAGE_SCALE_Y imgscale

static int imgheight,imgwidth;
static double imgscale;
static int option;

static void
usage(void)
{
	fprintf(stderr,
		"Usage: ff-icosa <width> <height> <scale amount> <?seed?> <?option?>           \n"
		"                                                                              \n"
		"This program generates psychedelic icosahedral fractal pictures and writes the\n"
		"result to stdout.                                                             \n"
		"                                                                              \n"
		"This program does not read from standard in.                                  \n"
		"                                                                              \n"
		"The program takes up to five arguments:                                       \n"
		"   - width                                                                    \n"
		"   - height                                                                   \n"
		"   - scale amount (not necessarily an integer)                                \n"
		"   - random seed (optional)                                                   \n"
		"   - option (integer from 0 to 127)                                           \n"
		"                                                                              \n"
		"The option defaults to 0 and has various effects.                             \n"
		"                                                                              \n"
		"Example usage:                                                                \n"
		"   $ ff-icosa 640 480 68.83 1.49 > image-icosa.ff                             \n"
		"   $ ff-icosa 640 480 281.83 41.49 89 > image-icosa.ff                        \n"
		"\n"
	);
	exit(1);
}

#if FUCKING_ARCHAIC_SYSTEM
static double
cabs (double complex z)
{
	double a, b;

	a = creal (z);
	b = cimag (z);
	return sqrt(a*a+b*b);
}
#endif

static double complex
icosahedral_function (double complex z)
{
	double complex z2, z3, z5, z10, z15, z20;
	double complex zvert, zface, zvert2, zvert3, zvert5, zface2, zface3;
	z2 = z*z;
	z3 = z*z2;
	z5 = z2*z3;
	z10 = z5*z5;
	z15 = z5*z10;
	z20 = z10*z10;
	zvert = z*(z10+11*z5-1);
	zface = -(z20+1)+228*(z15-z5)-494*z10;
	zvert2 = zvert*zvert;
	zvert3 = zvert*zvert2;
	zvert5 = zvert2*zvert3;
	zface2 = zface*zface;
	zface3 = zface*zface2;
	return 1728.*zvert5/zface3;
}

static double complex
octahedral_function (double complex z)
{
	double complex z2, z4, z8;
	double complex zvert, zface, zvert2, zvert4, zface2, zface3;
	z2 = z*z;
	z4 = z2*z2;
	z8 = z4*z4;
	zvert = z*(z4-1);
	zface = z8+14*z4+1;
	zvert2 = zvert*zvert;
	zvert4 = zvert2*zvert2;
	zface2 = zface*zface;
	zface3 = zface*zface2;
	return 108.*zvert4/zface3;
}

static double complex
tetrahedral_function (double complex z)
{
	double complex z2, z4, z8;
	double complex zvert, zface, zvert2, zvert3, zface2, zface3;
	z2 = z*z;
	z4 = z2*z2;
	z8 = z4*z4;
	zvert = z4-2*I*sqrt(3.)*z2+1;
	zface = z4+2*I*sqrt(3.)*z2+1;
	zvert2 = zvert*zvert;
	zvert3 = zvert*zvert2;
	zface2 = zface*zface;
	zface3 = zface*zface2;
	return zvert3/zface3;
}

static void
riemann_sphere (double complex z, double *u, double *v, double *w)
{
	double t;

	t = cabs (z);
	*u = 2*creal(z)/(1+t*t);
	*v = 2*cimag(z)/(1+t*t);
	*w = (1-t*t)/(1+t*t);
}

struct rot {
	double complex a, b, c, d;
};

static double
uniform_random (void)
{
	return (random()%1000)/1000. + (random()%1000)/1.e6;
}

static void
prepare_random_rotation (struct rot *r)
{
	double a, b, c, d, t;

	do {
		a = uniform_random()*2.-1.;
		b = uniform_random()*2.-1.;
		c = uniform_random()*2.-1.;
		d = uniform_random()*2.-1.;
		t = a*a+b*b+c*c+d*d;
	} while ( t >= 1 );
	t = sqrt (t);
	a /= t;
	b /= t;
	c /= t;
	d /= t;
	r->a = d + I*c;
	r->b = -b + I*a;
	r->c = b + I*a;
	r->d = d - I*c;
}

static double complex
do_rotation (const struct rot *r, double complex z)
{
	return (r->a*z+r->b)/(r->c*z+r->d);
}

static struct rot r0;
static struct rot r1;
static struct rot r2;

static void
make_image (void)
{
	int i, j;

	for ( i=0 ; i<IMAGE_HEIGHT ; i++ )
		{
			if(option&8) r1.c=-r1.b*(i*0.5/IMAGE_HEIGHT);
			for ( j=0 ; j<IMAGE_WIDTH ; j++ )
	{
		double complex z;
		double u, v, w;
		unsigned int r, g, b;

		z = (j-IMAGE_CENTER_X)/IMAGE_SCALE_X
			+ I * (i-IMAGE_CENTER_Y)/IMAGE_SCALE_Y;
		w = z = do_rotation (&r0, z);
		v = z = option&2 ? octahedral_function(z) : icosahedral_function(z);
		u = z = do_rotation (&r1, z);
		z = option&4 ? tetrahedral_function(z) : icosahedral_function(z);
		if(option&64) z=0.5*(tetrahedral_function(w)+z);
		z = do_rotation (&r2, z);
		if(option&16) z=0.5*(u+z);
		if(option&32) z=0.5*(v+z);
		riemann_sphere (z, &u, &v, &w);
		if ( w < 0 )
			r = (-w)*0xFFFF;
		else
			r = 0;
		if ( u > 0 )
			g = u*0xFFFF;
		else
			g = 0;
		if(option&1) w=v;
		if ( w > 0 )
			b = w*0xFFFF;
		else
			b = 0;
		putchar(r>>8); putchar(r);
		putchar(g>>8); putchar(g);
		putchar(b>>8); putchar(b);
					putchar(255); putchar(255);
	}
		}
}

int main (int argc,char**argv)
{
	if (argc<4 || !strcmp(argv[1],"-h") || !strcmp(argv[1],"--help")) {
		usage();
	}
	imgwidth=strtol(argv[1],0,0);
	imgheight=strtol(argv[2],0,0);
	imgscale=strtod(argv[3],0);
	if(argc>4) srandom(strtol(argv[4],0,0));
	if(argc>5) option=strtol(argv[5],0,0);
	prepare_random_rotation (&r0);
	prepare_random_rotation (&r1);
	prepare_random_rotation (&r2);
	fwrite("farbfeld",1,8,stdout);
	putchar(imgwidth>>24); putchar(imgwidth>>16); putchar(imgwidth>>8); putchar(imgwidth);
	putchar(imgheight>>24); putchar(imgheight>>16); putchar(imgheight>>8); putchar(imgheight);
	make_image ();
	return 0;
}
