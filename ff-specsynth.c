#if 0
gcc -s -O2 -o ~/bin/ff-specsynth ff-specsynth.c -lm
exit
#endif

#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define TAU (6.283185307179586476925286766559005768394)

static double*gRe;
static double*gIm;
static double*GRe;
static double*GIm;

static int size;
static int nrand;
static double opt[127];

static void
usage(void)
{
	fprintf(stderr,
		"Usage: ff-specsynth <size> <?opt=value?> ... <?opt=value?>                    \n"
		"                                                                              \n"
		"This farbfeld program generates a randomized picture using spectral synthesis.\n"
		"                                                                              \n"
		"Make a greyscale texture pattern by spectral synthesis.                       \n"
		"                                                                              \n"
		"This program does not read from standard in.                                  \n"
		"                                                                              \n"
		"The filter takes the following arguments:                                     \n"
		"   - size - must be a power of two and maximum size is 32768 (the output is a \n"
		"            square image, i.e. providing 16 will give an image that is 16x16) \n"
		"   - option - zero or more options on the form of letter followed by equal    \n"
		"              sign and then a number (which can include fractions)            \n"
		"                                                                              \n"
		"The following options are available:                                          \n"
		"   a -- (-1.0) if nonnegative, specifies the minimum value for normalization  \n"
		"        zero makes the picture dark and higher numbers make it brighter       \n"
		"   h -- (-0.5) defines zoom level, where lower numbers are zoomed in more     \n"
		"   j -- (0.0) this parameter has various effects on the output                \n"
		"   m -- (1.0) this parameter has various effects on the output                \n"
		"   n -- (4.0) number of times to add together uniform random numbers; this    \n"
		"        controls the distribution of the random number generator              \n"
		"   s -- (1.0) random number seed                                              \n"
		"   v -- (-1.0) affects the distribution of shades during normalization        \n"
		"   z -- (-1.0) if nonnegative, specifies the minimum value for normalization  \n"
		"        zero makes the picture bright and higher numbers make it darker       \n"
		"                                                                              \n"
		"Example usage:                                                                \n"
		"   $ ff-specsynth 512 > image-specsynth.ff                                    \n"
		"   $ ff-specsynth 512 h=-0.1 > image-specsynth.ff                             \n"
		"   $ ff-specsynth 512 s=0.1944 j=0.21 > image-specsynth.ff                    \n"
		"\n"
	);
	exit(1);
}

static void fatal(const char*s) {
	fprintf(stderr,"FATAL: %s\n",s);
	exit(1);
}

static void FFT2D(void) {
	int x,y,i,l;
	int n=size;
	int m=size;
	int l2n = 0, p = 1; //l2n will become log_2(n)
	while(p < n) {p *= 2; l2n++;}
	int l2m = 0; p = 1; //l2m will become log_2(m)
	while(p < m) {p *= 2; l2m++;}

	//Erase all history of this array
	for(x = 0; x < m; x++) //for each column
	for(y = 0; y < m; y++) //for each row
	{
		GRe[m * x + y] = gRe[m * x + y];
		GIm[m * x + y] = gIm[m * x + y];
	}

	//Bit reversal of each row
	int j;
	for(y = 0; y < m; y++) //for each row
	{
		j = 0;
		for(i = 0; i < n - 1; i++)
		{
			GRe[m * i + y] = gRe[m * j + y];
			GIm[m * i + y] = gIm[m * j + y];
			int k = n / 2;
			while (k <= j) {j -= k; k/= 2;}
			j += k;
		}
	}
	//Bit reversal of each column
	double tx = 0, ty = 0;
	for(x = 0; x < n; x++) //for each column
	{
		j = 0;
		for(i = 0; i < m - 1; i++)
		{
			if(i < j)
			{
				tx = GRe[m * x + i];
				ty = GIm[m * x + i];
				GRe[m * x + i] = GRe[m * x + j];
				GIm[m * x + i] = GIm[m * x + j];
				GRe[m * x + j] = tx;
				GIm[m * x + j] = ty;
			}
			int k = m / 2;
			while (k <= j) {j -= k; k/= 2;}
			j += k;
		}
	}

	//Calculate the FFT of the columns
	for(x = 0; x < n; x++) //for each column
	{
		//This is the 1D FFT:
		double ca = -1.0;
		double sa = 0.0;
		int l1 = 1, l2 = 1;
		for(l=0;l < l2n;l++)
		{
			l1 = l2;
			l2 *= 2;
			double u1 = 1.0;
			double u2 = 0.0;
			for(j = 0; j < l1; j++)
			{
				for(i = j; i < n; i += l2)
				{
					int i1 = i + l1;
					double t1 = u1 * GRe[m * x + i1] - u2 * GIm[m * x + i1];
					double t2 = u1 * GIm[m * x + i1] + u2 * GRe[m * x + i1];
					GRe[m * x + i1] = GRe[m * x + i] - t1;
					GIm[m * x + i1] = GIm[m * x + i] - t2;
					GRe[m * x + i] += t1;
					GIm[m * x + i] += t2;
				}
				double z =  u1 * ca - u2 * sa;
				u2 = u1 * sa + u2 * ca;
				u1 = z;
			}
			sa = sqrt((1.0 - ca) / 2.0);
			//if(!inverse) sa = -sa;
			ca = sqrt((1.0 + ca) / 2.0);
		}
	}
	//Calculate the FFT of the rows
	for(y = 0; y < m; y++) //for each row
	{
		//This is the 1D FFT:
		double ca = -1.0;
		double sa = 0.0;
		int l1= 1, l2 = 1;
		for(l = 0; l < l2m; l++)
		{
			l1 = l2;
			l2 *= 2;
			double u1 = 1.0;
			double u2 = 0.0;
			for(j = 0; j < l1; j++)
			{
				for(i = j; i < n; i += l2)
				{
					int i1 = i + l1;
					double t1 = u1 * GRe[m * i1 + y] - u2 * GIm[m * i1 + y];
					double t2 = u1 * GIm[m * i1 + y] + u2 * GRe[m * i1 + y];
					GRe[m * i1 + y] = GRe[m * i + y] - t1;
					GIm[m * i1 + y] = GIm[m * i + y] - t2;
					GRe[m * i + y] += t1;
					GIm[m * i + y] += t2;
				}
				double z =  u1 * ca - u2 * sa;
				u2 = u1 * sa + u2 * ca;
				u1 = z;
			}
			sa = sqrt((1.0 - ca) / 2.0);
			//if(!inverse) sa = -sa;
			ca = sqrt((1.0 + ca) / 2.0);
		}
	}
}

static void normalize(double*q) {
	int i;
	double mi=0.0;
	double ma=0.0;
	for(i=size*size;i>=0;i--) {
		if(opt['i'] && q[i]<0) q[i]*=-1.0;
		if(mi>q[i]) mi=q[i];
		if(ma<q[i]) ma=q[i];
	}
	if(opt['a']>=0.0) mi=-opt['a'];
	if(opt['z']>=0.0) ma=opt['z'];
	if(mi==ma) ma+=1.0;
	for(i=size*size;i>=0;i--) q[i]=(q[i]-mi)/(ma-mi);
	if(opt['v']>=0.0) for(i=size*size;i>=0;i--) q[i]*=q[i]+opt['v'];
}

static inline void put16(int v) {
	putchar(v>>8);
	putchar(v);
}

static double spec_random(void) {
	double r=-0.5*opt['n'];
	int i;
	for(i=0;i<nrand;i++) r+=((double)random())/(double)RAND_MAX;
	return r;
}

int main(int argc, char **argv) {
	int i,x,y;
	double a,z;
	if (argc<2 || !strcmp(argv[1],"-h") || !strcmp(argv[1],"--help")) {
		usage();
	}
	size=strtol(argv[1],0,0);
	if(size<=0 || size>32768 || (size&(size-1))) fatal("Bad size");
	gRe=calloc(sizeof(double),size*size);
	gIm=calloc(sizeof(double),size*size);
	GRe=calloc(sizeof(double),size*size);
	GIm=calloc(sizeof(double),size*size);
	if(!gRe || !gIm || !GRe || !GIm) fatal("Out of memory");
	opt['a']=-1.0;
	opt['h']=-0.5;
	opt['j']=0.0;
	opt['m']=1.0;
	opt['n']=4.0;
	opt['s']=1.0;
	opt['v']=-1.0;
	opt['z']=-1.0;
	for(i=2;i<argc;i++) if(argv[i][0] && argv[i][1]=='=') opt[argv[i][0]]=strtod(argv[i]+2,0);
	srandom(opt['s']);
	nrand=(int)opt['n'];
	for(x=0;x<=size/2;x++) for(y=0;y<=size/2;y++) {
		a=TAU*((double)random())/(double)RAND_MAX;
		z=pow((double)(x*x+y*(long long)y),opt['h'])*spec_random();
		gRe[size*(y?size-y:0)+(x?size-x:0)]=(gRe[size*y+x]=z*cos(a))*opt['m'];
		gIm[size*(y?size-y:0)+(x?size-x:0)]=-(gIm[size*y+x]=z*sin(a))*opt['m'];
		if(x && y && x!=size/2 && y!=size/2) {
			a=TAU*((double)random())/(double)RAND_MAX;
			z=pow((double)(x*x+y*(long long)y),opt['h']+opt['j'])*spec_random()*opt['m'];
			gRe[size*(size-y)+x]=gRe[size*y+(size-x)]=z*cos(a);
			gIm[size*(size-y)+x]=-(gIm[size*y+(size-x)]=z*sin(a));
		}
	}
	*gRe=*gIm=0.0;
	gIm[size*size/2]=gIm[size/2]=gIm[(size+1)*size/2]=0.0;
	FFT2D();
	fwrite("farbfeld\0",1,10,stdout);
	put16(size);
	put16(0);
	put16(size);
	normalize(GRe);
	for(x=0;x<size;x++) for(y=0;y<size;y++) {
		z=65535.0*GRe[size*y+x];
		if(z<0.0) z=0;
		if(z>65535.0) z=65535.0;
		put16((int)z); put16((int)z); put16((int)z);
		put16(65535);
	}
	return 0;
}
