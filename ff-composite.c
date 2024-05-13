#if 0
gcc -s -O2 -o ~/bin/ff-composite ff-composite.c
exit
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
	unsigned short r,g,b,a;
} Pixel;

static unsigned char buf[16];
static Pixel*data;
static Pixel data2;
static int width,height,xpos,ypos,mode;
static FILE*infile;

static void
usage(void)
{
	fprintf(stderr,
		"Usage: ff-composite <image.ff> <x,y,mode> <?image2.ff?> <?x,y,mode?> ...      \n"
		"                                                                              \n"
		"This farbfeld filter composes one or more pictures over a background picture. \n"
		"                                                                              \n"
		"The input is the background picture. Each picture to add is specified as a    \n"
		"pair of arguments, where the first argument of the pair is the picture to add \n"
		"and the second is three numbers with commas in between.                       \n"
		"                                                                              \n"
		"The first two numbers being the position in the background to place the       \n"
		"top-left of the added picture, and the third being the mode.                  \n"
		"                                                                              \n"
		"The valid modes are:                                                          \n"
		"    0 -- overlay the picture in the ordinary way, using the alpha channel to  \n"
		"         determine what parts of the background are let through               \n"
		"    1 -- copy all four channels of the source to the target; the background is\n"
		"         not let through at all                                               \n"
		"    2 -- like mode 0, but the alpha channel of the target is not altered      \n"
		"    3 -- add the values together, clipping                                    \n"
		"    4 -- add the values together, wrapping, and ignoring alpha                \n"
		"    5 -- multiply the values together, ignoring alpha                         \n"
		"    6 -- add the values together, wrapping                                    \n"
		"    7 -- multiply the values together                                         \n"
		"    8 -- average of all four channels                                         \n"
		"    9 -- copy the red channel only; leave the rest alone                      \n"
		"   10 -- copy the green channel only; leave the rest alone                    \n"
		"   11 -- copy the blue channel only; leave the rest alone                     \n"
		"   12 -- copy the alpha channel only; leave the rest alone                    \n"
		"   13 -- copy the red channel only with alpha applied; leave the rest alone   \n"
		"   14 -- copy the green channel only with alpha applied; leave the rest alone \n"
		"   15 -- copy the blue channel only with alpha applied; leave the rest alone  \n"
		"   16 -- subtract all values, with clipping                                   \n"
		"   17 -- bitwise XOR the values, except alpha                                 \n"
		"   18 -- bitwise OR the values, including alpha                               \n"
		"   19 -- absolute value of difference, not including alpha                    \n"
		"   20 -- colour negation - background channels are divided by picture channels\n"
		"   21 -- colour negation - picture channels are divided by background channels\n"
		"   22 -- like mode 1, but the alpha is the average of the red, green and blue \n"
		"         channels and the two (source and target) alphas multiplied           \n"
		"   23 -- pixels with matching colours (not counting the alpha channel) are    \n"
		"         made transparent                                                     \n"
		"   24 -- uses the alpha channel as a depth buffer (like mode 25), but same    \n"
		"         depth is tied in favour of the background picture. Also, instead of  \n"
		"         copying the colour, it is combining similar to symbol recolouring in \n"
		"         Magic Set Editor.                                                    \n"
		"   25 -- overlay using the alpha channel as the depth buffer instead of       \n"
		"         transparency. Zero is the back depth and 65535 is the front depth.   \n"
		"         Same depth is tied in favour of the added picture.                   \n"
		"   26 -- quaternion multiplication, with wrapping - the alpha channel is      \n"
		"         treated as the scalar component                                      \n"
		"   27 -- quaternion multiplication, with clipping - the alpha channel is      \n"
		"         treated as the scalar component                                      \n"
		"                                                                              \n"
		"Example usage:                                                                \n"
		"   $ ff-composite 1.ff 50,50,0 2.ff 100,50,0 < image.ff > image-composite.ff  \n"
		"\n"
	);
	exit(1);
}

static inline unsigned short get16(FILE*fp) {
	int x=fgetc(fp)<<8;
	return x|fgetc(fp);
}

static inline void put16(int x) {
	putchar(x>>8);
	putchar(x&255);
}

static void process(void) {
	int i,j,k,fwidth,fheight,twidth,theight,offset;
	long q;
	i=fread(buf,1,16,infile);
	offset=0;
	fwidth=twidth=(buf[8]<<24)|(buf[9]<<16)|(buf[10]<<8)|buf[11];
	fheight=theight=(buf[12]<<24)|(buf[13]<<16)|(buf[14]<<8)|buf[15];
	if(xpos<0) twidth+=xpos,offset-=xpos,xpos=0;
	if(ypos<0) theight+=ypos,offset-=fwidth*ypos,ypos=0;
	if(fwidth+xpos>width) twidth=width;
	if(fheight+ypos>height) theight=height;
	if(twidth<=0 || theight<=0) return;
	k=offset<<3; while(k--) fgetc(infile); //fseek(infile,offset<<3,SEEK_CUR);
	for(i=0;i<theight;i++) {
		for(j=0;j<twidth;j++) {
			data2.r=get16(infile);
			data2.g=get16(infile);
			data2.b=get16(infile);
			data2.a=get16(infile);
			k=xpos+j+(ypos+i)*width;
			switch(mode>7?mode:mode==3?3:mode&5) {
				case 0:
					q=(65535*(long)data2.a+data[k].a*(long)(65535-data2.a))/65535?:1;
					data[k].r=(data2.r*(long)data2.a+(data[k].r*(long)data[k].a*(long)(65535-data2.a))/65535)/q;
					data[k].g=(data2.g*(long)data2.a+(data[k].g*(long)data[k].a*(long)(65535-data2.a))/65535)/q;
					data[k].b=(data2.b*(long)data2.a+(data[k].b*(long)data[k].a*(long)(65535-data2.a))/65535)/q;
					if(!(mode&2)) data[k].a=q;
					break;
				case 1:
					data[k]=data2;
					break;
				case 3:
					data[k].r=data[k].r+data2.r>65535?65535:data[k].r+data2.r;
					data[k].g=data[k].g+data2.g>65535?65535:data[k].g+data2.g;
					data[k].b=data[k].b+data2.b>65535?65535:data[k].b+data2.b;
					data[k].a=data[k].a+data2.a>65535?65535:data[k].a+data2.a;
					break;
				case 4:
					data[k].r+=data2.r;
					data[k].g+=data2.g;
					data[k].b+=data2.b;
					if(mode&2) data[k].a+=data2.a;
					break;
				case 5:
					data[k].r=(data2.r*(long)data[k].r)/65535;
					data[k].g=(data2.g*(long)data[k].g)/65535;
					data[k].b=(data2.b*(long)data[k].b)/65535;
					if(mode&2) data[k].a=(data2.a*(long)data[k].a)/65535;
					break;
				case 8:
					data[k].r=(data[k].r+data2.r)>>1;
					data[k].g=(data[k].g+data2.g)>>1;
					data[k].b=(data[k].b+data2.b)>>1;
					data[k].a=(data[k].a+data2.a)>>1;
					break;
				case 9:
					data[k].r=data2.r;
					break;
				case 10:
					data[k].g=data2.g;
					break;
				case 11:
					data[k].b=data2.b;
					break;
				case 12:
					data[k].a=data2.a;
					break;
				case 13:
					q=(65535*(long)data2.a+data[k].a*(long)(65535-data2.a))/65535?:1;
					data[k].r=(data2.r*(long)data2.a+(data[k].r*(long)data[k].a*(long)(65535-data2.a))/65535)/q;
					break;
				case 14:
					q=(65535*(long)data2.a+data[k].a*(long)(65535-data2.a))/65535?:1;
					data[k].g=(data2.g*(long)data2.a+(data[k].g*(long)data[k].a*(long)(65535-data2.a))/65535)/q;
					break;
				case 15:
					q=(65535*(long)data2.a+data[k].a*(long)(65535-data2.a))/65535?:1;
					data[k].b=(data2.b*(long)data2.a+(data[k].b*(long)data[k].a*(long)(65535-data2.a))/65535)/q;
					break;
				case 16:
					data[k].r=data[k].r-data2.r<0?0:data[k].r-data2.r;
					data[k].g=data[k].g-data2.g<0?0:data[k].g-data2.g;
					data[k].b=data[k].b-data2.b<0?0:data[k].b-data2.b;
					data[k].a=data[k].a-data2.a<0?0:data[k].a-data2.a;
					break;
				case 17:
					data[k].r^=data2.r;
					data[k].g^=data2.g;
					data[k].b^=data2.b;
					break;
				case 18:
					data[k].r|=data2.r;
					data[k].g|=data2.g;
					data[k].b|=data2.b;
					data[k].a|=data2.a;
					break;
				case 19:
					data[k].r=data[k].r>data2.r?data[k].r-data2.r:data2.r-data[k].r;
					data[k].g=data[k].g>data2.g?data[k].g-data2.g:data2.g-data[k].g;
					data[k].b=data[k].b>data2.b?data[k].b-data2.b:data2.b-data[k].b;
					break;
				case 20:
					if(data2.a) {
						data[k].r=data2.r?(data[k].r*65535LL)/data2.r:0;
						data[k].g=data2.g?(data[k].g*65535LL)/data2.g:0;
						data[k].b=data2.b?(data[k].b*65535LL)/data2.b:0;
					}
					break;
				case 21:
					if(data2.a) {
						data[k].r=data[k].r?(data2.r*65535LL)/data[k].r:0;
						data[k].g=data[k].g?(data2.g*65535LL)/data[k].g:0;
						data[k].b=data[k].b?(data2.b*65535LL)/data[k].b:0;
					}
					break;
				case 22:
					q=(data2.r+data2.g+data2.b+data2.a*(long long)data[k].a)/65535;
					data[k].r=data2.r;
					data[k].g=data2.g;
					data[k].b=data2.b;
					data[k].a=q>65535?65535:q;
					break;
				case 23:
					if(data[k].r==data2.r && data[k].g==data2.g && data[k].b==data2.b) data[k].a=0;
					break;
				case 24:
					if(data[k].a<data2.a) {
						data[k].a=data2.a;
						long long nw=data2.r<data2.b?data2.r:data2.b;
						long long dark=6969LL*data[k].r+23434LL*data[k].g+2365LL*data[k].b<838860888?65535:0;
						if(data2.g<nw) nw=data2.g;
						long long nr=data2.r-nw;
						long long ng=data2.g-nw;
						long long nb=data2.b-nw;
						int total=nr+ng+nb+nw;
						if(total<65535) total=65535;
						data[k].r=(nw*nw+nr*data[k].r+ng*(65535-dark)+nb*dark)/total;
						data[k].g=(nw*nw+nr*data[k].g+ng*(65535-dark)+nb*dark)/total;
						data[k].b=(nw*nw+nr*data[k].b+ng*(65535-dark)+nb*dark)/total;
					}
					break;
				case 25:
					if(data[k].a<=data2.a) data[k]=data2;
					break;
				case 26:
					if(1) {
						long long a=(data[k].a*(long)data2.a)-(data[k].r*(long)data2.r)-(data[k].g*(long)data2.g)-(data[k].b*(long)data2.b);
						long long r=(data[k].a*(long)data2.r)+(data[k].r*(long)data2.a)+(data[k].g*(long)data2.b)-(data[k].b*(long)data2.g);
						long long g=(data[k].a*(long)data2.g)-(data[k].r*(long)data2.b)+(data[k].g*(long)data2.a)+(data[k].b*(long)data2.r);
						long long b=(data[k].a*(long)data2.b)+(data[k].r*(long)data2.g)-(data[k].g*(long)data2.r)+(data[k].b*(long)data2.a);
						data[k].r=r;
						data[k].g=g;
						data[k].b=b;
						data[k].a=a;
					}
					break;
				case 27:
					if(1) {
						long long a=(data[k].a*(long)data2.a)-(data[k].r*(long)data2.r)-(data[k].g*(long)data2.g)-(data[k].b*(long)data2.b);
						long long r=(data[k].a*(long)data2.r)+(data[k].r*(long)data2.a)+(data[k].g*(long)data2.b)-(data[k].b*(long)data2.g);
						long long g=(data[k].a*(long)data2.g)-(data[k].r*(long)data2.b)+(data[k].g*(long)data2.a)+(data[k].b*(long)data2.r);
						long long b=(data[k].a*(long)data2.b)+(data[k].r*(long)data2.g)-(data[k].g*(long)data2.r)+(data[k].b*(long)data2.a);
						data[k].r=r<0?0:r/65535>65535?65535:r/65535;
						data[k].g=g<0?0:g/65535>65535?65535:g/65535;
						data[k].b=b<0?0:b/65535>65535?65535:b/65535;
						data[k].a=a<0?0:a/65535>65535?65535:a/65535;
					}
					break;
				default:
					if(mode&64) {
						signed long long r,g,b,s,d,c,e,i;
						if((mode&128) && !data2.a) break;
						c=mode&1024?0:data[k].a*(65535LL-data2.a); e=mode&512?0:(65535LL-data[k].a)*data2.a; i=mode&256?0:data[k].a*(long long)data2.a;
#define FF(A,z) s=data2.A; d=data[k].A; A=(long long)(z); A=A>65535?65535:A<0?0:A; A=(c*d+e*s+i*A+32767)/65535;
//  A=(long long)(z); A=((A>65535?65535:A<0?0:A)*data2.a*data[k].a+(mode&512?0:data2.a*(65535LL-data[k].a)*s)+(mode&1024?0:data[k].a*(65535LL-data2.a)*d))/4294836225;
#define F(n,z) case n: FF(r,z); FF(g,z); FF(b,z); break
						switch(mode&63) {
							default: // this avoids the use of uninitialized variables
							F(0,s);
							F(1,d);
							F(2,0);
							F(3,s+d);
							F(4,(s*d)/65535);
							F(5,s+d-(s*d)/65535);
							F(6,s<d?s:d);
							F(7,s>d?s:d);
							F(8,d+d<data[k].a?(2*s*d)/65535:65535-(2*(65535-s)*(65535-d))/65535);
							F(9,(s*s)/(65535*(65536-d)));
							F(10,(d*d)/(65535*(65536-s)));
							F(11,s+d-65535);
							F(12,s+d+d-65536);
							F(13,s+s+d-65536);
							F(14,s+d>65535?65535:(65535*d)/(65536-s));
							F(15,s+d<65536?0:((s+d-1)*65536)/(s+1));
							F(16,s>d?s-d:d-s);
							F(17,s<32768?(2*s*d)/65535:65535-(2*(65535-s)*(65535-d))/65535);
							F(18,s+d-(2*s*d)/65535);
							F(19,s<32768?(65535*(65536-d))/(2*s+1):(65535*d)/(2*(65536-s)));
							F(20,d<s+s-65536?s+s-65536:d>s+s?d:s+s);
							F(21,65536-(65535*(65536-s))/(d+1));
							F(22,65536-(65535*(65536-d))/(s+1));
							F(23,d-s);
							F(24,(s+d)&65535);
							F(25,(s+d)&32768);
							F(26,s<data[k].a?s:data[k].a);
							F(27,s>data[k].a?s:data[k].a);
							F(28,s-d);
							F(29,d+(s*data[k].a)/65536-data2.a);
							F(30,(s>d?d-s:s-d)+65535);
							F(31,(2*s*d+s*s+(s*s*d)/32768)/65535);
							F(32,(2*s*d+d*d+(s*d*d)/32768)/65535);
							F(33,65535-((65535-s)*(65535-s))/(65535*d+1));
							F(34,65535-((65535-d)*(65535-d))/(65535*s+1));
							F(35,(d*data2.r)/65535+(data2.g-32768)*2+(data2.b*(long long)data[k].a)/65535);
							F(36,(d*data2.a+s*data[k].a)/131070);
							F(37,(s*(data[k].r+data[k].g+data[k].b)+1)/(d*3+1));
							F(38,d+((3*d-data[k].r-data[k].g-data[k].b)*(s-32768))/65535);
							F(39,s+d-32768);
							F(40,d-s+32768);
						}
#undef FF
#undef F
						if(data[k].a=(c+e+i>4294836225?4294836225:c+e+i)/65535) r/=data[k].a,g/=data[k].a,b/=data[k].a;
						data[k].r=r>65535?65535:r<0?0:r;
						data[k].g=g>65535?65535:g<0?0:g;
						data[k].b=b>65535?65535:b<0?0:b;
						//data[k].a=((mode&256?0:data[k].a*(long long)data2.a)+(mode&512?0:data2.a*(65535LL-data[k].a))+(mode&1024?0:data[k].a*(65535LL-data2.a)))/65535;
					}
			}
		}
		k=(fwidth-twidth)<<3; while(k--) fgetc(infile); //fseek(infile,(fwidth-twidth)<<3,SEEK_CUR);
	}
}

int main(int argc,char**argv) {
	int i;
	if (argc>1 && (!strcmp(argv[1],"-h") || !strcmp(argv[1],"--help"))) {
		usage();
	}
	i=fread(buf,1,16,stdin);
	i=fwrite(buf,1,16,stdout);
	width=(buf[8]<<24)|(buf[9]<<16)|(buf[10]<<8)|buf[11];
	height=(buf[12]<<24)|(buf[13]<<16)|(buf[14]<<8)|buf[15];
	data=malloc(width*height*sizeof(Pixel));
	if(!data) {
		fprintf(stderr,"Out of memory\n");
		return 1;
	}
	for(i=0;i<width*height;i++) {
		data[i].r=get16(stdin);
		data[i].g=get16(stdin);
		data[i].b=get16(stdin);
		data[i].a=get16(stdin);
	}
	for(i=1;i<argc-1;i+=2) {
		xpos=ypos=mode=0;
		sscanf(argv[i+1],"%i,%i,%i",&xpos,&ypos,&mode);
		infile=fopen(argv[i],"rb");
		if(!infile) {
			fprintf(stderr,"Cannot open file: %s\n",argv[i]);
			return 1;
		}
		process();
		fclose(infile);
	}
	for(i=0;i<width*height;i++) {
		put16(data[i].r);
		put16(data[i].g);
		put16(data[i].b);
		put16(data[i].a);
	}
	return 0;
}

