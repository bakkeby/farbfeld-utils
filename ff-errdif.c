#if 0
gcc -s -O2 -o ~/bin/ff-errdif -Wno-unused-result ff-errdif.c
exit
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned char head[16];
static double*diffuse;
static double*row;
static double here;
static double right[8];
static int rowswitch;
static int rowval;
static double matrix[12];
static double total;
static int width;
static int height;

static void
usage(void)
{
	fprintf(stderr,
		"Usage: ff-errdif <diffusion 1> ... <diffusion 12> <?divisor?>                 \n"
		"                                                                              \n"
		"This farbfeld filter is used to apply a two-level error diffusion,            \n"
		"individually per channel.                                                     \n"
		"                                                                              \n"
		"The filter takes twelve arguments and optionally a custom divisor.            \n"
		"                                                                              \n"
		"   . . * A B                                                                  \n"
		"   C D E F G                                                                  \n"
		"   H I J K L                                                                  \n"
		"                                                                              \n"
		"The twelve arguments are numbers named A to L in the above grid and these     \n"
		"indicate the amount of diffusion in that position. The asterisk indicates the \n"
		"current position.                                                             \n"
		"                                                                              \n"
		"The divisor is normally equal to the sum of these numbers, but optionally a   \n"
		"thirteenth argument can be added to specify the divisor.                      \n"
		"                                                                              \n"
		"Example usage:                                                                \n"
		"   $ ff-errdif 1.0 1.0 5.0 1.0 1.0 1.0 1.0 5.0 1.0 1.0 1.0 1.0 < i.ff > e.ff  \n"
		"\n"
	);
	exit(1);
}

static inline int get16(void) {
	int x=fgetc(stdin)<<8;
	return x|fgetc(stdin);
}

int main(int argc,char**argv) {
	int i;
	if (argc>1 && (!strcmp(argv[1],"-h") || !strcmp(argv[1],"--help"))) {
		usage();
	}
	if(argc!=13 && argc!=14) {
		fprintf(stderr,"Too %s arguments\n",argc<13?"few":"many");
		return 1;
	}
	fread(head,1,16,stdin);
	fwrite(head,1,16,stdout);
	width=(head[8]<<24)|(head[9]<<16)|(head[10]<<8)|head[11];
	height=(head[12]<<24)|(head[13]<<16)|(head[14]<<8)|head[15];
	total=0.0;
	for(i=0;i<12;i++) total+=matrix[i]=strtod(argv[i+1],0);
	if(argc==14) total=strtod(argv[13],0);
	if(total) for(i=0;i<12;i++) matrix[i]/=total;
	diffuse=calloc(sizeof(double),8*width+64);
	row=calloc(sizeof(double),4*width);
	rowval=4*width+16;
	if(!diffuse || !row) {
		fprintf(stderr,"Allocation failed\n");
		return 1;
	}
	diffuse+=16;
	while(height--) {
		memset(right,0,8*sizeof(double));
		for(i=0;i<4*width;i++) {
			here=diffuse[(rowswitch^rowval)+i]+right[i&7]+(double)get16();
			if(here>=32768.0) {
				here-=65535.0;
				putchar(255);
				putchar(255);
			} else {
				putchar(0);
				putchar(0);
			}
			right[(i+4)&7]+=matrix[0]*here;
			right[i&7]=matrix[1]*here;
			row[i]=here;
		}
		for(i=0;i<4*width;i++) {
			diffuse[rowswitch+i-8]+=matrix[2]*row[i];
			diffuse[rowswitch+i-4]+=matrix[3]*row[i];
			diffuse[rowswitch+i]+=matrix[4]*row[i];
			diffuse[rowswitch+i+4]+=matrix[5]*row[i];
			diffuse[rowswitch+i+8]+=matrix[6]*row[i];
		}
		rowswitch^=rowval;
		for(i=0;i<4*width;i++) {
			diffuse[rowswitch+i-8]=matrix[7]*row[i];
			diffuse[rowswitch+i-4]=matrix[8]*row[i];
			diffuse[rowswitch+i]=matrix[9]*row[i];
			diffuse[rowswitch+i+4]=matrix[10]*row[i];
			diffuse[rowswitch+i+8]=matrix[11]*row[i];
		}
	}
	return 0;
}
