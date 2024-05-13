#if 0
gcc -s -O2 -o ~/bin/ff-reduce -Wno-unused-result ff-reduce.c
exit
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
	int r;
	int g;
	int b;
} Color;

static unsigned char head[16];
static Color*row;
static int width,height,count;
static Color palette[1024];
static char cmode; // a b c d e m n s t w x y
static char dmode; // e k o q r v x y z
static int carg[16];
static int darg[16];
static int ham;

static void
usage(void)
{
	fprintf(stderr,
		"Usage: ff-reduce <nearest colour mode> <dithering> <colour> <?colour?> ...    \n"
		"                                                                              \n"
		"This farbfeld filter reduces a picture to a specified palette.                \n"
		"                                                                              \n"
		"The filter takes at least three arguments:                                    \n"
		"   - nearest colour mode; one letter, optionally followed by numeric arguments\n"
		"     each preceded by a comma (all as a single command-line argument)         \n"
		"     Precede with + for hold-and-modify                                       \n"
		"   - dithering mode; one letter, optionally followed by numeric arguments each\n"
		"     preceded by a comma (all as a single command-line argument)              \n"
		"   - the rest of the arguments are the colours (which must be opaque; the     \n"
		"     alpha value cannot be specified)                                         \n"
		"     (the maximum number of colours is 1024)                                  \n"
		"                                                                              \n"
		"Nearest colour modes:                                                         \n"
		"   a -- absolute value of differences - if arguments are given, they are the  \n"
		"        denominator and numerator of a fraction to add additional badness     \n"
		"        multiplied by the palette index                                       \n"
		"   b -- compare to previous colour                                            \n"
		"   c -- absolute value of differences - takes up to 16 arguments, it may be a \n"
		"        way to control which of the colours of the palette is used for what   \n"
		"        colours                                                               \n"
		"   d -- alternates between red, green and blue channels resulting in more     \n"
		"        detail and smoother gradients - if arguments are given then they can  \n"
		"        affect the output                                                     \n"
		"   e -- same as d, but also applies an absolute value of difference           \n"
		"   m -- determines palette to be used if the channels are greater than that of\n"
		"        the palette, takes no arguments                                       \n"
		"   n -- auto-weighted Euclidean distance, takes no arguments                  \n"
		"   s -- squared - value of differences squared - if arguments are given, they \n"
		"        are the denominator and numerator of a fraction to add additional     \n"
		"        badness multiplied by the palette index                               \n"
		"   t -- involves some multiplication of difference and bitshifting, takes up  \n"
		"        to 16 arguments, it may be a way to control which of the colours of   \n"
		"        the palette is used for what colours                                  \n"
		"   w -- weighted Euclidean distance; the three arguments are the weighting    \n"
		"        factors for red, green, blue                                          \n"
		"   x -- unclear, seems to only consider half of the palettes half of the time,\n"
		"        takes no arguments                                                    \n"
		"   y -- yliluoma; the three arguments are the weighting factors for red,      \n"
		"        green, blue                                                           \n"
		"                                                                              \n"
		"Dithering modes:                                                              \n"
		"   e -- diffusion method - takes 1 or more arguments, the first of which      \n"
		"        cannot be 0, add more arguments for greater effect (add too many and  \n"
		"        results in noise)                                                     \n"
		"   k -- bayer 8; the first argument is the amount of scanline shifting, and   \n"
		"        the next three are the right shift amounts for red, green, blue       \n"
		"   o -- takes three arguments, which are values between 0 and 65535 - use     \n"
		"        higher values to see the effect                                       \n"
		"   q -- takes three arguments, which are values between 0 and 65535 - use     \n"
		"        higher values to see the effect                                       \n"
		"   r -- random; the arguments are the modulus, bias, and mask; the first two  \n"
		"        are shared between all channels, and the third is computed separately \n"
		"        per channel                                                           \n"
		"   v -- diffusion method - takes 1 or more arguments, the first of which      \n"
		"        cannot be 0, add more arguments for greater effect (add too many and  \n"
		"        results in noise)                                                     \n"
		"   x -- diffusion method - takes 1 or more arguments, the first of which      \n"
		"        cannot be 0, add more arguments for greater effect (add too many and  \n"
		"        results in noise)                                                     \n"
		"   y -- diffusion method - takes 1 or more arguments, the first of which      \n"
		"        cannot be 0, add more arguments for greater effect (add too many and  \n"
		"        results in noise)                                                     \n"
		"   z -- no dithering                                                          \n"
		"                                                                              \n"
		"The colours can be defined through one of the following patterns:             \n"
		"   rrggbb            three pairs of 8-bit hex values                          \n"
		"   rrrrggggbbbb      three pairs of 16-bit hex values                         \n"
		"                                                                              \n"
		"Example usage:                                                                \n"
		"   $ ff-reduce d,4,1 z 3b2625 955045 c73933 ... < image.ff > image-reduce.ff  \n"
		"   $ ff-reduce d,4,1 z 3b2625 955045 c73933 ... < image.ff > image-reduce.ff  \n"
		"\n"
	);
	exit(1);
}

static Color parse_color(const char*s) {
	Color c={0};
	switch(strlen(s)) {
		case 6:
			sscanf(s,"%2x%2x%2x",&c.r,&c.g,&c.b);
			c.r*=0x101;
			c.g*=0x101;
			c.b*=0x101;
			break;
		case 12:
			sscanf(s,"%4x%4x%4x",&c.r,&c.g,&c.b);
			break;
	}
	return c;
}

static double colorcompare(Color c1,Color c2,int q) {
	double l1,l2;
	l1=(c1.r*carg[0]+c1.g*carg[1]+c1.b*carg[2])/65535000.0;
	l2=(c2.r*carg[0]+c2.g*carg[1]+c2.b*carg[2])/65535000.0;
	return ((c1.r-c2.r)*((double)(c1.r-c2.r))*(carg[0]/4294836225000.0)
				 +(c1.g-c2.g)*((double)(c1.g-c2.g))*(carg[1]/4294836225000.0)
				 +(c1.b-c2.b)*((double)(c1.b-c2.b))*(carg[2]/4294836225000.0)
	)*carg[q]+(l1-l2)*(l1-l2)*carg[q+1];
}

static long colourdistance(int r1,int g1,int b1,int r2,int g2,int b2) {
	int m=(r1+r2)>>1;
	int r=(r1-r2)*(r1-r2);
	int g=(g1-g2)*(g1-g2);
	int b=(b1-b2)*(b1-b2);
	return (((512+m)*r)>>8)+4*g+(((767-m)*b)>>8);
}

static const int bayer8[64]={
	 0*1024,32*1024, 8*1024,40*1024, 2*1024,34*1024,10*1024,42*1024,
	48*1024,16*1024,56*1024,24*1024,50*1024,18*1024,58*1024,26*1024,
	12*1024,44*1024, 4*1024,36*1024,14*1024,46*1024, 6*1024,38*1024,
	60*1024,28*1024,52*1024,20*1024,62*1024,30*1024,54*1024,22*1024,
	 3*1024,35*1024,11*1024,43*1024, 1*1024,33*1024, 9*1024,41*1024,
	51*1024,19*1024,59*1024,27*1024,49*1024,17*1024,57*1024,25*1024,
	15*1024,47*1024, 7*1024,39*1024,13*1024,45*1024, 5*1024,37*1024,
	63*1024,31*1024,55*1024,23*1024,61*1024,29*1024,53*1024,21*1024,
};

static int yliluoma1(int pos,Color cur) {
	Color out;
	int i1,i2,i3,ri,ro;
	int p[4];
	double v,m;
	p[0]=p[1]=p[2]=p[3]=0;
	ro=32;
	m=1.0e99;
	for(i1=0;i1<count;i1++) for(i2=i1;i2<count;i2++) {
		if(palette[i1].r==palette[i2].r && palette[i1].g==palette[i2].g && palette[i1].b==palette[i2].b) {
			ri=32;
			out=palette[i1];
		} else {
			ri=(palette[i1].r==palette[i2].r?0:(carg[0]*64*(cur.r-palette[i1].r))/(palette[i2].r-palette[i1].r));
			ri+=(palette[i1].g==palette[i2].g?0:(carg[1]*64*(cur.g-palette[i1].g))/(palette[i2].g-palette[i1].g));
			ri+=(palette[i1].b==palette[i2].b?0:(carg[2]*64*(cur.b-palette[i1].b))/(palette[i2].b-palette[i1].b));
			ri/=(palette[i1].r==palette[i2].r?0:carg[0])+(palette[i1].g==palette[i2].g?0:carg[1])+(palette[i1].b==palette[i2].b?0:carg[2]);
			if(ri<0) ri=0; else if(ri>63) ri=63;
			out.r=palette[i1].r+(ri*(palette[i2].r-palette[i1].r))/64;
			out.g=palette[i1].g+(ri*(palette[i2].r-palette[i1].g))/64;
			out.b=palette[i1].b+(ri*(palette[i2].b-palette[i1].b))/64;
		}
		v=colorcompare(cur,out,3)+colorcompare(palette[i1],palette[i2],5);
		if(v<m) {
			m=v;
			p[0]=i1;
			p[1]=i2;
			ro=ri;
		}
		if(i1!=i2 && carg[7]) for(i3=0;i3<count;i3++) {
			out.r=(palette[i1].r+palette[i2].r+palette[i3].r*2)>>2;
			out.g=(palette[i1].g+palette[i2].g+palette[i3].g*2)>>2;
			out.b=(palette[i1].b+palette[i2].b+palette[i3].b*2)>>2;
			v=colorcompare(cur,out,7)+colorcompare(palette[i1],palette[i2],9);
			out.r=(palette[i1].r+palette[i2].r)>>1;
			out.g=(palette[i1].g+palette[i2].g)>>1;
			out.b=(palette[i1].b+palette[i2].b)>>1;
			v+=colorcompare(out,palette[i3],9)*(abs(ri-32)/64.0+0.5);
			if(v<m) {
				m=v;
				p[0]=i3;
				p[1]=i1;
				p[2]=i2;
				p[3]=i3;
				ro=-1;
			}
		}
	}
	if(ro==-1) {
		return p[((height&1)<<1)|(pos&1)];
	} else {
		ro*=1024;
		return p[bayer8[(pos&7)|((height&7)<<3)]<ro?1:0];
	}
}

#define Diffuse(x,y) \
	row[pos+(x)].r+=((cur.r-palette[p].r)*darg[y])/darg[0], \
	row[pos+(x)].g+=((cur.g-palette[p].g)*darg[y])/darg[0], \
	row[pos+(x)].b+=((cur.b-palette[p].b)*darg[y])/darg[0], 0
static inline void process(int pos) {
	Color cur,cur2;
	int i,p;
	long long v,m;
	cur.r=fgetc(stdin)<<8;
	cur.r|=fgetc(stdin);
	cur.g=fgetc(stdin)<<8;
	cur.g|=fgetc(stdin);
	cur.b=fgetc(stdin)<<8;
	cur.b|=fgetc(stdin);
	if(fgetc(stdin)&0x80) {
		fgetc(stdin);
	} else {
		fgetc(stdin);
		for(i=0;i<8;i++) putchar(0);
		return;
	}
	if(cmode=='b') cur2=cur;
	if(ham) {
		count=ham+(pos?3:0);
		palette[ham+0].r=cur.r;
		palette[ham+1].g=cur.g;
		palette[ham+2].b=cur.b;
	}
	switch(dmode) {
		case 'e':
			cur.r+=row[pos].r;
			cur.g+=row[pos].g;
			cur.b+=row[pos].b;
			if(darg[6]) {
				if(cur.r>darg[6]) cur.r=darg[6];
				if(cur.g>darg[6]) cur.g=darg[6];
				if(cur.b>darg[6]) cur.b=darg[6];
				if(cur.r<darg[7]) cur.r=darg[7];
				if(cur.g<darg[7]) cur.g=darg[7];
				if(cur.b<darg[7]) cur.b=darg[7];
			}
			break;
		case 'k':
			pos+=*darg*height;
			cur.r+=bayer8[(pos&7)|((height&7)<<3)]>>darg[1];
			cur.g+=bayer8[(pos&7)|((height&7)<<3)]>>darg[2];
			cur.b+=bayer8[(pos&7)|((height&7)<<3)]>>darg[3];
			pos-=*darg*height;
			break;
		case 'o':
			cur.r+=darg[(pos&3)|((height&3)<<2)];
			cur.g+=darg[(pos&3)|((height&3)<<2)];
			cur.b+=darg[(pos&3)|((height&3)<<2)];
			break;
		case 'q':
			cur.r+=darg[(pos&3)|((height&3)<<2)];
			cur.g+=darg[((pos+1)&3)|((height&3)<<2)];
			cur.b+=darg[(pos&3)|(((height+1)&3)<<2)];
			break;
		case 'r':
			v=random()%darg[0]-darg[1];
			cur.r+=v+(darg[2]&random());
			cur.g+=v+(darg[2]&random());
			cur.b+=v+(darg[2]&random());
			break;
		case 'v':
			cur.r+=row[pos].r;
			cur.g+=row[pos].g;
			cur.b+=row[pos].b;
			break;
		case 'x':
			cur.r+=row[pos].r;
			cur.g+=row[pos].g;
			cur.b+=row[pos].b;
			break;
		case 'y':
			cur.r+=row[pos].r>>darg[6];
			cur.g+=row[pos].g>>darg[7];
			cur.b+=row[pos].b>>darg[8];
			break;
	}
	p=0;
	m=0x4000000000000LL;
	switch(cmode) {
		case 'a':
			for(i=0;i<count;i++) {
				v=abs(cur.r-palette[i].r)+abs(cur.g-palette[i].g)+abs(cur.b-palette[i].b);
				if(*carg) v+=(i/carg[0])*carg[1];
				if(v<m) m=v,p=i;
			}
			break;
		case 'b':
			for(i=0;i<count;i++) {
				v=(cur2.r+carg[2])*(long long)abs(cur.r-palette[i].r)+(cur2.g+carg[3])*(long long)abs(cur.g-palette[i].g)+(cur2.b+carg[4])*(long long)abs(cur.b-palette[i].b);
				if(*carg) v+=(i/carg[0])*carg[1]*(long long)abs(carg[1]);
				if(v<m) m=v,p=i;
			}
			break;
		case 'c':
			for(i=0;i<count;i++) {
				v=abs(cur.r-palette[i].r)+abs(cur.g-palette[i].g)+abs(cur.b-palette[i].b);
				v*=(long long)carg[i&15];
				if(v<m) m=v,p=i;
			}
			break;
		case 'd':
			for(i=0;i<count;i++) {
				if((pos+*carg*height)%3==0) v=abs(cur.r-palette[i].r);
				else if((pos+*carg*height)%3==1) v=abs(cur.g-palette[i].g);
				else if((pos+*carg*height)%3==2) v=abs(cur.b-palette[i].b);
				if(v<m) m=v,p=i;
			}
			break;
		case 'e':
			for(i=0;i<count;i++) {
				if((pos+*carg*height)%3==0) v=abs(cur.r-palette[i].r);
				else if((pos+*carg*height)%3==1) v=abs(cur.g-palette[i].g);
				else if((pos+*carg*height)%3==2) v=abs(cur.b-palette[i].b);
				v*=carg[1]+0x8000;
				v+=abs(cur.r-palette[i].r)+abs(cur.g-palette[i].g)+abs(cur.b-palette[i].b);
				if(v<m) m=v,p=i;
			}
			break;
		case 'm':
			for(i=0;i<count;i++) {
				v=(cur.r>=palette[i].r?cur.r-palette[i].r:0x100000)+(cur.g>=palette[i].g?cur.g-palette[i].g:0x100000)+(cur.b>=palette[i].b?cur.b-palette[i].b:0x100000);
				if(v<m) m=v,p=i;
			}
			break;
		case 'n':
			for(i=0;i<count;i++) {
				v=colourdistance(cur.r>>8,cur.g>>8,cur.b>>8,palette[i].r>>8,palette[i].g>>8,palette[i].b>>8);
				if(v<m) m=v,p=i;
			}
			break;
		case 's':
			for(i=0;i<count;i++) {
				v=(cur.r-palette[i].r)*((long long)(cur.r-palette[i].r))+(cur.g-palette[i].g)*((long long)(cur.g-palette[i].g))+(cur.b-palette[i].b)*((long long)(cur.b-palette[i].b));
				if(*carg) v+=(i/carg[0])*carg[1]*(long long)abs(carg[1]);
				if(v<m) m=v,p=i;
			}
			break;
		case 't':
			for(i=0;i<count;i++) {
				v=(cur.r-palette[i].r)*((long long)(cur.r-palette[i].r))+(cur.g-palette[i].g)*((long long)(cur.g-palette[i].g))+(cur.b-palette[i].b)*((long long)(cur.b-palette[i].b));
				v>>=9;
				v*=(long long)carg[i&15];
				if(v<m) m=v,p=i;
			}
			break;
		case 'w':
			for(i=0;i<count;i++) {
				v=carg[0]*(cur.r-palette[i].r)*((long long)(cur.r-palette[i].r))+carg[1]*(cur.g-palette[i].g)*((long long)(cur.g-palette[i].g))+carg[2]*(cur.b-palette[i].b)*((long long)(cur.b-palette[i].b));
				if(v<m) m=v,p=i;
			}
			break;
		case 'x':
			for(i=((pos^height)&1?count/2:0);i<((pos^height)&1?count:count/2);i++) {
				v=(cur.r-palette[i].r)*((long long)(cur.r-palette[i].r))+(cur.g-palette[i].g)*((long long)(cur.g-palette[i].g))+(cur.b-palette[i].b)*((long long)(cur.b-palette[i].b));
				if(v<m) m=v,p=i;
			}
			break;
		case 'y':
			p=yliluoma1(pos,cur);
			break;
	}
	putchar(palette[p].r>>8);
	putchar(palette[p].r);
	putchar(palette[p].g>>8);
	putchar(palette[p].g);
	putchar(palette[p].b>>8);
	putchar(palette[p].b);
	putchar(255);
	putchar(255);
	if(ham) palette[ham+0]=palette[ham+1]=palette[ham+2]=palette[p];
	switch(dmode) {
		case 'e':
			Diffuse(width,3);
			if(pos<width-1) Diffuse(1,1),Diffuse(width+1,4);
			if(pos<width-2) Diffuse(2,5);
			if(pos) Diffuse(width-1,2);
			break;
		case 'v':
			Diffuse(width,height&1?8:3);
			if(pos<width-1) Diffuse(1,height&1?6:1),Diffuse(width+1,height&1?9:4);
			if(pos<width-2) Diffuse(2,height&1?10:5);
			if(pos) Diffuse(width-1,height&1?7:2);
			break;
		case 'x':
			Diffuse(width,(pos^height)&1?8:3);
			if(pos<width-1) Diffuse(1,(pos^height)&1?6:1),Diffuse(width+1,(pos^height)&1?9:4);
			if(pos<width-2) Diffuse(2,(pos^height)&1?10:5);
			if(pos) Diffuse(width-1,(pos^height)&1?7:2);
			break;
		case 'y':
			if((height&1) && !((pos+height/2)&1)) {
				Diffuse(width,3);
				if(pos<width-1) Diffuse(1,1),Diffuse(width+1,4);
				if(pos<width-3) Diffuse(3,5);
				if(pos) Diffuse(width-1,2);
			}
			break;
	}
}

int main(int argc,char**argv) {
	int i;
	char*s;
	if (argc>1 && (!strcmp(argv[1],"-h") || !strcmp(argv[1],"--help"))) {
		usage();
	}
	count=argc-3;
	if(count<1 || count>1024 || !argv[1][0] || !argv[2][0]) {
		fprintf(stderr,"Wrong number of arguments\n");
		return 1;
	}
	cmode=argv[1][0];
	s=argv[1]+1;
	if(cmode=='+') cmode=argv[1][1],s++;
	i=0;
	while(*s==',' && i<16) carg[i++]=strtol(s+1,&s,0);
	dmode=argv[2][0];
	s=argv[2]+1;
	i=0;
	while(*s==',' && i<16) darg[i++]=strtol(s+1,&s,0);
	for(i=0;i<count;i++) palette[i]=parse_color(argv[i+3]);
	if(argv[1][0]=='+') {
		if(count>1020) count=1020;
		ham=count;
	}
	fread(head,1,16,stdin);
	width=(head[8]<<24)|(head[9]<<16)|(head[10]<<8)|head[11];
	height=(head[12]<<24)|(head[13]<<16)|(head[14]<<8)|head[15];
	row=calloc(sizeof(Color),width*2);
	if(!row) {
		fprintf(stderr,"Allocation failed\n");
		return 1;
	}
	fwrite(head,1,16,stdout);
	while(height--) {
		for(i=0;i<width;i++) process(i);
		memcpy(row,row+width,width*sizeof(Color));
		memset(row+width,0,width*sizeof(Color));
	}
	return 0;
}
