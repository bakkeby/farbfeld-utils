#if 0
gcc -s -O2 -o ~/bin/ff-dice -Wno-unused-result ff-dice.c
exit
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define fatal(...) do{ fprintf(stderr,__VA_ARGS__); exit(1); }while(0)

static unsigned char head[16];
static int inwidth,inheight;
static unsigned char*pic;
static int slwidth,slheight;
static char*envname;
static char*envend;
static char envval[64];
static char*command;
static int znum,ynum;

#define Setenv(x,y) do{ *envend=x; sprintf(envval,"%d",y); if(setenv(envname,envval,1)) perror("Unable to set environment variable"),exit(1); }while(0)

static void
usage(void)
{
	fprintf(stderr,
		"Usage: ff-dice <slice width> <slice height> <?shell command?> <?env prefix?>  \n"
		"                                                                              \n"
		"This farbfeld program can dice a picture into many slices, and execute a      \n"
		"command for each one.                                                         \n"
		"                                                                              \n"
		"The program takes up to four arguments:                                       \n"
		"   - width of one slice                                                       \n"
		"   - height of one slice                                                      \n"
		"   - shell command to execute for each slice (optional)                       \n"
		"   - prefix for environment variable names (optional)                         \n"
		"                                                                              \n"
		"The height and width will be capped by the maximum size of the input file, so \n"
		"if you want slices 300 pixels wide and the full height then you can just give \n"
		"a very high number for the height.                                            \n"
		"                                                                              \n"
		"The specified shell command is required to consume the entire input. If no    \n"
		"shell command is specified, it acts like \"cat\" was specified (although it   \n"
		"is more efficient).                                                           \n"
		"                                                                              \n"
		"When executing the command, the picture is given on stdin, and anything from  \n"
		"stdout is forwarded to this program's stdout. If the error code is nonzero,   \n"
		"then this program also terminates with the same error code.                   \n"
		"                                                                              \n"
		"Also some environment variables are set:                                      \n"
		"   X = zero-based horizontal number of the slice in its row                   \n"
		"   Y = zero-based vertical number of the slice in its column                  \n"
		"   Z = zero-based slice number                                                \n"
		"   W = width of this slice                                                    \n"
		"   H = height of this slice                                                   \n"
		"                                                                              \n"
		"(W and H are provided for convenience to avoid having to copy the input to    \n"
		"find this information)                                                        \n"
		"                                                                              \n"
		"ff-hjoin and ff-vjoin can be used to glue pictures together again after       \n"
		"processing.                                                                   \n"
		"                                                                              \n"
		"Example usage:                                                                \n"
		"   $ ff-dice 300 40000 'cat > SLICE_$Z.ff' < iamge.ff                         \n"
		"\n"
	);
	exit(1);
}

static void process(int h) {
	int x=0;
	int w=inwidth;
	int y=0;
	unsigned char*p=pic;
	fread(pic,inwidth*h,8,stdin);
	head[12]=h>>24; head[13]=h>>16; head[14]=h>>8; head[15]=h;
	while(w) {
		int i=w;
		unsigned char*q=p;
		FILE*fp=stdout;
		if(i>slwidth) i=slwidth;
		if(command) {
			Setenv('X',x);
			Setenv('Y',ynum);
			Setenv('Z',znum);
			Setenv('W',i);
			Setenv('H',h);
			fp=popen(command,"w");
			if(!fp) {
				perror("Error opening pipe");
				exit(1);
			}
		}
		head[8]=i>>24; head[9]=i>>16; head[10]=i>>8; head[11]=i;
		fwrite(head,1,16,fp);
		for(y=0;y<h;y++) fwrite(q,i,8,fp),q+=inwidth<<3;
		p+=i<<3;
		w-=i;
		if(command && (i=pclose(fp))) {
			if(i==-1) perror("Error closing pipe");
			exit(i==-1?1:i);
		}
		++x; ++znum;
	}
}

int main(int argc,char**argv) {
	if (argc<3 || !strcmp(argv[1],"-h") || !strcmp(argv[1],"--help")) {
		usage();
	}
	slwidth=strtol(argv[1],0,0);
	slheight=strtol(argv[2],0,0);
	if(slwidth<1 || slheight<1) fatal("Improper slice dimensions\n");
	if(argc>4) {
		if(envname=malloc(strlen(argv[4])+2)) sprintf(envname,"%s?",argv[4]);
	} else {
		envname=strdup("?");
	}
	if(!envname) fatal("Allocation failed\n");
	envend=envname+strlen(envname)-1;
	if(argc>3 && argv[3][0]) command=argv[3];
	fread(head,1,16,stdin);
	inwidth=(head[8]<<24)|(head[9]<<16)|(head[10]<<8)|head[11];
	inheight=(head[12]<<24)|(head[13]<<16)|(head[14]<<8)|head[15];
	pic=malloc(inwidth*slheight*8);
	if(!pic) fatal("Allocation failed\n");
	while(inheight) {
		int i=inheight;
		if(i>slheight) i=slheight;
		process(i);
		inheight-=i;
		++ynum;
	}
	return 0;
}
