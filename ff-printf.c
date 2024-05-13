#if 0
gcc -s -O2 -o ~/bin/ff-printf -Wno-unused-result ff-printf.c
exit
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned char buf[16];
static int count;

static void
usage(void)
{
	fprintf(stderr,
		"Usage: ff-printf <?format string?> <?+?>                                      \n"
		"                                                                              \n"
		"This farbfeld program is used to convert a picture into a text representation \n"
		"of each pixel.                                                                \n"
		"                                                                              \n"
		"Output the contents of the picture as text.                                   \n"
		"                                                                              \n"
		"This program takes two optional arguments.                                    \n"
		"                                                                              \n"
		"The first argument is the format string; by default \"%%d %%d %%d %%d\\n\".   \n"
		"                                                                              \n"
		"Each pixel is output using this format. See printf(3) for details. Also you   \n"
		"may use shell operator to expand backslash escapes, as it is not done         \n"
		"automatically by this program. Note that you shouldn't change the number of   \n"
		"decimals in the format string unless you know what you are doing.             \n"
		"                                                                              \n"
		"An empty string can be given for the format string to skip it if the only     \n"
		"thing you want is to set the second argument.                                 \n"
		"                                                                              \n"
		"The second argument is optional and controls how many bits per channel is     \n"
		"used. By default 16 bits per channel is used. If a + is given as the argument \n"
		"then this indicates that 8 bits per channel should be used.                   \n"
		"                                                                              \n"
		"The width and height information are lost on output; only pixel values are    \n"
		"printed.                                                                      \n"
		"                                                                              \n"
		"A text representation of each pixel can make it easier to perform bespoke     \n"
		"alterations using simple scripts and convert it back to a picture again.      \n"
		"                                                                              \n"
		"Example usage:                                                                \n"
		"   $ ff-printf < image.ff > 16-bits-per-channel.txt                           \n"
		"   $ ff-printf "" + < image.ff > 8-bits-per-channel.txt                       \n"
		"   $ ff-printf \"%%d %%d %%d %%d                                              \n"
		"   \" + < image.ff                                                            \n"
		"   $ ff-printf \"$(echo -e '%%d-%%d-%%d-%%d\\n_')\" + < image.ff              \n"
		"\n"
	);
	exit(1);
}

int main(int argc,char**argv) {
	int i;
	if (argc>1 && (!strcmp(argv[1],"-h") || !strcmp(argv[1],"--help"))) {
		usage();
	}
	char*form=argc>1 && strlen(argv[1])?argv[1]:"%d %d %d %d\n";
	fread(buf,1,16,stdin);
	i=(buf[8]<<24)|(buf[9]<<16)|(buf[10]<<8)|buf[11];
	count=(buf[12]<<24)|(buf[13]<<16)|(buf[14]<<8)|buf[15];
	count*=i;
	if(argc>2 && argv[2][0]=='+') {
		while(count--) {
			fread(buf,1,8,stdin);
			printf(form,buf[0],buf[2],buf[4],buf[6]);
		}
	} else {
		while(count--) {
			fread(buf,1,8,stdin);
			printf(form,(buf[0]<<8)|buf[1],(buf[2]<<8)|buf[3],(buf[4]<<8)|buf[5],(buf[6]<<8)|buf[7]);
		}
	}
	return 0;
}
