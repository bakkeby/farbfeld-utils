#if 0
gcc -s -O2 -o ~/bin/ff-xdraw -Wno-unused-result ff-xdraw.c -lX11
exit
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

typedef struct {
	unsigned char d[8];
} Color;

typedef struct {
	int reg[27];
	GC gc;
	Pixmap pic;
} Stack;

typedef struct {
	char depth;
	int screen;
} DepthInfo;

static unsigned char buf[16];
static Display*dis;
static Pixmap pic;
static GC gc;
static int reg[32];
static unsigned char inbuf[1024];
static Color palette[256];
static Stack*stack;
static int stackptr;
static int stackalloc;
static Font fonts[256];
static int args[64];
static int nargs;
static XChar2b txtbuf[1024];
static int txtlen;
static DepthInfo depthinfo[64];
static int nameidx;
static int opcode;
static char opt[128];
static int width;
static int height;
static Region regions[8];
static int mydepth;
static XFontStruct*fontinfo;

#define fatal(...) do { fprintf(stderr,__VA_ARGS__); fputc('\n',stderr); exit(1); } while(0)

static void
usage(void)
{
	fprintf(stderr,
		"Usage: ff-xdraw <?option?> ... <?option?>                                     \n"
		"                                                                              \n"
		"This program can be used to make a drawing using a X server, and then outputs \n"
		"the farbfeld picture that it has drawn.                                       \n"
		"                                                                              \n"
		"Command-line arguments can be one of two formats:                             \n"
		"   - a number 0 to 9 followed by : and a font name, to preset that font number\n"
		"   - a letter A to Z followed by = and a number, to preset that register      \n"
		"                                                                              \n"
		"The instructions are taken from stdin in text format. Refer to the man page   \n"
		"for details on this.                                                          \n"
		"                                                                              \n"
		"Example usage:                                                                \n"
		"   $ ff-xdraw < instructions.txt > image-xdraw.ff                             \n"
		"\n"
	);
	exit(1);
}

static void stack_push(void) {
	int i;
	GC gc2;
	if(stackalloc==stackptr) {
		stack=realloc(stack,(stackalloc=(stackalloc?:1)<<1)*sizeof(Stack));
		if(!stack) fatal("Stack overflow (%d)",stackalloc);
	}
	for(i=0;i<27;i++) stack[stackptr].reg[i]=reg[i];
	stack[stackptr].gc=gc;
	stack[stackptr].pic=0;
	++stackptr;
	gc2=XCreateGC(dis,pic,0,0);
	XCopyGC(dis,gc,-1,gc2);
	gc=gc2;
}

static void stack_pop(void) {
	int i;
	if(!stackptr--) fatal("Stack underflow");
	for(i=0;i<27;i++) reg[i]=stack[stackptr].reg[i];
	XFreeGC(dis,gc);
	gc=stack[stackptr].gc;
	if(stack[stackptr].pic) {
		XFreePixmap(dis,pic);
		pic=stack[stackptr].pic;
	}
	XSync(dis,1);
}

static void op_equal(void) {
	reg[args[63]&31]=nargs?*args:*reg;
}

static void op_plus(void) {
	if(!nargs) ++reg[args[63]&31];
	else if(nargs==1) reg[args[63]&31]+=*args;
	else reg[args[63]&31]=args[0]+args[1];
}

static void op_minus(void) {
	if(!nargs) --reg[args[63]&31];
	else if(nargs==1) reg[args[63]&31]-=*args;
	else reg[args[63]&31]=args[0]-args[1];
}

static void op_asterisk(void) {
	if(!nargs) reg[args[63]&31]<<=1;
	else if(nargs==1) reg[args[63]&31]*=*args;
	else reg[args[63]&31]=args[0]*args[1];
}

static void op_slash(void) {
	if(!nargs) reg[args[63]&31]>>=1;
	else if(nargs==1) reg[args[63]&31]/=*args;
	else reg[args[63]&31]=args[0]/args[1];
}

static void op_percent(void) {
	if(!nargs) reg[args[63]&31]%=*reg;
	else if(nargs==1) reg[args[63]&31]%=*args;
	else reg[args[63]&31]=args[0]%args[1];
}

static void op_tilde(void) {
	fprintf(stderr,"%d : %d %d %d %d : %s\n",nargs,args[0],args[1],args[2],args[3],inbuf);
	if(!nargs) {
		int i;
		for(i=0;i<64;i++) fprintf(stderr,"[%d %d %d]\n",i,depthinfo[i].depth,depthinfo[i].screen);
	}
}

static void op_exclamation(void) {
	GC gc2;
	if(nargs!=2 && nargs!=3) fatal("Incorrect number of arguments");
	if(nargs==2) args[2]=1;
	if(args[2]<1 || args[2]>63 || !depthinfo[args[2]].depth) fatal("Invalid depth");
	if(pic) {
		if(!stackptr || stack[stackptr-1].pic) fatal("Picture already initialized");
		stack[stackptr-1].pic=pic;
	} else {
		width=args[0];
		height=args[1];
		mydepth=args[2];
	}
	pic=XCreatePixmap(dis,RootWindow(dis,depthinfo[args[2]].screen),args[0],args[1],depthinfo[args[2]].depth);
	gc=XCreateGC(dis,pic,0,0);
	gc2=XCreateGC(dis,pic,0,0);
	if(!pic || !gc || !gc2) fatal("Cannot initialize picture/GC");
	XSetState(dis,gc2,0,0,3,-1);
	XFillRectangle(dis,pic,gc2,0,0,args[0],args[1]);
	XFreeGC(dis,gc2);
	XSync(dis,1);
}

static void op_dollar(void) {
	int i=args[0]&255;
	palette[i].d[0]=args[1]>>8;
	palette[i].d[1]=args[1];
	palette[i].d[2]=args[2]>>8;
	palette[i].d[3]=args[2];
	palette[i].d[4]=args[3]>>8;
	palette[i].d[5]=args[3];
	if(nargs>4) {
		palette[i].d[6]=args[4]>>8;
		palette[i].d[7]=args[4];
	} else {
		palette[i].d[6]=palette[i].d[7]=255;
	}
	if(opt['f']) XSetForeground(dis,gc,i);
	if(opt['b']) XSetBackground(dis,gc,i);
}

static void op_dot(void) {
	XPoint pt[32];
	int i;
	for(i=0;i+i<nargs;i++) pt[i].x=args[i+i],pt[i].y=args[i+i+1];
	XDrawPoints(dis,pic,gc,pt,nargs>>1,opt['r']?CoordModePrevious:CoordModeOrigin);
}

static void op_comma(void) {
	if(opt['f']) XSetForeground(dis,gc,*args);
	if(opt['b']) XSetBackground(dis,gc,*args);
	if(opt['m']) XSetPlaneMask(dis,gc,*args);
	if(opt['-']) XSetPlaneMask(dis,gc,-1);
}

static void op_colon(void) {
	int i=*args;
	if(!nargs) return;
	if(opt['-']) i=i<0?0:1;
	if(opt['+']) i=i>0?0:1;
	if(opt['!']) i=i?0:1;
	if(opt[':']) i=i?1:0;
	if(nameidx) {
		if(i) return;
		strncpy(buf,inbuf+nameidx,14);
		while(fgets(inbuf,1024,stdin)) {
			i=0;
			while(inbuf[i] && inbuf[i]<=' ') i++;
			if(inbuf[i++]!=':') continue;
			if(!strncmp(inbuf+i,buf,14)) return;
		}
	} else {
		while(i-->0 && fgets(inbuf,1024,stdin));
	}
}

static void op_semicolon(void) {
	int i=args[2]&7;
	if(!regions[i]) regions[i]=XCreateRegion();
	if(opt['i']) XIntersectRegion(regions[args[0]&7],regions[args[1]&7],regions[i]);
	if(opt['u']) XUnionRegion(regions[args[0]&7],regions[args[1]&7],regions[i]);
	if(opt['s']) XSubtractRegion(regions[args[0]&7],regions[args[1]&7],regions[i]);
	if(opt['x']) XXorRegion(regions[args[0]&7],regions[args[1]&7],regions[i]);
	if(opt['?']) *reg=XEmptyRegion(regions[i])?0:1;
	if(opt['=']) *reg=XEqualRegion(regions[args[0]&7],regions[args[1]&7])?0:1;
}

static void op_question(void) {
	int dir,asc,desc;
	XCharStruct ov;
	if(nameidx) XQueryTextExtents(dis,XGContextFromGC(gc),inbuf+nameidx,strlen(inbuf+nameidx),&dir,&asc,&desc,&ov);
	else XQueryTextExtents16(dis,XGContextFromGC(gc),txtbuf,txtlen,&dir,&asc,&desc,&ov);
	*reg=0;
	if(opt['l']) *reg+=ov.lbearing;
	if(opt['r']) *reg+=ov.rbearing;
	if(opt['L']) *reg-=ov.lbearing;
	if(opt['R']) *reg-=ov.rbearing;
	if(opt['w']) *reg+=ov.width;
	if(opt['a']) *reg+=ov.ascent;
	if(opt['d']) *reg+=ov.descent;
	if(opt['A']) *reg+=asc;
	if(opt['D']) *reg+=desc;
	if(opt['/']) *reg>>=1;
}

static void op_less(void) {
	int r=*args;
	int i;
	for(i=1;i<nargs;i++) if(args[i]<r) r=args[i];
	reg[args[63]&31]=r;
}

static void op_greater(void) {
	int r=*args;
	int i;
	for(i=1;i<nargs;i++) if(args[i]>r) r=args[i];
	reg[args[63]&31]=r;
}

static void op_bar(void) {
	if(nargs==2) {
		reg[args[63]&31]=args[0]?0:args[1];
	} else if(nargs==3) {
		reg[args[63]&31]=args[0]?args[2]:args[1];
	} else if(nargs==4) {
		reg[args[63]&31]=args[0]?(args[0]<0?args[1]:args[3]):args[2];
	}
}

static void op_ampersand(void) {
	XSetFunction(dis,gc,nargs?*args:3);
}

static void op_leftbracket(void) {
	stack_push();
	if(nargs) op_exclamation();
}

static void op_rightbracket(void) {
	if(stackptr) {
		if(opt['t']) XSetTile(dis,stack[stackptr-1].gc,pic);
		if(opt['s']) XSetStipple(dis,stack[stackptr-1].gc,pic);
	}
	stack_pop();
	if(nargs) XSetTSOrigin(dis,gc,args[0],args[1]);
}

static void op_hat(void) {
	int i;
	if(!stackptr) fatal("Stack underflow");
	for(i='@';i<='Z';i++) if(opt[i]) stack[stackptr-1].reg[i&31]=reg[i&31];
}

static void op_A(void) {
	int x=args[0];
	int y=args[1];
	int w=args[2];
	int h=nargs>3?args[3]:w;
	int a1=nargs>4?args[4]:0;
	int a2=nargs>5?args[5]:360*64;
	if(opt['f']) XFillArc(dis,pic,gc,x,y,w,h,a1,a2);
	else XDrawArc(dis,pic,gc,x,y,w,h,a1,a2);
}

static void op_C(void) {
	XCopyArea(dis,pic,pic,gc,args[0],args[1],args[2],args[3],args[4],args[5]);
}

static void op_F(void) {
	if(!nameidx) {
		XSetFont(dis,gc,fonts[*args&255]);
		return;
	}
	if(fonts[*args&255]) XUnloadFont(dis,fonts[*args&255]);
	fonts[*args&255]=XLoadFont(dis,inbuf+nameidx);
	XSync(dis,1);
}

static void op_H(void) {
	if(!fontinfo) fatal("Font info not loaded");
	if(nargs<2) args[1]=*reg;
	*reg=XTextWidth16(fontinfo,txtbuf,txtlen);
	if(!nargs) args[0]=0;
	*reg=args[1]+*reg*((args[0]-1000.0)/1000.0)+0.495;
}

static void op_I(void) {
	XCharStruct*cs;
	int i,j;
	if(!fontinfo) fatal("Font info not loaded");
	*reg=nargs>1?args[1]:0;
	if(opt['c']) *reg=fontinfo->default_char;
	if(opt['i']) *reg=fontinfo->direction;
	if(opt['a']) *reg+=fontinfo->ascent;
	if(opt['d']) *reg+=fontinfo->descent;
	if(opt['m']) {
		cs=&fontinfo->min_bounds;
	} else if(opt['M']) {
		cs=&fontinfo->max_bounds;
	} else if(nargs || opt['C']) {
		if(opt['C']) *args=fontinfo->default_char; else if(*args<0) return;
		if(!fontinfo->min_byte1 && !fontinfo->max_byte1) {
			if(*args<fontinfo->min_char_or_byte2 || *args>fontinfo->max_char_or_byte2) return;
			i=*args-fontinfo->min_char_or_byte2;
		} else {
			i=*args>>8;
			j=*args&255;
			if(i<fontinfo->min_byte1 || i>fontinfo->max_byte1) return;
			if(j<fontinfo->min_char_or_byte2 || j>fontinfo->max_char_or_byte2) return;
			i=(i-fontinfo->min_byte1)*(fontinfo->max_char_or_byte2+1-fontinfo->min_char_or_byte2)+j;
		}
		cs=fontinfo->per_char?fontinfo->per_char+i:&fontinfo->max_bounds;
	} else {
		return;
	}
	if(opt['F']) *reg=cs->attributes;
	if(opt['A']) *reg+=cs->ascent;
	if(opt['D']) *reg+=cs->descent;
	if(opt['W']) *reg+=cs->width;
	if(opt['L']) *reg+=cs->lbearing;
	if(opt['R']) *reg+=cs->rbearing;
}

static void op_L(void) {
	XPoint pt[32];
	int i;
	for(i=0;i+i<nargs;i++) pt[i].x=args[i+i],pt[i].y=args[i+i+1];
	if(opt['f']) XFillPolygon(dis,pic,gc,pt,nargs>>1,opt['c']?Convex:opt['n']?Nonconvex:Complex,opt['r']?CoordModePrevious:CoordModeOrigin);
	else XDrawLines(dis,pic,gc,pt,nargs>>1,opt['r']?CoordModePrevious:CoordModeOrigin);
}

static void op_M(void) {
	XSetClipMask(dis,gc,nargs?*args:0);
}

static void op_O(void) {
	int i=args[63]&7;
	XOffsetRegion(regions[i],args[0],args[1]);
	if(nargs>2) XShrinkRegion(regions[i],args[2],args[nargs>3?3:2]);
}

static void op_P(void) {
	XCopyPlane(dis,pic,pic,gc,args[0],args[1],args[2],args[3],args[4],args[5],1<<args[6]);
}

static void op_Q(void) {
	if(fontinfo) XFreeFontInfo(0,fontinfo,1);
	if(opt['Z']) {
		fontinfo=0;
		return;
	}
	fontinfo=XQueryFont(dis,nargs?fonts[*args&255]:XGContextFromGC(gc));
	if(!fontinfo) fatal("Cannot load font info");
}

static void op_R(void) {
	if(opt['f']) XFillRectangle(dis,pic,gc,args[0],args[1],args[2],args[3]);
	else XDrawRectangle(dis,pic,gc,args[0],args[1],args[2],args[3]);
}

static void op_T(void) {
	if(opt['i']) {
		if(nameidx) XDrawImageString(dis,pic,gc,args[0],args[1],inbuf+nameidx,strlen(inbuf+nameidx));
		else XDrawImageString16(dis,pic,gc,args[0],args[1],txtbuf,txtlen);
	} else {
		if(nameidx) XDrawString(dis,pic,gc,args[0],args[1],inbuf+nameidx,strlen(inbuf+nameidx));
		else XDrawString16(dis,pic,gc,args[0],args[1],txtbuf,txtlen);
	}
	if(opt['q']) {
		int dir,asc,desc;
		XCharStruct ov;
		if(nameidx) XQueryTextExtents(dis,XGContextFromGC(gc),inbuf+nameidx,strlen(inbuf+nameidx),&dir,&asc,&desc,&ov);
		else XQueryTextExtents16(dis,XGContextFromGC(gc),txtbuf,txtlen,&dir,&asc,&desc,&ov);
		*reg=*args+ov.width;
	}
}

static void op_X(void) {
	XCopyArea(dis,args[6],pic,gc,args[0],args[1],args[2],args[3],args[4],args[5]);
}

static void op_Z(void) {
	Window r;
	int x,y,w,h,b,d;
	GC gc2;
	XGetGeometry(dis,pic,&r,&x,&y,&w,&h,&b,&d);
	gc2=XCreateGC(dis,pic,0,0);
	XSetState(dis,gc2,*args,*args,3,-1);
	XFillRectangle(dis,pic,gc2,0,0,w,h);
	XFreeGC(dis,gc2);
	XSync(dis,1);
}

static void op_a(void) {
	if(opt['('] || opt[')']) XSetArcMode(dis,gc,ArcChord);
	if(opt['<'] || opt['>']) XSetArcMode(dis,gc,ArcPieSlice);
}

static void op_b(void) {
	if(nargs>1) *reg=args[1];
	if(opt['&']) *reg&=*args;
	if(opt['|']) *reg|=*args;
	if(opt['^']) *reg^=*args;
	if(opt['~']) *reg^=-1;
	if(opt['-']) *reg=-*args;
}

static void op_c(void) {
	XChar2b ch;
	ch.byte1=args[2]>>8;
	ch.byte2=args[2]&255;
	if(opt['i']) XDrawImageString16(dis,pic,gc,args[0],args[1],&ch,1);
	else XDrawString16(dis,pic,gc,args[0],args[1],&ch,1);
}

static void op_f(void) {
	if(opt['=']) XSetFillStyle(dis,gc,FillSolid);
	if(opt['t']) XSetFillStyle(dis,gc,FillTiled);
	if(opt['s']) XSetFillStyle(dis,gc,FillStippled);
	if(opt['S']) XSetFillStyle(dis,gc,FillOpaqueStippled);
	if(opt['e']) XSetFillRule(dis,gc,EvenOddRule);
	if(opt['w']) XSetFillRule(dis,gc,WindingRule);
	if(nargs) XSetTSOrigin(dis,gc,args[0],args[1]);
}

static void op_g(void) {
	int i=args[63]&7;
	XPoint pt[32];
	if(regions[i]) XDestroyRegion(regions[i]);
	regions[i]=0;
	if(nargs) {
		if(nargs>60) fatal("Too many points");
		for(i=0;i+i<nargs;i++) pt[i].x=args[i+i],pt[i].y=args[i+i+1];
		regions[args[63]&7]=XPolygonRegion(pt,nargs>>1,EvenOddRule);
	}
}

static void op_h(void) {
	int i;
	for(i=0;i<txtlen;i++) txtbuf[i].byte1=*args;
}

static void op_i(void) {
	if(!stackptr || !stack[stackptr-1].pic) fatal("No picture available to copy");
	if(opt['p']) XCopyPlane(dis,stack[stackptr-1].pic,pic,gc,args[0],args[1],args[2],args[3],args[4],args[5],args[6]);
	else XCopyArea(dis,stack[stackptr-1].pic,pic,gc,args[0],args[1],args[2],args[3],args[4],args[5]);
}

static void op_j(void) {
	int i;
	for(i=0;i<txtlen;i++) {
		txtbuf[i].byte2+=*args;
		if(opt['c'] && txtbuf[i].byte2<*args) ++txtbuf[i].byte1;
	}
}

static void op_l(void) {
	char dash[64];
	int i;
	if(*args>=0) XSetLineAttributes(dis,gc,*args
		,opt['o']?LineOnOffDash:opt['d']?LineDoubleDash:LineSolid
		,opt['n']?CapNotLast:opt['r']?CapRound:opt['p']?CapProjecting:CapButt
		,opt['j']?JoinRound:opt['v']?JoinBevel:JoinMiter
	);
	if(nargs>2) {
		for(i=2;i<nargs;i++) dash[i-2]=args[i];
		XSetDashes(dis,gc,args[1],dash,nargs-2);
	}
}

static void op_m(void) {
	if(!stackptr || !stack[stackptr-1].pic) fatal("No picture available to use for mask");
	XSetClipMask(dis,stack[stackptr-1].gc,pic);
	XSetClipOrigin(dis,stack[stackptr-1].gc,args[0],args[1]);
}

static void op_o(void) {
	if(!stackptr || !stack[stackptr-1].pic) fatal("No picture available to copy");
	if(opt['p']) XCopyPlane(dis,pic,stack[stackptr-1].pic,stack[stackptr-1].gc,args[0],args[1],args[2],args[3],args[4],args[5],args[6]);
	else XCopyArea(dis,pic,stack[stackptr-1].pic,stack[stackptr-1].gc,args[0],args[1],args[2],args[3],args[4],args[5]);
}

static void op_p(void) {
	unsigned long lo;
	if(!fontinfo) fatal("Font info not loaded");
	if(!nameidx) fatal("Property name required");
	*reg=XGetFontProperty(fontinfo,XInternAtom(dis,inbuf+nameidx,1),&lo)?lo:*args;
}

static void op_r(void) {
	XRectangle rect;
	rect.x=args[0];
	rect.y=args[1];
	rect.width=args[2];
	rect.height=args[3];
	XSetClipRectangles(dis,gc,0,0,&rect,1,YXBanded);
}

static void op_u(void) {
	int i=args[63]&7;
	XRectangle r;
	if(!regions[i]) regions[i]=XCreateRegion();
	r.x=args[0];
	r.y=args[1];
	r.width=args[2];
	r.height=args[3];
	XUnionRectWithRegion(&r,regions[i],regions[i]);
}

static void op_v(void) {
	int i;
	if(!stackptr) fatal("Stack underflow");
	for(i='@';i<='Z';i++) if(opt[i]) reg[i&31]=stack[stackptr-1].reg[i&31];
}

static void op_w(void) {
	if(!fontinfo) fatal("Font info not loaded");
	if(nameidx) *reg=XTextWidth(fontinfo,inbuf+nameidx,strlen(inbuf+nameidx));
	else *reg=XTextWidth16(fontinfo,txtbuf,txtlen);
}

static void op_x(void) {
	char*x=XFetchBuffer(dis,&txtlen,nargs?*args:0);
	int i;
	if(txtlen>1024) txtlen=1024;
	for(i=0;i<txtlen;i++) txtbuf[i].byte1=0,txtbuf[i].byte2=x[i]&255;
	if(x) XFree(x);
	if(opt['n']) *reg=txtlen;
}

static void op_y(void) {
	int i=args[63]&7;
	if(!regions[i]) regions[i]=XCreateRegion();
	XSetRegion(dis,gc,regions[i]);
}

typedef void(*Opcode)(void);

static const Opcode optab[128]={
	['=']=op_equal,
	['+']=op_plus,
	['-']=op_minus,
	['*']=op_asterisk,
	['/']=op_slash,
	['%']=op_percent,
	['~']=op_tilde,
	['!']=op_exclamation,
	['$']=op_dollar,
	['.']=op_dot,
	[',']=op_comma,
	[':']=op_colon,
	[';']=op_semicolon,
	['?']=op_question,
	['<']=op_less,
	['>']=op_greater,
	['|']=op_bar,
	['&']=op_ampersand,
	['[']=op_leftbracket,
	[']']=op_rightbracket,
	['^']=op_hat,
	['A']=op_A,
	['C']=op_C,
	['F']=op_F,
	['H']=op_H,
	['I']=op_I,
	['L']=op_L,
	['M']=op_M,
	['O']=op_O,
	['P']=op_P,
	['Q']=op_Q,
	['R']=op_R,
	['T']=op_T,
	['X']=op_X,
	['Z']=op_Z,
	['a']=op_a,
	['b']=op_b,
	['c']=op_c,
	['f']=op_f,
	['g']=op_g,
	['h']=op_h,
	['i']=op_i,
	['j']=op_j,
	['l']=op_l,
	['m']=op_m,
	['o']=op_o,
	['p']=op_p,
	['r']=op_r,
	['u']=op_u,
	['v']=op_v,
	['w']=op_w,
	['x']=op_x,
	['y']=op_y,
};

static void get_depth_info(void) {
	int i,n;
	XVisualInfo*vis=XGetVisualInfo(dis,0,0,&n);
	if(!vis) fatal("No visual info available");
	depthinfo[1].depth=1;
	depthinfo[1].screen=DefaultScreen(dis);
	for(i=0;i<n;i++) if(vis[i].depth>0 && vis[i].depth<64) {
		if(vis[i].screen==DefaultScreen(dis) || !depthinfo[vis[i].depth].depth) {
			depthinfo[vis[i].depth].depth=vis[i].depth;
			depthinfo[vis[i].depth].screen=vis[i].screen;
		}
	}
	XFree(vis);
	for(i=62;i;i--) {
		if(!depthinfo[i].depth) depthinfo[i]=depthinfo[i+1];
	}
}

static void process_line(void) {
	static const char hexcode[128]={
		['0']=0x0, ['1']=0x1, ['2']=0x2, ['3']=0x3, ['4']=0x4, ['5']=0x5, ['6']=0x6, ['7']=0x7,
		['8']=0x8, ['9']=0x9, ['A']=0xA, ['B']=0xB, ['C']=0xC, ['D']=0xD, ['E']=0xE, ['F']=0xF,
	};
	int st=0;
	int end=strlen(inbuf);
	int i;
	while(inbuf[st] && inbuf[st]<=' ') st++;
	if(inbuf[st]=='#' || !inbuf[st]) return;
	while(end && inbuf[end-1]<=' ') inbuf[--end]=0;
	opcode=inbuf[st++]&127;
	if(opcode=='"') {
		txtlen=0;
		while((i=inbuf[st]) && i!='"') {
			if(i=='\\') {
				if(inbuf[st+1]=='x') {
					if(st>=1020) fatal("Input buffer overflow");
					txtbuf[txtlen].byte1=0;
					txtbuf[txtlen].byte2=(hexcode[inbuf[st+1]&127]<<4)|hexcode[inbuf[st+2]&128];
					st+=3;
				} else if(inbuf[st+1]=='X') {
					if(st>=1016) fatal("Input buffer overflow");
					txtbuf[txtlen].byte1=(hexcode[inbuf[st+1]&127]<<4)|hexcode[inbuf[st+2]&128];
					txtbuf[txtlen].byte2=(hexcode[inbuf[st+3]&127]<<4)|hexcode[inbuf[st+4]&128];
					st+=5;
				} else {
					txtbuf[txtlen].byte1=0;
					if(txtbuf[txtlen].byte2=inbuf[++st]) st++;
				}
			} else {
				txtbuf[txtlen].byte1=0;
				txtbuf[txtlen].byte2=i;
				st++;
			}
			txtlen++;
		}
		return;
	}
	memset(opt,0,128);
	nargs=0;
	nameidx=0;
	args[63]=inbuf[st];
	while(i=inbuf[st]) {
		opt[i&127]=1;
		st++;
		if(i==' ' || i=='\t') break;
	}
	while(inbuf[st] && nargs<64) {
		while(inbuf[st] && inbuf[st]<=' ') st++;
		switch(i=inbuf[st]) {
			case '-':
				i=1;
				st++;
				goto isnum;
			case '+':
				i=0;
				st++;
				goto isnum;
			case '0' ... '9':
				i=0;
			isnum:
				args[nargs]=0;
				while(inbuf[st]>='0' && inbuf[st]<='9') args[nargs]=args[nargs]*10+inbuf[st++]-'0';
				if(i) args[nargs]=-args[nargs];
				if(inbuf[st]=='.') st++,args[nargs]*=64;
				nargs++;
				break;
			case ':':
				st++;
				while(inbuf[st] && inbuf[st]<=' ') st++;
				nameidx=st;
				goto done;
			case '@' ... 'Z':
				args[nargs++]=reg[i&31];
				st++;
				break;
			case '#':
				goto done;
			case '$':
				st++;
				i=0;
				while(inbuf[st]=='0' || hexcode[inbuf[st]&127]) i=(i<<4)|hexcode[inbuf[st++]&127];
				args[nargs++]=i;
				break;
			default:
				if(i) fatal("Invalid character %c",i);
		}
	}
	done:
	if(!optab[opcode]) fatal("Invalid opcode %c",opcode);
	optab[opcode]();
}

static void ship_out(void) {
	XImage*im=XGetImage(dis,pic,0,0,width,height,(1<<mydepth)-1,ZPixmap);
	int i,x,y;
	long z;
	if(!im) fatal("Problem with picture");
	fwrite("farbfeld",1,8,stdout);
	buf[0]=width>>24; buf[1]=width>>16; buf[2]=width>>8; buf[3]=width;
	buf[4]=height>>24; buf[5]=height>>16; buf[6]=height>>8; buf[7]=height;
	fwrite(buf,1,8,stdout);
	for(y=0;y<height;y++) for(x=0;x<width;x++) {
		z=XGetPixel(im,x,y);
		if(mydepth<=8) {
			fwrite(palette[z].d,1,8,stdout);
		} else if(mydepth<=16) {
			z=((z>>8)+z)&255;
			fwrite(palette[z].d,1,8,stdout);
		} else {
			putchar(z>>16); putchar(z>>16);
			putchar(z>>8); putchar(z>>8);
			putchar(z); putchar(z);
			putchar(255); putchar(255);
		}
	}
}

int main(int argc,char**argv) {
	int i;
	if (argc>1 && (!strcmp(argv[1],"-h") || !strcmp(argv[1],"--help"))) {
		usage();
	}
	dis=XOpenDisplay(0);
	if(!dis) fatal("Cannot connect to X server");
	for(i=1;i<argc;i++) {
		if(argv[i][0]=='+') {
			stdin=fopen(argv[++i],"r");
		} else if(argv[i][0] && argv[i][1]=='=') {
			reg[argv[i][0]&31]=strtol(argv[i]+2,0,0);
		} else if(argv[i][0] && argv[i][1]==':') {
			fonts[(argv[i][0]-'0')&15]=XLoadFont(dis,argv[i]+2);
		}
	}
	get_depth_info();
	while(fgets(inbuf,1024,stdin)) process_line();
	XSync(dis,1);
	if(stackptr) fatal("Incomplete grouping");
	ship_out();
	return 0;
}

