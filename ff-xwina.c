#if 0
gcc -s -O2 -o ~/bin/ff-xwina -Wno-unused-result ff-xwina.c -lX11 -lXrender
exit
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/cursorfont.h>
#include <X11/extensions/Xrender.h>

#define fatal(...) do { fprintf(stderr,__VA_ARGS__); fputc(10,stderr); exit(1); } while(0)

static unsigned char buf[16];
static Display *dpy;
static XImage img;
static Window win;
static XColor xcol;
static XStandardColormap*stdmap;
static XSetWindowAttributes attr;
static int attrmask=CWCursor|CWEventMask|CWColormap;
static int border;
static GC gc;
static XVisualInfo *vout;
static Visual *visual;
static Colormap cmap;
static int screen;
static int depth;
static Window root;
static char*opt[128];
static XSizeHints sizehints;
static XClassHint classhint;
static char txt[128];
static XEvent ev;
static KeySym key;

typedef struct {
	unsigned long flags;
	unsigned long functions;
	unsigned long decorations;
	long inputMode;
	unsigned long status;
} MotifWmHints;

static void
usage(void)
{
	fprintf(stderr,
		"Usage: ff-xwina <?opt?> <?value?> <?opt?> <?value?> ...                       \n"
		"                                                                              \n"
		"This farbfeld program displays a picture on an X server.                      \n"
		"                                                                              \n"
		"This program will display a picture in a window on the X server specified by  \n"
		"the DISPLAY environment variable.                                             \n"
		"                                                                              \n"
		"This is a variant of ff-xwin that allows for transparent images to be shown   \n"
		"using an ARGB Visual. This adds an additional dependency on the Xrender       \n"
		"library.                                                                      \n"
		"                                                                              \n"
		"Specify zero or more options by key/value pairs, with alternating the keys and\n"
		"values as arguments. Only the first letter of the key is significant, and     \n"
		"values must be specified even if it isn't in use (but a blank value is OK,    \n"
		"which is not the same as not specifying the option at all).                   \n"
		"                                                                              \n"
		"Valid options include:                                                        \n"
		"   b -- if set (value is irrelevant), causes any mouse button to output the   \n"
		"        coordinates and button number (with spaces in between) and then exit  \n"
		"   c -- specify the class hint (the default is \"Farbfeld\")                  \n"
		"   d -- sets the _MOTIF_WM_HINTS X property hint indicating that the window   \n"
		"        manager should not decorate the window                                \n"
		"   k -- if set (value is irrelevant), causes any letter or number key to exit;\n"
		"        the return code is the ASCII code of that letter or number (lowercase \n"
		"        letters are converted to uppercase)                                   \n"
		"   n -- specify the window title to use                                       \n"
		"   o -- set the override redirect flag to indicate to window managers that    \n"
		"        the program manages the window                                        \n"
		"   s -- select which screen number to use, rather than the default screen for \n"
		"        this X server                                                         \n"
		"   t -- if set, sets the transient hint to the specified window ID            \n"
		"   v -- override which visual ID to use, instead of using the default         \n"
		"   x -- specify X position of window                                          \n"
		"   y -- specify Y position of window                                          \n"
		"                                                                              \n"
		"A left click (button 1) will write the coordinate number into the window      \n"
		"title, and a right click (button 3) will write one line to stdout with the    \n"
		"clicked coordinates with a space in between. A middle click (button 2) will   \n"
		"exit the program. The space bar can also be pushed to exit the program.       \n"
		"Using the b and k options can alter this.                                     \n"
		"                                                                              \n"
		"Holding shift and doing a left click (button 1) will draw ruler lines; use    \n"
		"numbers 0 to 7 to adjust the colour of lines to draw, and push = to clear the \n"
		"ruler lines.                                                                  \n"
		"                                                                              \n"
		"Example usage:                                                                \n"
		"   $ ff-xwin < image.ff                                                       \n"
		"   $ ff-xwin b 1 < image.ff                                                   \n"
		"\n"
	);
	exit(1);
}

int main(int argc,char**argv) {
	int i,x,y;
	int nitems;
	long masks = VisualScreenMask | VisualDepthMask | VisualClassMask;
	XRenderPictFormat *fmt;
	XVisualInfo tpl = {
		.screen = screen,
		.depth = 32,
		.class = TrueColor
	};

	if (argc>1 && (!strcmp(argv[1],"-h") || !strcmp(argv[1],"--help"))) {
		usage();
	}
	for(i=1;i<argc-1;i+=2) opt[argv[i][0]&127]=argv[i+1];
	fread(buf,1,16,stdin);
	if(memcmp(buf,"farbfeld",8)) fatal("Not farbfeld");
	if(!(dpy=XOpenDisplay(0))) fatal("Unable to connect to X display");
	attr.cursor=XCreateFontCursor(dpy,XC_tcross);
	attr.event_mask=KeyPressMask|ButtonPressMask|ButtonReleaseMask|ExposureMask;
	root=opt['s']?RootWindow(dpy,strtol(opt['s'],0,0)):DefaultRootWindow(dpy);
	screen = DefaultScreen(dpy);

	vout = XGetVisualInfo(dpy, masks, &tpl, &nitems);
	if(!nitems || !vout) fatal("Invalid visual ID");

	for (i = 0; i < nitems; i ++) {
		fmt = XRenderFindVisualFormat(dpy, vout[i].visual);
		if (fmt->type == PictTypeDirect && fmt->direct.alphaMask) {
			visual = vout[i].visual;
			depth = vout[i].depth;
			cmap = XCreateColormap(dpy, root, visual, AllocNone);
			break;
		}
	}

	attr.colormap = cmap;
	attr.background_pixel = 0;
	attr.border_pixel = 0;
	attrmask |= CWBackPixel;
	attrmask |= CWBorderPixel;

	stdmap=XAllocStandardColormap();
	if(!stdmap) fatal("Allocation failed");
	stdmap->red_mult=vout->red_mask&~(vout->red_mask-1);
	stdmap->green_mult=vout->green_mask&~(vout->green_mask-1);
	stdmap->blue_mult=vout->blue_mask&~(vout->blue_mask-1);
	stdmap->red_max=vout->red_mask/stdmap->red_mult;
	stdmap->green_max=vout->green_mask/stdmap->green_mult;
	stdmap->blue_max=vout->blue_mask/stdmap->blue_mult;
	stdmap->base_pixel=0;

	img.width=(buf[8]<<24)|(buf[9]<<16)|(buf[10]<<8)|buf[11];
	img.height=(buf[12]<<24)|(buf[13]<<16)|(buf[14]<<8)|buf[15];
	img.format=ZPixmap;
	img.byte_order=LSBFirst;
	img.bitmap_unit=img.bitmap_pad=8;
	img.bits_per_pixel=((img.depth=vout->depth)+7)&~7;
	img.red_mask=vout->red_mask;
	img.green_mask=vout->green_mask;
	img.blue_mask=vout->blue_mask;
	img.data=malloc(img.width*img.height*(img.bits_per_pixel>>3));
	if(!img.data) fatal("Allocation failed");
	XInitImage(&img);
	for(y=0;y<img.height;y++) for(x=0;x<img.width;x++) {
		fread(buf,1,8,stdin);
		XPutPixel(&img,x,y,(buf[6] << 24) | (buf[0] << 16) | (buf[2] << 8) | buf[4]);
	}
	/* Set the override redirect flag indicating to window managers to leave this window alone */
	if (opt['o']) {
		attr.override_redirect = True;
		attrmask |= CWOverrideRedirect;
	}
	win=XCreateWindow(
		dpy,
		root,
		opt['x']?strtol(opt['x'],0,0):0,
		opt['y']?strtol(opt['y'],0,0):0,
		img.width,
		img.height,
		border,
		depth,
		InputOutput,
		visual,
		attrmask,
		&attr
	);
	sizehints.flags=PSize|PMinSize|PMaxSize|PBaseSize;
	sizehints.width=sizehints.min_width=sizehints.max_width=sizehints.base_width=img.width;
	sizehints.height=sizehints.min_height=sizehints.max_height=sizehints.base_height=img.height;
	if(opt['x'] || opt['y']) sizehints.flags|=USPosition;
	/* Set the MOTIF hint to indicate to window managers not to decorate the window */
	if(opt['d']) {
		Atom atom = XInternAtom(dpy, "_MOTIF_WM_HINTS", False);
		MotifWmHints hints;
		hints.flags = 1 << 1;
		XChangeProperty(dpy, win, atom, atom, 32, PropModeReplace, (unsigned char *)&hints, sizeof(MotifWmHints)/sizeof(long));
	}
	classhint.res_name=classhint.res_class=opt['c']?:"Farbfeld";
	XmbSetWMProperties(dpy,win,opt['n']?:"Farbfeld Viewer",opt['n']?:"Farbfeld Viewer",argv,argc,&sizehints,0,&classhint);
	if(opt['t']) XSetTransientForHint(dpy,win,strtol(opt['t'],0,0));
	gc=XCreateGC(dpy,win,0,0);
	XMapWindow(dpy,win);
	for(;;) {
		XNextEvent(dpy,&ev);
		switch(ev.type) {
			case Expose:
				if(!ev.xexpose.count) XPutImage(dpy,win,gc,&img,0,0,0,0,img.width,img.height);
				break;
			case ButtonPress:
				if(ev.xbutton.button==1) {
					sprintf(txt,"(%d,%d)",ev.xbutton.x,ev.xbutton.y);
					XStoreName(dpy,win,txt);
					if(ev.xbutton.state&ShiftMask) {
						XDrawLine(dpy,win,gc,0,ev.xbutton.y,img.width,ev.xbutton.y);
						XDrawLine(dpy,win,gc,ev.xbutton.x,0,ev.xbutton.x,img.height);
					}
				} else if(ev.xbutton.button==3 && !opt['b']) {
					printf("%d %d\n",ev.xbutton.x,ev.xbutton.y);
				}
				break;
			case ButtonRelease:
				if(opt['b']) {
					printf("%d %d %d\n",ev.xbutton.x,ev.xbutton.y,ev.xbutton.button);
					return 0;
				}
				if(ev.xbutton.button==2) return 0;
				break;
			case KeyPress:
				if(XLookupString(&ev.xkey,txt,2,&key,0)) {
					if(key==32) return 0;
					if(*txt=='=') XPutImage(dpy,win,gc,&img,0,0,0,0,img.width,img.height);
					if(!opt['k']) {
						if(*txt>='0' && *txt<='7') {
							x=y=stdmap->base_pixel;
							if(*txt&1) x+=stdmap->blue_mult*stdmap->blue_max,y+=stdmap->blue_mult*(stdmap->blue_max>>1);
							if(*txt&2) x+=stdmap->green_mult*stdmap->green_max,y+=stdmap->green_mult*(stdmap->green_max>>1);
							if(*txt&4) x+=stdmap->red_mult*stdmap->red_max,y+=stdmap->red_mult*(stdmap->red_max>>1);
							XSetState(dpy,gc,x,y,3,-1);
							XSetLineAttributes(dpy,gc,0,LineDoubleDash,CapButt,JoinMiter);
							XSetDashes(dpy,gc,0,"\x01\x01",2);
						}
						break;
					}
					if(*txt>='0' && *txt<='9') return *txt;
					if(*txt>='A' && *txt<='Z') return *txt;
					if(*txt>='a' && *txt<='z') return *txt+'A'-'a';
				}
				break;
			case MappingNotify:
				XRefreshKeyboardMapping(&ev.xmapping);
				break;
		}
	}
	return 0;
}
