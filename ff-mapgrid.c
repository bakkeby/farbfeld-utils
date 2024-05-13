#if 0
gcc -s -O2 -o ~/bin/ff-mapgrid -Wno-unused-result ff-mapgrid.c
exit
#endif

#define fatal(...) do{ fprintf(stderr,__VA_ARGS__); fputc('\n',stderr); exit(1); }while(0)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
	unsigned char c;
	unsigned char f;
} Cell;

typedef struct {
	unsigned char d[8];
} Pixel;

typedef struct Rule {
	unsigned char opt,act,prob;
	unsigned char match[8];
	struct Rule*next;
} Rule;

typedef struct {
	short x[16];
	short y[16];
	unsigned char w,h;
	unsigned char opt,misc;
	Rule*rules;
} PieceDef;

static unsigned char buf[1024];
static Cell*grid;
static int owidth,oheight,iwidth,iheight,gwidth,gheight;
static short priority_mode,priority_shift,wrap_mode;
static int xytransform[8];
static Pixel*ipic;
static Pixel*opic;
static PieceDef pieces[128];

static void
usage(void)
{
	fprintf(stderr,
		"Usage: ff-mapgrid <def_file.txt> <map_file.txt>                               \n"
		"                                                                              \n"
		"This farbfeld program generates a map grid.                                   \n"
		"                                                                              \n"
		"This program takes as input a picture with the map tiles.                     \n"
		"                                                                              \n"
		"The general idea is that the program takes a tile set and uses input rules to \n"
		"place those tiles before writing the output picture to standard out.          \n"
		"                                                                              \n"
		"It should be noted that this program is documented being untested.            \n"
		"                                                                              \n"
		"The program takes two arguments:                                              \n"
		"   - the definition file name                                                 \n"
		"   - the map file name                                                        \n"
		"                                                                              \n"
		"The general format of these two files is an option letter followed by a number\n"
		"value, optionally followed by a space and another number value. Then one or   \n"
		"more lines of options.                                                        \n"
		"                                                                              \n"
		"Refer to the man page for details on the definition and map file format       \n"
		"options.                                                                      \n"
		"                                                                              \n"
		"Example usage:                                                                \n"
		"   $ ff-mapgrid def_file.txt map_file.txt < tileset.ff > image-mapgrid.ff     \n"
		"\n"
	);
	exit(1);
}

static int input_line(FILE*fp) {
	unsigned char*s;
	again:
	*buf=0;
	if(!fgets(buf,1024,fp)) return 0;
	s=buf+strlen(buf);
	while(s!=buf && s[-1]<=32) *s--=0;
	if(*buf=='#' || !*buf) goto again;
	return 1;
}

static inline int xtran(int x,int y) {
	return x*xytransform[0]+y*xytransform[1]+xytransform[2];
}

static inline int ytran(int x,int y) {
	return x*xytransform[4]+y*xytransform[5]+xytransform[6];
}

static inline int max4(int a,int b,int c,int d) {
	if(a>=b && a>=c && a>=d) return a;
	if(b>=a && b>=c && b>=d) return b;
	if(c>=a && c>=b && c>=d) return c;
	if(d>=a && d>=b && d>=c) return d;
}

static void parse_match(Rule*r) {
	char*s=buf+1;
	int m=0;
	while(m<8) {
		while(*s==' ' || *s=='\t') s++;
		if(!*s) return;
		if(s[1] && s[1]!=' ' && s[1]!='\t') {
			switch(*s) {
				case '^': *s=s[1]^64; break;
				case '-': *s=s[1]|128; break;
				case '?': *s=255; break;
				default: fatal("Unrecognized matching type: %c%c",s[0],s[1]);
			}
		}
		r->match[m++]=*s;
		s++;
		if(!*++s) return;
	}
}

static void load_definition_file(const char*filename) {
	FILE*fp=fopen(filename,"r");
	char*s;
	Rule*r=0;
	int g=0;
	int i;
	if(!fp) fatal("Cannot open definition file: %m");
	while(input_line(fp)) switch(*buf) {
		case 'D':
			pieces->w=strtol(buf+1,&s,10);
			if(*s) pieces->h=strtol(s,0,10); else pieces->h=pieces->w;
			for(i=0;i<128;i++) pieces[i].w=pieces->w,pieces[i].h=pieces->h;
			break;
		case 'P':
			priority_mode=strtol(buf+1,&s,10);
			priority_shift=strtol(s,&s,10);
			break;
		case 'T':
			xytransform[0]=strtol(buf+1,&s,10);
			xytransform[1]=strtol(s,&s,10);
			xytransform[2]=strtol(s,&s,10);
			xytransform[3]=strtol(s,&s,10);
			xytransform[4]=strtol(s,&s,10);
			xytransform[5]=strtol(s,&s,10);
			xytransform[6]=strtol(s,&s,10);
			xytransform[7]=strtol(s,&s,10);
			break;
		case 'W':
			if(buf[1]=='h' || buf[2]=='h') wrap_mode|=1;
			if(buf[1]=='v' || buf[2]=='v') wrap_mode|=2;
			break;
		case 'a':
			if(!r) fatal("Rule not defined");
			if(buf[1]=='^' && buf[2]) buf[1]=buf[2]^64;
			r->act=buf[1];
			break;
		case 'd':
			pieces[g].w=strtol(buf+1,&s,10);
			if(*s) pieces[g].h=strtol(s,&s,10); else pieces[g].h=pieces[g].w;
			break;
		case 'm':
			if(!r) fatal("Rule not defined");
			parse_match(r);
			break;
		case 'n':
			if(!r) fatal("Rule not defined");
			r->act=strtol(buf+1,0,10)+128;
			break;
		case 'o':
			pieces[g].opt=buf[1];
			pieces[g].misc=strtol(buf+2,0,10);
			break;
		case 'p':
			if(!r) fatal("Rule not defined");
			r->prob=strtol(buf+1,0,10);
			break;
		case 'r':
			r=malloc(sizeof(Rule));
			if(!r) fatal("Allocation failed");
			r->opt=buf[1];
			r->act=255;
			r->prob=255;
			r->match[0]='*'+128;
			r->next=pieces[g].rules;
			pieces[g].rules=r;
			break;
		case 'x':
			s=buf+1;
			for(i=0;i<16;i++) pieces[g].x[i]=strtol(s,&s,10);
			break;
		case 'y':
			s=buf+1;
			for(i=0;i<16;i++) pieces[g].y[i]=strtol(s,&s,10);
			break;
		case '@':
			g=buf[1]&127;
			if(g=='^' && buf[2]) g=buf[2]^64;
			r=0;
			break;
		default:
			fatal("Invalid command in definition file");
	}
	fclose(fp);
}

static void load_map_file(const char*filename) {
	FILE*fp=fopen(filename,"r");
	char*s;
	int i,g;
	if(!fp) fatal("Cannot open map file: %m");
	while(input_line(fp)) switch(*buf) {
		case 'g':
			gwidth=strtol(buf+1,&s,10);
			gheight=strtol(s,&s,10);
			if(gwidth<=0 || gheight<=0) fatal("Invalid grid size (%d x %d)", gwidth, gheight);
			owidth=max4(xtran(gwidth,gheight),xtran(gwidth,0),xtran(0,gheight),xtran(0,0))+xytransform[3];
			oheight=max4(ytran(gwidth,gheight),ytran(gwidth,0),ytran(0,gheight),ytran(0,0))+xytransform[7];
			if(owidth<=0 || oheight<=0) fatal("Invalid output grid size (%d x %d)", owidth, oheight);
			opic=calloc(owidth,oheight*8);
			grid=calloc(gwidth,gheight*sizeof(Cell));
			if(!opic || !grid) fatal("Allocation failed");
			g=0;
			break;
		case 's':
			srandom(strtol(buf+1,0,10));
			break;
		case '-':
			if(g>=gwidth*gheight) fatal("Too many rows of data in map file");
			s=buf+1;
			for(i=0;i<gwidth;i++) {
				if(*s) grid[g+i].c=*s==' '?0:*s,s++;
			}
			if(*s) fatal("Row too long");
			g+=gwidth;
			break;
		default:
			fatal("Invalid command in map file");
	}
	fclose(fp);
}

static void draw_picture(int ox,int oy,int ix,int iy,int w,int h,int al) {
	int x,y,a;
	Pixel*pi;
	Pixel*po;
	if(priority_mode==1) al-=oy>>priority_shift;
	if(priority_mode==2) al-=(oheight-oy-h)>>priority_shift;
	if(priority_mode==3) al-=ox>>priority_shift;
	if(priority_mode==4) al-=(owidth-ox-w)>>priority_shift;
	if(ox<0) w+=ox,ix-=ox,ox=0;
	if(oy<0) h+=oy,ix-=oy,oy=0;
	if(ox+w>=owidth) w=owidth-ox;
	if(oy+h>=oheight) h=oheight-oy;
	for(y=0;y<h;y++) {
		pi=ipic+iwidth*iy+ix;
		po=opic+owidth*oy+ox;
		for(x=0;x<w;x++) {
			if(priority_mode==9) al=random()&255;
			a=al+(pi->d[6]-po->d[6])*256+pi->d[7]-po->d[7];
			if(a>0) {
				*po=*pi;
				if(al) {
					a=al+pi->d[6]*256+pi->d[7];
					if(a<0) a=0;
					if(a>0xFFFF) a=0xFFFF;
					po->d[6]=a>>8;
					po->d[7]=a;
				}
			}
			pi++; po++;
		}
	}
}

static int tile_at(int x,int y) {
	if(x<0 || x>=gwidth) {
		if(wrap_mode&1) {
			while(x<0) x+=gwidth;
			while(x>=gwidth) x-=gwidth;
		} else {
			return 0;
		}
	}
	if(y<0 || y>=gheight) {
		if(wrap_mode&2) {
			while(y<0) y+=gheight;
			while(y>=gheight) x-=gheight;
		} else {
			return 0;
		}
	}
	return grid[y*gwidth+x].c&127;
}

static int do_connections(int i,int x,int y,int c) {
	int e=tile_at(x+1,y);
	int n=tile_at(x,y-1);
	int w=tile_at(x-1,y);
	int s=tile_at(x,y+1);
	int f=0;
	if(i==e || ((pieces[e].opt=='c' || pieces[e].opt=='C') && (c&pieces[e].misc))) f|=1;
	if(i==n || ((pieces[n].opt=='c' || pieces[n].opt=='C') && (c&pieces[n].misc))) f|=2;
	if(i==w || ((pieces[w].opt=='c' || pieces[w].opt=='C') && (c&pieces[w].misc))) f|=4;
	if(i==s || ((pieces[s].opt=='c' || pieces[s].opt=='C') && (c&pieces[s].misc))) f|=8;
	return f;
}

#define MatchAt(a,xx,yy) if(r->match[a]!=255 && r->match[a]!=tile_at(x+(xx),y+(yy))) return 0;
static int do_rule(int o,Rule*r,int x,int y) {
	if(r->prob!=255 && (random()&255)>r->prob) return 0;
	switch(r->opt) {
		case 'n':
			if(grid[o].f) return 0;
			break;
	}
	switch(r->match[0]) {
		case 'h'+128:
			MatchAt(1,-1,-1);
			MatchAt(2,-1,1);
			MatchAt(3,-2,0);
			MatchAt(4,2,0);
			MatchAt(5,-1,1);
			MatchAt(6,1,1);
			break;
		case 'v'+128:
			MatchAt(1,0,-2);
			MatchAt(2,-1,-1);
			MatchAt(3,1,-1);
			MatchAt(4,-1,1);
			MatchAt(5,1,1);
			MatchAt(6,0,2);
			break;
		case 'x'+128:
			MatchAt(1,0,-1);
			MatchAt(2,-1,0);
			MatchAt(3,1,0);
			MatchAt(4,0,1);
			break;
		default:
			MatchAt(0,-1,-1);
			MatchAt(1,0,-1);
			MatchAt(2,1,-1);
			MatchAt(3,-1,0);
			MatchAt(4,1,0);
			MatchAt(5,-1,1);
			MatchAt(6,0,1);
			MatchAt(7,1,1);
	}
	if(r->act&128) grid[o].f=r->act&127; else grid[o].c=r->act;
	return (r->opt!='m');
}

static void process_rules(int phase) {
	int i,n,x,y;
	Rule*r;
	for(i=x=y=0;y<gheight;i++) {
		n=grid[i].c;
		switch(pieces[n].opt) {
			case '0':
				if(phase) goto next;
				break;
			case '1':
				if(!phase) goto next;
				break;
			case 'c':
				grid[i].f=do_connections(grid[i].c,x,y,pieces[n].misc);
				break;
			case 'r':
				grid[i].f=random()%pieces[n].misc;
				break;
		}
		r=pieces[n].rules;
		while(r) {
			if(do_rule(i,r,x,y)) break;
			r=r->next;
		}
		next: if(++x==gwidth) x=0,++y;
	}
}

static void process(void) {
	int a,c,f,i,x,y,z;
	process_rules(0);
	process_rules(1);
	a=0;
	for(i=x=y=0;y<gheight;i++) {
		c=grid[i].c;
		f=grid[i].f;
		if(priority_mode==5) a=y<<priority_shift;
		if(priority_mode==6) a=(gheight-y-1)<<priority_shift;
		if(priority_mode==7) a=x<<priority_shift;
		if(priority_mode==8) a=(gwidth-x-1)<<priority_shift;
		switch(pieces[c].opt) {
			case 'a':
				draw_picture(xtran(x,y),ytran(x,y),pieces[c].x[0],pieces[c].y[0],pieces[c].w,pieces[c].h,a+f);
				break;
			default:
				f&=15;
				draw_picture(xtran(x,y),ytran(x,y),pieces[c].x[f],pieces[c].y[f],pieces[c].w,pieces[c].h,a);
		}
		if(++x==gwidth) x=0,++y;
	}
}

int main(int argc,char**argv) {
	if (argc!=3 || !strcmp(argv[1],"-h") || !strcmp(argv[1],"--help")) {
		usage();
	}
	load_definition_file(argv[1]);
	load_map_file(argv[2]);
	fread(buf,1,16,stdin);
	iwidth=(buf[8]<<24)|(buf[9]<<16)|(buf[10]<<8)|buf[11];
	iheight=(buf[12]<<24)|(buf[13]<<16)|(buf[14]<<8)|buf[15];
	ipic=malloc(iwidth*iheight*8);
	if(!ipic) fatal("Allocation failed");
	fread(ipic,iwidth,iheight<<3,stdin);
	process();
	buf[8]=owidth>>24; buf[9]=owidth>>16; buf[10]=owidth>>8; buf[11]=owidth;
	buf[12]=oheight>>24; buf[13]=oheight>>16; buf[14]=oheight>>8; buf[15]=oheight;
	fwrite(buf,1,16,stdout);
	fwrite(opic,owidth,oheight<<3,stdout);
	return 0;
}
