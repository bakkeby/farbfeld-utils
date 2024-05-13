#if 0
gcc -s -O2 -o ~/bin/ff-shadow -Wno-unused-result ff-shadow.c
exit
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define fatal(...) do{ fprintf(stderr,__VA_ARGS__); fputc('\n',stderr); exit(1); }while(0)

typedef struct {
	unsigned short d[4];
} Pixel;

typedef struct {
	int v[36];
} Light;

static unsigned char buf[16];
static int width,height,nlights;
static Pixel*pic;
static unsigned short*shadow;
static Light lights[32] = {0};
static char opt[127];

#define LightV(x,y) lights[x].v[y-'a']
#define Alpha(x,y) pic[(y)*width+(x)].d[3]
#define Shadow(x,y) shadow[(y)*width+(x)]

static void
usage(void)
{
	fprintf(stderr,
		"Usage: ff-shadow <?-options?> <light> <?light?> ...                           \n"
		"                                                                              \n"
		"This farbfeld filter that generates shadows.                                  \n"
		"                                                                              \n"
		"The alpha channel is used as the depth buffer, to tell where are the objects  \n"
		"that the shadows are from.                                                    \n"
		"                                                                              \n"
		"The filter takes one or more arguments:                                       \n"
		"   options - a dash followed by one or more letters to enable options         \n"
		"   light - one or more light sources                                          \n"
		"                                                                              \n"
		"The options available are:                                                    \n"
		"   a -- by default the program will make the output picture opaque and darken \n"
		"        the parts with shadow; with the -a option the shadows are written to  \n"
		"        the alpha channel instead of darkening the picture.                   \n"
		"                                                                              \n"
		"Every following argument is a light. Each light is defined by one or more     \n"
		"light options, which consists of a letter followed by an integer (which may be\n"
		"positive, zero, or negative), with nothing in between.                        \n"
		"                                                                              \n"
		"Valid light options include:                                                  \n"
		"   b -- block shadow mode;                                                    \n"
		"        - if set to 1, drop shadows are drawn at the correct height for the   \n"
		"          object the shadow is cast on                                        \n"
		"        - if set to 2, block shadows are drawn for the height of the object   \n"
		"          the shadow is cast on                                               \n"
		"        (note that this mode is very slow to compute)                         \n"
		"   c -- ceiling of shadows, where 65535 is the default setting and draws      \n"
		"        shadows for all objects; lower numbers mean do not draw shadows for   \n"
		"        objects with a higher alpha value than the specified number           \n"
		"   i -- amount of shadow to add when this light is blocked by an object, from \n"
		"        0 to 65535; the total of all lights must not exceed 65535             \n"
		"        (not valid if b is also set)                                          \n"
		"   m -- shadow amount when this light is blocked by an object, from 0 to 65535\n"
		"        Unlike i, these shadows do not accumulate, are allowed to total more  \n"
		"        than 65535, and are valid even if b is set.                           \n"
		"   x -- X offset to draw shadows for objects with depth 65535 - may be        \n"
		"        negative for shadows to the left                                      \n"
		"   y -- Y offset to draw shadows for objects with depth 65535 - may be        \n"
		"        negative for shadows up                                               \n"
		"                                                                              \n"
		"As an example a light may be defined by an X offset, an Y offset, and an m    \n"
		"option: x-200y150m48000                                                       \n"
		"                                                                              \n"
		"A maximum of 32 lights can be specified.                                      \n"
		"                                                                              \n"
		"Example usage:                                                                \n"
		"   $ ff-shadow i42500x150y100 < image.ff > image-shadow.ff                    \n"
		"   $ ff-shadow -a i42500x150y100 < image.ff > image-shadow.ff                 \n"
		"\n"
	);
	exit(1);
}

static void process(int n) {
	int x,y,xx,yy,a,h;
	for(y=0;y<height;y++) for(x=0;x<width;x++) {
		a=Alpha(x,y);
		if(a<=LightV(n,'c')) {
			if(LightV(n,'b')) {
				for(h=0;a>=0 && h<65536;h++) {
					a--;
					xx=x+(LightV(n,'x')*(double)h)/65535.0+0.33;
					yy=y+(LightV(n,'y')*(double)h)/65535.0+0.33;
					if(x==xx && y==yy) continue;
					if(xx<0 || xx>=width || yy<0 || yy>=height) break;
					if(a==Alpha(xx,yy) || (LightV(n,'b')==2 && a>Alpha(xx,yy))) {
						if(Shadow(xx,yy)<LightV(n,'m')) Shadow(xx,yy)=LightV(n,'m');
					}
				}
			} else {
				xx=x+(LightV(n,'x')*(double)a)/65535.0+0.33;
				yy=y+(LightV(n,'y')*(double)a)/65535.0+0.33;
				if(x==xx && y==yy) continue;
				if(xx<0 || xx>=width || yy<0 || yy>=height) continue;
				if(a>Alpha(xx,yy)+LightV(n,'h')) {
					if(Shadow(xx,yy)<LightV(n,'m')) Shadow(xx,yy)=LightV(n,'m');
					Shadow(xx,yy)+=LightV(n,'i');
				}
			}
		}
	}
}

int main(int argc,char**argv) {
	int i,j;
	if (argc>1 && (!strcmp(argv[1],"-h") || !strcmp(argv[1],"--help"))) {
		usage();
	}
	while(argc>1 && argv[1][0]=='-') {
		for(i=1;argv[1][i];i++) opt[argv[1][i]&127]=1;
		argc--;
		argv++;
	}

	if(argc>32) fatal("Too many arguments");
	for(i=1;i<argc;i++) {
		char*p=argv[i];
		LightV(i-1,'c')=65535;
		while(*p) {
			if(*p<'a' || *p>'z') fatal("Invalid option");
			int o = *p;
			LightV(i-1,o)=strtol(p+1,&p,10);
		}
	}

	nlights=argc-1;
	fread(buf,1,16,stdin);
	width=(buf[8]<<24)|(buf[9]<<16)|(buf[10]<<8)|buf[11];
	height=(buf[12]<<24)|(buf[13]<<16)|(buf[14]<<8)|buf[15];
	pic=malloc(width*height*sizeof(Pixel));
	shadow=calloc(width*height,sizeof(unsigned short));
	fwrite(buf,1,16,stdout);
	for(i=0;i<width*height;i++) {
		fread(buf,1,8,stdin);
		pic[i].d[0]=(buf[0]<<8)|buf[1];
		pic[i].d[1]=(buf[2]<<8)|buf[3];
		pic[i].d[2]=(buf[4]<<8)|buf[5];
		pic[i].d[3]=(buf[6]<<8)|buf[7];
	}
	for(i=0;i<nlights;i++) process(i);
#define OutShadow(a) j=((0x10000-shadow[i])*(long long)(pic[i].d[a]))>>16,buf[a+a]=j>>8,buf[a+a+1]=j
	if(opt['a']) {
		for(i=0;i<width*height;i++) {
			buf[0]=pic[i].d[0]>>8; buf[1]=pic[i].d[0];
			buf[2]=pic[i].d[1]>>8; buf[3]=pic[i].d[1];
			buf[4]=pic[i].d[2]>>8; buf[5]=pic[i].d[2];
			buf[6]=shadow[i]>>8; buf[7]=shadow[i];
			fwrite(buf,1,8,stdout);
		}
	} else {
		for(i=0;i<width*height;i++) {
			OutShadow(0);
			OutShadow(1);
			OutShadow(2);
			buf[6]=buf[7]=255;
			fwrite(buf,1,8,stdout);
		}
	}
	return 0;
}
