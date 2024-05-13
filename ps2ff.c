#if 0
gcc -s -O2 -o ~/bin/ps2ff ps2ff.c /usr/lib/libgs.so
exit
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define gs_error_Quit (-101)
#include <ghostscript/gdevdsp.h>
#include <ghostscript/iapi.h>

static void*gs;
static int width,height,raster,done;
static unsigned char*image;

static void
usage(void)
{
	fprintf(stderr,
		"Usage: ps2ff <?options...?>                                                   \n"
		"                                                                              \n"
		"Converts PostScript to farbfeld. ps2ff reads PostScript from stdin, converts  \n"
		"that to farbfeld and writes the result to stdout.                             \n"
		"                                                                              \n"
		"The command line arguments are the same as in Ghostscript, although some (such\n"
		"as -q) are already set.                                                       \n"
		"                                                                              \n"
		"If no filename is specified, then it will read a PostScript program from stdin\n"
		"and execute it (use -dBATCH to suppress this).                                \n"
		"                                                                              \n"
		"Only the first page will be rendered. You can use -sPageList= to specify which\n"
		"page you want.                                                                \n"
		"                                                                              \n"
		"Use -g and -r to set the picture size and dots per inch.                      \n"
		"                                                                              \n"
		"Changing the output file with -sOutputFile= or -o has no effect; it will      \n"
		"always write to stdout.                                                       \n"
		"                                                                              \n"
		"If the PostScript program writes to stdout, it will write that data to stderr \n"
		"instead; this way, stdout is used only for the picture itself.                \n"
		"                                                                              \n"
		"Example usage:                                                                \n"
		"   $ ps2ff < postscript > image.ff                                            \n"
		"\n"
	);
	exit(1);
}

static int d_noop(void*a,void*b) {
	return 0;
}

static int d_presize(void*a,void*b,int w,int h,int r,unsigned int f) {
	return f!=0x00400884;
}

static int d_size(void*a,void*b,int w,int h,int r,unsigned int f,unsigned char*p) {
	width=w;
	height=h;
	raster=r;
	image=p;
	return 0;
}

static int d_page(void*a,void*b,int c,int f) {
	if(done) return 0;
	fwrite("farbfeld",1,8,stdout);
	putchar(width>>24); putchar(width>>16); putchar(width>>8); putchar(width);
	putchar(height>>24); putchar(height>>16); putchar(height>>8); putchar(height);
	c=width*height;
	while(c--) {
		putchar(image[0]); putchar(image[0]);
		putchar(image[1]); putchar(image[1]);
		putchar(image[2]); putchar(image[2]);
		putchar(255); putchar(255);
		image+=4;
	}
	done=1;
	return 0;
}

static struct display_callback_v1_s display={
	.size=DISPLAY_CALLBACK_V1_SIZEOF,
	.version_major=1,
	.version_minor=0,
	.display_open=d_noop,
	.display_preclose=d_noop,
	.display_close=d_noop,
	.display_presize=d_presize,
	.display_size=d_size,
	.display_sync=d_noop,
	.display_page=d_page,
};

int main(int argc,char**argv) {
	int i,ex;
	if (argc > 1 && (!strcmp(argv[1],"-h") || !strcmp(argv[1],"--help"))) {
		usage();
	}
	char**nargv;
	nargv=malloc((argc+8)*sizeof(char*));
	if(!nargv) {
		fprintf(stderr,"Allocation failed\n");
		return 2;
	}
	if(i=gsapi_new_instance(&gs,0)) {
		fprintf(stderr,"Ghostscript failed to initialize (code %d)\n",i);
		return 1;
	}
	nargv[0]=argv[0];
	nargv[1]="-sstdout=%stderr";
	nargv[2]="-q";
	nargv[3]="-sDEVICE=display";
	nargv[4]="-sDisplayHandle=0";
	nargv[5]="-dDisplayFormat=16#00400884"; // 0x00402024 doesn't work
	nargv[6]="-dNOPAUSE";
	nargv[7]="-dNOPROMPT";
	memcpy(nargv+8,argv+1,(argc-1)*sizeof(char*));
	if(i=gsapi_set_display_callback(gs,(void*)&display)) {
		fprintf(stderr,"Ghostscript failed to set display callbacks (code %d)\n",i);
		return 4;
	}
	i=gsapi_init_with_args(gs,argc+7,nargv);
	if(!i) i=gsapi_run_string(gs,"systemdict /start get exec\n",0,&ex);
	if(i && i!=gs_error_Quit) {
		fprintf(stderr,"Ghostscript failed (code %d)\n",i);
		return 3;
	}
	gsapi_exit(gs);
	gsapi_delete_instance(gs);
	return 0;
}
