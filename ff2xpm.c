#if 0
gcc -s -O2 -o ~/bin/ff2xpm -Wno-unused-result ff2xpm.c
exit
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
	unsigned char data[6];
	unsigned char code[2];
} Color;

typedef struct {
	unsigned char w,h;
	char*data;
} Stipple;

static unsigned char buf[8];
static Color palette[4096];
static const char*symbolic[4097];
static Stipple*stipple[2048];
static int noneindex,npal,form,codesize;
static int width,height,size;
static unsigned short*data;
static char opt[128];
static char*name="picture";

static void
usage(void)
{
	fprintf(stderr,
		"Usage: ff2xpm <?picture name?> <?options...?> <?palette...?>                  \n"
		"                                                                              \n"
		"Converts farbfeld to X pixmap image format. ff2xpm reads a farbfeld image from\n"
		"stdin, converts it to X pixmap image and writes the result to stdout.         \n"
		"                                                                              \n"
		"This program makes a X pixmap image (XPM) output.                             \n"
		"                                                                              \n"
		"The arguments are:                                                            \n"
		"   - Picture name, with a equal sign at front (the default is \"picture\").   \n"
		"     Not used if format 2 is specified, and should be a valid C identifier.   \n"
		"   - Options                                                                  \n"
		"   - Palette (multiple arguments)                                             \n"
		"                                                                              \n"
		"All arguments are optional, but if the colours are specified, then the options\n"
		"must also be specified. Also, the order must be as listed above.              \n"
		"                                                                              \n"
		"Options:                                                                      \n"
		"   1 -- Select format 1                                                       \n"
		"   2 -- Select format 2                                                       \n"
		"   3 -- Select format 3 (default)                                             \n"
		"   m -- Include a monochrome palette (as well as colours). Only valid if the  \n"
		"        palette is specified via command-line arguments, and not for format 1.\n"
		"   s -- Include symbolic names. Only valid if the palette is specified via    \n"
		"        command-line arguments, and not for format 1.                         \n"
		"   t -- Include stipples. Same restrictions apply as m and mutually exclusive \n"
		"        with m. In addition, encoding of each colour must be only one         \n"
		"        character rather than two. This is used to specify a colour picture   \n"
		"        that the monochrome version is to be stippled instead of solid.       \n"
		"                                                                              \n"
		"The palette consists of, for each colour, two or three arguments:             \n"
		"   - Encoding, which is one or two characters. All encodings must be the same \n"
		"     length, and all must be unique. Should not include any quotation marks or\n"
		"     backslashes (if it does, they will be escaped in format 1 and 3, although\n"
		"     other programs are not guaranteed to be able to read it in this case, and\n"
		"     this also makes it inconvenient to edit with a text editor).             \n"
		"   - Colour. You can also specify . by itself for transparency. If option m is\n"
		"     set, must be preceded by + for white or - for black.                     \n"
		"   - Stipple pattern; only included if option t is set. Use 0 or b or - for   \n"
		"     black pixels, and use 1 or w or + for white pixels. Use / or : or ,      \n"
		"     between rows, all of which must be the same size.                        \n"
		"   - Symbolic name; only included if option s is set. Must not include any    \n"
		"     spaces, quotation marks, or backslashes.                                 \n"
		"                                                                              \n"
		"If the palette is not specified, it will automatically make it up, encoding   \n"
		"each pixel as one or two base64 characters, except for transparent pixels     \n"
		"which are encoded as spaces.                                                  \n"
		"                                                                              \n"
		"Example usage:                                                                \n"
		"   $ ff2xpm < image.ff > image.xpm                                            \n"
		"\n"
	);
	exit(1);
}

static inline void outch(unsigned char x) {
	if(form!=2 && (x=='\\' || x=='"')) putchar('\\');
	putchar(x);
}

static void define_color(const char*a,const char*b) {
	if(!a || !b) a=b="";
	if(npal==4096) {
		fprintf(stderr,"Too many colors\n");
		exit(1);
	}
	memcpy(palette[npal].code,a,2);
	if(opt['m']) {
		if(*b=='+') {
			palette[npal].code[1]|=128;
		} else if(*b!='-' && *b!='.') {
			fprintf(stderr,"Invalid color\n");
			exit(1);
		}
		b++;
	}
	switch(strlen(b)) {
		case 6:
			sscanf(b,"%2hhX%2hhX%2hhX",palette[npal].data+0,palette[npal].data+2,palette[npal].data+4);
			palette[npal].data[1]=palette[npal].data[0];
			palette[npal].data[3]=palette[npal].data[2];
			palette[npal].data[5]=palette[npal].data[4];
			break;
		case 12:
			sscanf(b,"%2hhX%2hhX%2hhX%2hhX%2hhX%2hhX",palette[npal].data+0,palette[npal].data+1,palette[npal].data+2,palette[npal].data+3,palette[npal].data+4,palette[npal].data+5);
			break;
		default:
			if((*b=='.' || !*b) && noneindex==-1) {
				noneindex=npal;
			} else {
				fprintf(stderr,"Invalid color\n");
				exit(1);
			}
	}
	npal++;
}

static void define_stipple(const char*a) {
	Stipple*s=malloc(sizeof(Stipple));
	char*p;
	int i;
	if(!a) goto invalid;
	if(!s || !(p=s->data=strdup(a))) {
		fprintf(stderr,"Allocation failed\n");
		exit(1);
	}
	s->w=i=0;
	s->h=1;
	while(*a) {
		switch(*a++) {
			case '0': case 'b': case '-':
				*p++=0;
				i++;
				break;
			case '1': case 'w': case '+':
				*p++=1;
				i++;
				break;
			case '/': case ':': case ',': case ';': case ' ':
				s->h++;
				if(s->w && i!=s->w) goto invalid;
				if(!i) goto invalid;
				s->w=i;
				i=0;
				break;
			default: goto invalid;
		}
	}
	if(!s->w) s->w=i;
	if(i!=s->w || !i) goto invalid;
	stipple[npal>>1]=s;
	palette[npal]=palette[npal-1];
	palette[npal-1].code[1]=':';
	palette[npal].code[1]='.'|128;
	npal++;
	if(s->w && s->h) return;
invalid:
	fprintf(stderr,"Invalid stipple\n");
	exit(1);
}

static void read_picture_1(void) {
	static const char base64[64]="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
	int i,j;
	for(i=0;i<size;i++) {
		fread(buf,1,8,stdin);
		if(buf[6]&128) {
			for(j=0;j<npal && (j==noneindex || memcmp(buf,palette[j].data,6));j++);
			if(j==4096) goto invalid;
			if(j==npal) {
				memcpy(palette[j].data,buf,6);
				palette[j].code[0]=base64[j&63];
				palette[j].code[1]=base64[j>>6];
				npal++;
			}
			data[i]=j;
		} else {
			if(noneindex==-1) {
				if(npal==4096) goto invalid;
				palette[npal].code[0]=palette[npal].code[1]=' ';
				noneindex=npal++;
			}
			data[i]=noneindex;
		}
	}
	if(npal>64) codesize=2;
	return;
invalid:
	fprintf(stderr,"Too many colors\n");
	exit(1);
}

static void read_picture_2(void) {
	int i,j;
	for(i=0;i<size;i++) {
		fread(buf,1,8,stdin);
		if(buf[6]&128) {
			for(j=0;j<npal && (j==noneindex || memcmp(buf,palette[j].data,6));j++);
			if(j==npal) goto invalid;
			data[i]=j;
		} else {
			if(noneindex==-1) goto invalid;
			data[i]=noneindex;
		}
	}
	return;
invalid:
	fprintf(stderr,"Incorrect palette\n");
	exit(1);
}

static void output_colors(void) {
	int i;
	for(i=0;i<npal;i++) {
		if(form!=2) putchar('"');
		outch(palette[i].code[0]&127);
		if(codesize==2) outch(palette[i].code[1]&127);
		if(i==noneindex)
		 printf(" c None");
		else if(palette[i].data[0]==palette[i].data[1] && palette[i].data[2]==palette[i].data[3] && palette[i].data[4]==palette[i].data[5])
		 printf(" c #%02x%02x%02x",palette[i].data[0],palette[i].data[2],palette[i].data[4]);
		else
		 printf(" c #%02x%02x%02x%02x%02x%02x",palette[i].data[0],palette[i].data[1],palette[i].data[2],palette[i].data[3],palette[i].data[4],palette[i].data[5]);
		if(opt['t'] || opt['m']) printf(" m %s",i==noneindex?"None":palette[i].code[1]&128?"white":"black");
		if(opt['s'] && symbolic[i] && symbolic[i][0]) printf(" s %s",symbolic[i]);
		puts(form==2?"":"\",");
		if(opt['t'] && i==noneindex) i++;
	}
}

static void output_picture(void) {
	int x,y;
	unsigned short*p=data;
	for(y=0;y<height;y++) {
		if(form!=2) putchar('"');
		for(x=0;x<width;x++) {
			outch(palette[*p].code[0]&127);
			if(codesize==2) outch(palette[*p].code[1]&127);
			p++;
		}
		puts(form==2?"":y==height-1?"\"":"\",");
	}
}

static void output_picture_stippled(void) {
	int x,y;
	unsigned short*p=data;
	for(y=0;y<height;y++) {
		if(form!=2) putchar('"');
		for(x=0;x<width;x++) {
			outch(palette[*p].code[0]&127);
			putchar(":."[stipple[*p>>1]->data[(x%stipple[*p>>1]->w)+stipple[*p>>1]->w*(y%stipple[*p>>1]->h)]]);
			p++;
		}
		puts(form==2?"":y==height-1?"\"":"\",");
	}
}

int main(int argc,char**argv) {
	int i;
	noneindex=-1;
	codesize=1;
	form=3;
	if (argc > 1 && (!strcmp(argv[1],"-h") || !strcmp(argv[1],"--help"))) {
		usage();
	}
	if(argc>1 && argv[1][0]=='=') {
		name=argv[1]+1;
		argc--; argv++;
	}
	if(argc>1) for(i=0;argv[1][i];i++) opt[argv[1][i]&127]=1;
	if(opt['1']) form=1;
	if(opt['2']) form=2;
	fread(buf,1,8,stdin);
	if(memcmp(buf,"farbfeld",8)) {
		fprintf(stderr,"Not farbfeld\n");
		return 1;
	}
	fread(buf,1,8,stdin);
	width=(buf[0]<<24)|(buf[1]<<16)|(buf[2]<<8)|buf[3];
	height=(buf[4]<<24)|(buf[5]<<16)|(buf[6]<<8)|buf[7];
	size=width*height;
	data=malloc(size*sizeof(unsigned short));
	if(!data) {
		fprintf(stderr,"Allocation failed\n");
		return 1;
	}
	if(argc>2) {
		if(!argv[2][0] || (argv[2][1] && (argv[2][2] || opt['t']))) {
			fprintf(stderr,"Invalid code\n");
			return 1;
		}
		if(argv[2][1]) codesize=2;
		for(i=2;i<argc;i+=opt['s']+opt['t']+2) {
			if(!argv[i][0] || !argv[i][codesize-1] || argv[i][codesize]) {
				fprintf(stderr,"Invalid code\n");
				return 1;
			}
			if(opt['s']) symbolic[npal]=argv[i+opt['s']+2];
			define_color(argv[i],argv[i+1]);
			if(opt['t']){
				if(opt['s']) symbolic[npal]=argv[i+opt['s']+2];
				define_stipple(argv[i+2]);
			}
		}
		read_picture_2();
	} else {
		opt['t']=0;
		read_picture_1();
	}
	if(opt['t']) codesize=2;
	switch(form) {
		case 1:
			printf("#define %s_format 1\n",name);
			printf("#define %s_width %d\n",name,width);
			printf("#define %s_height %d\n",name,height);
			printf("#define %s_ncolors %d\n",name,npal);
			printf("#define %s_chars_per_pixel %d\n",name,codesize);
			printf("static char * %s_colors[] = {\n",name);
			for(i=0;i<npal;i++) {
				putchar('"');
				outch(palette[i].code[0]&127);
				if(codesize==2) outch(palette[i].code[1]&127);
				printf("\", \"");
				if(i==noneindex)
				 printf("None\"");
				else if(palette[i].data[0]==palette[i].data[1] && palette[i].data[2]==palette[i].data[3] && palette[i].data[4]==palette[i].data[5])
				 printf("#%02x%02x%02x\"",palette[i].data[0],palette[i].data[2],palette[i].data[4]);
				else
				 printf("#%02x%02x%02x%02x%02x%02x\"",palette[i].data[0],palette[i].data[1],palette[i].data[2],palette[i].data[3],palette[i].data[4],palette[i].data[5]);
				if(i!=npal-1) putchar(',');
				putchar('\n');
			}
			printf("};\nstatic char * %s_pixels[] = {\n",name);
			if(opt['t']) output_picture_stippled(); else output_picture();
			printf("};\n");
			break;
		case 2:
			printf("! XPM2\n%d %d %d %d\n",width,height,npal-(opt['t']&&noneindex!=-1?1:0),codesize);
			output_colors();
			if(opt['t']) output_picture_stippled(); else output_picture();
			break;
		case 3:
			printf("/* XPM */\nstatic char * %s[] = {\n",name);
			printf("\"%d %d %d %d\",\n",width,height,npal-(opt['t']&&noneindex!=-1?1:0),codesize);
			output_colors();
			if(opt['t']) output_picture_stippled(); else output_picture();
			printf("};\n");
			break;
	}
	return 0;
}
