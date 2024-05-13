#if 0
gcc -s -O2 -o ~/bin/ff-rephase -Wno-unused-result ff-rephase.c -lm
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
static char opt[128];
static unsigned char head[16];

static void
usage(void)
{
	fprintf(stderr,
		"Usage: ff-rephase <?seed?> <?options?>                                        \n"
		"                                                                              \n"
		"This farbfeld filter is used to randomize the phase of the frequency domain of\n"
		"the picture.                                                                  \n"
		"                                                                              \n"
		"The input picture must be square and have a size that is the power of 2 (e.g. \n"
		"256, 512 or 1024).                                                            \n"
		"                                                                              \n"
		"The filter takes the following optional arguments:                            \n"
		"   seed - a random seed                                                       \n"
		"   options - one or more letters combined to a string (no spaces)             \n"
		"                                                                              \n"
		"The available options are:                                                    \n"
		"   o -- seems to generate rainbow colours                                     \n"
		"   w -- simpler transformation that generates a cloud like look, negates o    \n"
		"   x -- alternate transformation that generates relief and keeps some of the  \n"
		"        original shapes (when transparency is involved)                       \n"
		"   z -- reduces value passed to sinus and cosinus curves by 50%% - tends to   \n"
		"        generate an all white image                                           \n"
		"   r -- reduce red channel strength by 50%%                                   \n"
		"   g -- reduce green channel strength by 50%%                                 \n"
		"   b -- reduce blue channel strength by 50%%                                  \n"
		"                                                                              \n"
		"The options result in different algorithms being used when rephasing the image\n"
		"and the effects are not straightforward to describe.                          \n"
		"                                                                              \n"
		"Example usage:                                                                \n"
		"   $ ff-rephase 313 w < image.ff > image-rephase.ff                           \n"
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
	int j,x,y;
	double a,z;
	double cr[3];
	if (argc>1 && (!strcmp(argv[1],"-h") || !strcmp(argv[1],"--help"))) {
		usage();
	}
	fread(head,1,16,stdin);
	if(memcmp(head+8,head+12,4)) fatal("Bad size");
	size=(head[8]<<24)|(head[9]<<16)|(head[10]<<8)|head[11];
	if(size<=0 || size>32768 || (size&(size-1))) fatal("Bad size");
	gRe=calloc(sizeof(double)*3,size*size);
	gIm=calloc(sizeof(double)*3,size*size);
	GRe=calloc(sizeof(double)*3,size*size);
	GIm=calloc(sizeof(double)*3,size*size);
	if(!gRe || !gIm || !GRe || !GIm) fatal("Out of memory");
	fwrite(head,1,16,stdout);
	for(x=size*size-1;x>=0;x--) {
		for(j=0;j<3;j++) {
			y=fgetc(stdin)<<8;
			y|=fgetc(stdin);
			gRe[3*x+j]=(double)y;
		}
		fgetc(stdin); fgetc(stdin);
	}
	FFT2D();
	srandom(argc>1?strtol(argv[1],0,0):0);
	if(argc>2) for(j=0;argv[2][j];j++) opt[argv[2][j]&127]=1;
	for(x=0;x<size/(2-opt['w']);x++) for(y=0;y<size/(2-opt['w']);y++) {
		if(opt['x'] && ((x^y)&1)) {
			for(j=0;j<3;j++) {
				gIm[3*size*y+3*x+j]=GIm[3*size*y+3*x+j];
				gRe[3*size*y+3*x+j]=GRe[3*size*y+3*x+j];
			}
		} else {
			z=TAU*((double)random())/(double)RAND_MAX;
			if(!x && !y) z=0.0; else if(opt['z']) z*=0.5;
			for(j=0;j<3;j++) {
				a=hypot(GRe[3*size*y+3*x+j],GIm[3*size*y+3*x+j]);
				gIm[3*size*y+3*x+j]=sin(z)*a;
				gRe[3*size*y+3*x+j]=cos(z)*a;
				if(opt['o'] && !opt['w']) {
					a=hypot(GRe[3*size*(y+size/2)+3*x+j],GIm[3*size*(y+size/2)+3*x+j]);
					gIm[3*size+(y+size/2)+3*x+j]=sin(z)*a;
					gRe[3*size+(y+size/2)+3*x+j]=cos(z)*a;
					a=hypot(GRe[3*size*(y+size/2)+3*(x+size/2)+j],GIm[3*size*(y+size/2)+3*(x+size/2)+j]);
					gIm[3*size+(y+size/2)+3*(x+size/2)+j]=sin(z)*a;
					gRe[3*size+(y+size/2)+3*(x+size/2)+j]=cos(z)*a;
					a=hypot(GRe[3*size*y+3*(x+size/2)+j],GIm[3*size*y+3*(x+size/2)+j]);
					gIm[3*size+y+3*(x+size/2)+j]=sin(z)*a;
					gRe[3*size+y+3*(x+size/2)+j]=cos(z)*a;
				} else if(!opt['w']) {
					gIm[3*size+(y+size/2)+3*x+j]=0.0;
					gRe[3*size+(y+size/2)+3*x+j]=0.0;
					gIm[3*size+(y+size/2)+3*(x+size/2)+j]=0.0;
					gRe[3*size+(y+size/2)+3*(x+size/2)+j]=0.0;
					gIm[3*size+y+3*(x+size/2)+j]=0.0;
					gRe[3*size+y+3*(x+size/2)+j]=0.0;
				}
			}
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
