#if 0
gcc -s -O2 -o ~/bin/ff-scanf -Wno-unused-result ff-scanf.c
exit
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned char buf[8];
static int count,width,height;

static void
usage(void)
{
	fprintf(stderr,
		"Usage: ff-scanf <width> <height> <?format string?> <?+?>                      \n"
		"                                                                              \n"
		"This farbfeld program converts text representation pixels into a picture.     \n"
		"                                                                              \n"
		"The program reads text of pixels from standard in, converts that to a farbfeld\n"
		"picture and writes the result to standard out. It is the inverse operation of \n"
		"ff-printf.                                                                    \n"
		"                                                                              \n"
		"The program takes two, three or four arguments.                               \n"
		"                                                                              \n"
		"The first and second arguments are the width and height of the picture.       \n"
		"                                                                              \n"
		"The third argument is the format string; by default \"%%d %%d %%d %%d\\n\".   \n"
		"See scanf(3) for details if you plan on changing the format string. All       \n"
		"argument types in C are int, regardless of bits-per-channel.                  \n"
		"                                                                              \n"
		"You may use shell operator to expand backslash escapes, as it is not done     \n"
		"automatically by this program.                                                \n"
		"                                                                              \n"
		"An empty string can be given for the format string to skip it if the only     \n"
		"thing you want is to set the fourth argument.                                 \n"
		"                                                                              \n"
		"The fourth argument is optional and controls how many bits per channel is     \n"
		"used. By default 16 bits per channel is used. If a + is given as the argument \n"
		"then this indicates that the input uses 8 bits per channel.                   \n"
		"                                                                              \n"
		"You can use ff-info on the original image file to work out what the dimensions\n"
		"were if you are unsure.                                                       \n"
		"                                                                              \n"
		"Example usage:                                                                \n"
		"   $ ff-scanf 640 480 < 16-bits-per-channel.txt > image.ff                    \n"
		"   $ ff-scanf 640 480 "" + < 8-bits-per-channel.txt > image.ff                \n"
		"\n"
	);
	exit(1);
}

int main(int argc,char**argv) {
	int r,g,b,a;
	if (argc>1 && (!strcmp(argv[1],"-h") || !strcmp(argv[1],"--help"))) {
		usage();
	}
	char*form=argc>3 && strlen(argv[3])?argv[3]:"%d %d %d %d\n";
	if(argc<3) {
		fprintf(stderr,"Too few arguments\n");
		return 1;
	}
	width=strtol(argv[1],0,0);
	height=strtol(argv[2],0,0);
	fwrite("farbfeld",1,8,stdout);
	buf[0]=width>>24;
	buf[1]=width>>16;
	buf[2]=width>>8;
	buf[3]=width;
	buf[4]=height>>24;
	buf[5]=height>>16;
	buf[6]=height>>8;
	buf[7]=height;
	fwrite(buf,1,8,stdout);
	count=width*height;
	r=g=b=0;
	a=65535;
	if(argc>4 && argv[4][0]=='+') {
		while(count--) {
			scanf(form,&r,&g,&b,&a);
			buf[0]=buf[1]=r;
			buf[2]=buf[3]=g;
			buf[4]=buf[5]=b;
			buf[6]=buf[7]=a;
			fwrite(buf,1,8,stdout);
		}
	} else {
		while(count--) {
			scanf(form,&r,&g,&b,&a);
			buf[0]=r>>8;
			buf[1]=r;
			buf[2]=g>>8;
			buf[3]=g;
			buf[4]=b>>8;
			buf[5]=b;
			buf[6]=a>>8;
			buf[7]=a;
			fwrite(buf,1,8,stdout);
		}
	}
	return 0;
}
