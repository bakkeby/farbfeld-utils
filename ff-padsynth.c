#if 0
gcc -s -O2 -o ~/bin/ff-padsynth ff-padsynth.c -lm
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
static int opt[127];

static void
usage(void)
{
	fprintf(stderr,
		"Usage: ff-padsynth <size> <?-options?> <harmonics> <?harmonics?> ...          \n"
		"                                                                              \n"
		"This farbfeld program generates a randomized picture by using a               \n"
		"two-dimensional PADsynth algorithm.                                           \n"
		"                                                                              \n"
		"This program implements the PADsynth algorithm, which is designed for audio,  \n"
		"but this is a two-dimensional version that is designed for graphics.          \n"
		"                                                                              \n"
		"This program does not read from standard in.                                  \n"
		"                                                                              \n"
		"The filter takes one or more arguments.                                       \n"
		"                                                                              \n"
		"The first argument is the picture size, which must be a power of two (256,    \n"
		"512, 1024, etc).                                                              \n"
		"                                                                              \n"
		"Then follows a series of options, which is defined as a - followed by one or  \n"
		"more lowercase characters.                                                    \n"
		"                                                                              \n"
		"The available options are:                                                    \n"
		"   u -- alters the output by skipping some transformations                    \n"
		"   p -- increases colour intensitiy                                           \n"
		"   q -- significantly increases colour intensitiy, combine with p for even    \n"
		"        greater effect                                                        \n"
		"   s -- makes the program use an alternate seed for random number generation  \n"
		"   x -- use an alternate formula for generating the output                    \n"
		"   r -- reduce red channel output strenght by 50%%                            \n"
		"   g -- reduce green channel output strenght by 50%%                          \n"
		"   b -- reduce blue channel output strenght by 50%%                           \n"
		"                                                                              \n"
		"The rest of the arguments define the harmonics.                               \n"
		"                                                                              \n"
		"Each harmonic is specified as seven numbers (fractional parts are allowed)    \n"
		"with commas in between:                                                       \n"
		"   - horizontal frequency                                                     \n"
		"   - vertical frequency                                                       \n"
		"   - horizontal bandwidth                                                     \n"
		"   - vertical bandwidth                                                       \n"
		"   - red amplitude                                                            \n"
		"   - green amplitude                                                          \n"
		"   - blue amplitude                                                           \n"
		"                                                                              \n"
		"Example usage:                                                                \n"
		"   $ ff-padsynth 512 -ps 0.44,0.32,0.78,0.80,0.98,0.68,0.48 > padsynth.ff     \n"
		"\n"
	);
	exit(1);
}

static void fatal(const char*s) {
	fprintf(stderr,"FATAL: %s\n",s);
	exit(1);
}

static void FFT2D(void) {
	int x,y,c,i,l;
	int n=size;
	int m=size;
	int l2n = 0, p = 1; //l2n will become log_2(n)
	while(p < n) {p *= 2; l2n++;}
	int l2m = 0; p = 1; //l2m will become log_2(m)
	while(p < m) {p *= 2; l2m++;}

	//Erase all history of this array
	for(x = 0; x < m; x++) //for each column
	for(y = 0; y < m; y++) //for each row
	for(c = 0; c < 3; c++) //for each color component
	{
		GRe[3 * m * x + 3 * y + c] = gRe[3 * m * x + 3 * y + c];
		GIm[3 * m * x + 3 * y + c] = gIm[3 * m * x + 3 * y + c];
	}

	//Bit reversal of each row
	int j;
	for(y = 0; y < m; y++) //for each row
	for(c = 0; c < 3; c++) //for each color component
	{
		j = 0;
		for(i = 0; i < n - 1; i++)
		{
			GRe[3 * m * i + 3 * y + c] = gRe[3 * m * j + 3 * y + c];
			GIm[3 * m * i + 3 * y + c] = gIm[3 * m * j + 3 * y + c];
			int k = n / 2;
			while (k <= j) {j -= k; k/= 2;}
			j += k;
		}
	}
	//Bit reversal of each column
	double tx = 0, ty = 0;
	for(x = 0; x < n; x++) //for each column
	for(c = 0; c < 3; c++) //for each color component
	{
		j = 0;
		for(i = 0; i < m - 1; i++)
		{
			if(i < j)
			{
				tx = GRe[3 * m * x + 3 * i + c];
				ty = GIm[3 * m * x + 3 * i + c];
				GRe[3 * m * x + 3 * i + c] = GRe[3 * m * x + 3 * j + c];
				GIm[3 * m * x + 3 * i + c] = GIm[3 * m * x + 3 * j + c];
				GRe[3 * m * x + 3 * j + c] = tx;
				GIm[3 * m * x + 3 * j + c] = ty;
			}
			int k = m / 2;
			while (k <= j) {j -= k; k/= 2;}
			j += k;
		}
	}

	//Calculate the FFT of the columns
	for(x = 0; x < n; x++) //for each column
	for(c = 0; c < 3; c++) //for each color component
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
					double t1 = u1 * GRe[3 * m * x + 3 * i1 + c] - u2 * GIm[3 * m * x + 3 * i1 + c];
					double t2 = u1 * GIm[3 * m * x + 3 * i1 + c] + u2 * GRe[3 * m * x + 3 * i1 + c];
					GRe[3 * m * x + 3 * i1 + c] = GRe[3 * m * x + 3 * i + c] - t1;
					GIm[3 * m * x + 3 * i1 + c] = GIm[3 * m * x + 3 * i + c] - t2;
					GRe[3 * m * x + 3 * i + c] += t1;
					GIm[3 * m * x + 3 * i + c] += t2;
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
	for(c = 0; c < 3; c++) //for each color component
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
					double t1 = u1 * GRe[3 * m * i1 + 3 * y + c] - u2 * GIm[3 * m * i1 + 3 * y + c];
					double t2 = u1 * GIm[3 * m * i1 + 3 * y + c] + u2 * GRe[3 * m * i1 + 3 * y + c];
					GRe[3 * m * i1 + 3 * y + c] = GRe[3 * m * i + 3 * y + c] - t1;
					GIm[3 * m * i1 + 3 * y + c] = GIm[3 * m * i + 3 * y + c] - t2;
					GRe[3 * m * i + 3 * y + c] += t1;
					GIm[3 * m * i + 3 * y + c] += t2;
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
	for(i=3*size*size;i>=0;i--) {
		if(opt['i'] && q[i]<0) q[i]*=-1.0;
		if(mi>q[i]) mi=q[i];
		if(ma<q[i]) ma=q[i];
	}
	if(opt['a']) mi=0.0;
	if(opt['z']) ma=0.0;
	if(mi==ma) ma+=1.0;
	for(i=3*size*size;i>=0;i--) q[i]=(q[i]-mi)/(ma-mi);
	if(opt['v']) for(i=3*size*size;i>=0;i--) q[i]*=q[i];
}

static inline void put16(int v) {
	putchar(v>>8);
	putchar(v);
}

int main(int argc, char **argv) {
	int i,j,x,y;
	double a,b,z,xc,yc;
	double cr[3];
	if (argc>1 && (!strcmp(argv[1],"-h") || !strcmp(argv[1],"--help"))) {
		usage();
	}
	if(argc<2) fatal("Too few arguments");
	size=strtol(argv[1],0,0);
	if(size<=0 || size>32768 || (size&(size-1))) fatal("Bad size");
	gRe=calloc(sizeof(double)*3,size*size);
	gIm=calloc(sizeof(double)*3,size*size);
	GRe=calloc(sizeof(double)*3,size*size);
	GIm=calloc(sizeof(double)*3,size*size);
	if(!gRe || !gIm || !GRe || !GIm) fatal("Out of memory");
	for(i=2;i<argc;i++) {
		if(argv[i][0]=='-' && argv[i][1]>='Z') {
			for(j=1;argv[i][j];j++) opt[argv[i][j]&127]=1;
		} else {
			sscanf(argv[i],"%lf,%lf,%lf,%lf,%lf,%lf,%lf",&xc,&yc,&a,&b,cr+0,cr+1,cr+2);
			for(x=0;x<size/2;x++) for(y=0;y<size/2;y++) {
				if(opt['u'] && ((x^y)&10)) continue;
				if(opt['x']) z=exp(-pow(fabs(xc-(double)x)+fabs(yc-(double)y),2));
				else z=exp((xc-(double)x)*(xc-(double)x)/-a+(yc-(double)y)*(yc-(double)y)/-b);
				for(j=0;j<3;j++) gRe[3*size*y+3*x+j]+=cr[j]*z;
			}
		}
	}
	srandom(opt['s']+1);
	for(x=0;x<size/2;x++) for(y=0;y<size/2;y++) {
		z=TAU*((double)random())/(double)RAND_MAX;
		for(j=0;j<3;j++) {
			gIm[3*size*y+3*x+j]=sin(z)*gRe[3*size*y+3*x+j];
			gRe[3*size*y+3*x+j]=cos(z)*gRe[3*size*y+3*x+j];
			if(opt['p']) z+=0.6;
			if(opt['q']) z+=1.6;
		}

		/* The below was altering gIm in the original code but it didn't seem to have any
		 * particular visual effect on the ouput (although there are differenes in the generated
		 * binary). Changed this to alter the generated image (gRe) assuming that was the
		 * intended behaviour. */
		if(opt['r']) gRe[3*size*y+3*x+0]*=0.5;
		if(opt['g']) gRe[3*size*y+3*x+1]*=0.5;
		if(opt['b']) gRe[3*size*y+3*x+2]*=0.5;
	}
	FFT2D();
	fwrite("farbfeld\0",1,10,stdout);
	put16(size);
	put16(0);
	put16(size);
	normalize(GRe);
	for(x=0;x<size;x++) for(y=0;y<size;y++) {
		for(j=0;j<3;j++) {
			z=65535.0*GRe[3*size*y+3*x+j];
			if(z<0.0) z=0;
			if(z>65535.0) z=65535.0;
			put16((int)z);
		}
		put16(65535);
	}
	return 0;
}
