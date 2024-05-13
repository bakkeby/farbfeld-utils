#if 0
gcc -s -O2 -o ~/bin/ff-dntsc -Wno-unused-result ff-dntsc.c -lm
exit
#endif

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TAU (6.283185307179586476925286766559005768394)

static unsigned char head[16];
static float fopt[127];
static int iopt[127];
static float*tab;
static float*buf;
static float y,i,q,v,phase;
static int n,s,width;

#define R(a) ((a)>=0?buf[a]:0.0)
#define C(a) (tab[((a)+iopt['t']+(int)fopt['m'])%iopt['t']])

static void
usage(void)
{
	fprintf(stderr,
		"Usage: ff-dntsc <?opt=value?> ... <?opt=value?>                               \n"
		"                                                                              \n"
		"This farbfeld program decodes NTSC colours (does not decode the frame).       \n"
		"                                                                              \n"
		"The filter takes zero or more arguments that set values for options.          \n"
		"                                                                              \n"
		"Arguments consist of a single letter or number, followed by a equal sign,     \n"
		"followed by the value. The value can be a number, which may be positive or    \n"
		"negative, and some options may include a fractional part.                     \n"
		"                                                                              \n"
		"Only the red channel of the input is used to make the colours of the output;  \n"
		"the alpha channel of input is just copied to output, while the green and blue \n"
		"channels of input are ignored.                                                \n"
		"                                                                              \n"
		"Valid options include:                                                        \n"
		"   1 to 6 = coefficients for the equation to convert YIQ to RGB               \n"
		"            (0.966882, 0.623557, -0.274788, -0.635691, -1.108545, 1.709007)   \n"
		"   a = border voltage (0.0)                                                   \n"
		"   c = total contrast amount (0.009574) (all outputs are multiplied by this)  \n"
		"   f = phase finetune (in radians) (0.0)                                      \n"
		"   h = the voltage that a white pixel represents (92.0)                       \n"
		"   i = amount of spread of the I component (24)                               \n"
		"   l = the voltage that a black pixel represents (-11.0)                      \n"
		"   m = initial phase (0.0)                                                    \n"
		"   p = phase adjust per scanline (0.0) (should be nonzero for Famicom)        \n"
		"   q = amount of spread of the Q component (24)                               \n"
		"   s = saturation amount (0.8)                                                \n"
		"   t = colour subcarrier width (12.0)                                         \n"
		"   v = quadrature shift (3/4 of the value of t)                               \n"
		"   y = amount of spread of the Y component (12.0)                             \n"
		"   z = if nonzero, the output format is YIQ instead of RGB (0)                \n"
		"                                                                              \n"
		"NB: there is no option to detect a burst signal.                              \n"
		"                                                                              \n"
		"Unclear if it is the intended usage, but you can try passing a normal image   \n"
		"through this filter for interesting effects.                                  \n"
		"                                                                              \n"
		"Example usage:                                                                \n"
		"   $ ff-dntsc < image.ff > image-ntsc.ff                                      \n"
		"\n"
	);
	exit(1);
}

int main(int argc,char**argv) {
	if (argc>1 && (!strcmp(argv[1],"-h") || !strcmp(argv[1],"--help"))) {
		usage();
	}
	fopt['1']=0.966882;
	fopt['2']=0.623557;
	fopt['3']=-0.274788;
	fopt['4']=-0.635691;
	fopt['5']=-1.108545;
	fopt['6']=1.709007;
	iopt['y']=12;
	iopt['i']=iopt['q']=24;
	fopt['y']=12.0;
	fopt['i']=fopt['q']=24.0;
	fopt['l']=-11.0;
	fopt['h']=92.0;
	fopt['c']=0.009574;
	fopt['s']=0.8;
	fopt['a']=fopt['m']=fopt['p']=fopt['f']=0.0;
	fopt['t']=12.0;
	iopt['t']=12;
	iopt['v']=0;
	iopt['z']=0;
	fread(head,1,16,stdin);
	width=(head[8]<<24)|(head[9]<<16)|(head[10]<<8)|head[11];
	for(n=1;n<argc;n++) if(argv[n][0] && argv[n][1]=='=') {
		iopt[argv[n][0]&127]=strtol(argv[n]+2,0,10);
		fopt[argv[n][0]&127]=strtof(argv[n]+2,0);
	}
	buf=malloc(width*sizeof(float));
	tab=malloc(iopt['t']*sizeof(float));
	if(!buf || !tab) {
		fprintf(stderr,"Allocation failed\n");
		return 1;
	}
	while(fopt['m']<=0.0) fopt['m']+=fopt['t'];
	while(fopt['p']<=0.0) fopt['p']+=fopt['t'];
	if(!iopt['v']) iopt['v']=(3*iopt['t'])/4;
	for(n=0;n<iopt['t'];n++) tab[n]=sinf(fopt['f']+TAU*((float)n)/fopt['t']);
	fwrite(head,1,16,stdout);
	if(iopt['z']) for(;;) {
		if(!s) y=i=q=fopt['a'];
		n=fgetc(stdin);
		if(n==EOF) break;
		n=(n<<8)|fgetc(stdin);
		buf[s]=v=(fopt['h']-fopt['l'])*((float)n)/65535.0+fopt['l'];
		fgetc(stdin); fgetc(stdin); fgetc(stdin); fgetc(stdin); // unused input channels
		y+=v-R(s-iopt['y']);
		i+=v*C(s)-R(s-iopt['i'])*C(s-iopt['i']);
		q+=v*C(s+iopt['v'])-R(s-iopt['q'])*C(s+iopt['v']-iopt['q']);
		// Red
		v=fopt['s']*y/fopt['y'];
		n=(int)fmax(fmin(v*65535.0,65535.0),0.0);
		putchar(n>>8); putchar(n);
		// Green
		v=fopt['s']*i/fopt['i'];
		n=(int)fmax(fmin(v*65535.0,65535.0),0.0);
		putchar(n>>8); putchar(n);
		// Blue
		v=fopt['s']*q/fopt['q'];
		n=(int)fmax(fmin(v*65535.0,65535.0),0.0);
		putchar(n>>8); putchar(n);
		// Alpha
		putchar(fgetc(stdin)); putchar(fgetc(stdin));
		if(!(s=(s+1)%width)) fopt['m']+=fopt['p'];
	} else for(;;) {
		if(!s) y=fopt['a'],i=q=0.0;
		n=fgetc(stdin);
		if(n==EOF) break;
		n=(n<<8)|fgetc(stdin);
		buf[s]=v=(fopt['h']-fopt['l'])*((float)n)/65535.0+fopt['l'];
		fgetc(stdin); fgetc(stdin); fgetc(stdin); fgetc(stdin); // unused input channels
		y+=v-R(s-iopt['y']);
		i+=v*C(s)-R(s-iopt['i'])*C(s-iopt['i']);
		q+=v*C(s+iopt['v'])-R(s-iopt['q'])*C(s+iopt['v']-iopt['q']);
		// Red
		v=y*fopt['c']/fopt['y']+i*fopt['1']*fopt['c']*fopt['s']/fopt['i']+q*fopt['2']*fopt['c']*fopt['s']/fopt['q'];
		n=(int)fmax(fmin(v*65535.0,65535.0),0.0);
		putchar(n>>8); putchar(n);
		// Green
		v=y*fopt['c']/fopt['y']+i*fopt['3']*fopt['c']*fopt['s']/fopt['i']+q*fopt['4']*fopt['c']*fopt['s']/fopt['q'];
		n=(int)fmax(fmin(v*65535.0,65535.0),0.0);
		putchar(n>>8); putchar(n);
		// Blue
		v=y*fopt['c']/fopt['y']+i*fopt['5']*fopt['c']*fopt['s']/fopt['i']+q*fopt['6']*fopt['c']*fopt['s']/fopt['q'];
		n=(int)fmax(fmin(v*65535.0,65535.0),0.0);
		putchar(n>>8); putchar(n);
		// Alpha
		putchar(fgetc(stdin)); putchar(fgetc(stdin));
		if(!(s=(s+1)%width)) fopt['m']+=fopt['p'];
	}
	return 0;
}
