#if 0
gcc -s -O2 -o ~/bin/ff-shrink -Wno-unused-result ff-shrink.c
exit
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
	unsigned char data[8];
} Pixel;

static unsigned char buf[16];
static int width,height,xlarge,ylarge,mode;
static Pixel*row;
static long red,green,blue,alpha;

static void
usage(void)
{
	fprintf(stderr,
		"Usage: ff-shrink <?x factor?> <?y factor?> <?option?>                         \n"
		"                                                                              \n"
		"This farbfeld filter that shrinks an image by ignoring some pixels.           \n"
		"                                                                              \n"
		"The filter works similarly to ff-enlarge, but shrinks the picture instead of  \n"
		"making it larger.                                                             \n"
		"                                                                              \n"
		"The filter takes up to three optional arguments:                              \n"
		"   X factor (defaults to 2)                                                   \n"
		"   Y factor (defaults to value of X factor)                                   \n"
		"   option (affects how the image is reduced)                                  \n"
		"                                                                              \n"
		"The following options are available:                                          \n"
		"   a -- average                                                               \n"
		"   c -- corner (default)                                                      \n"
		"   m -- maximum                                                               \n"
		"                                                                              \n"
		"Example usage:                                                                \n"
		"   $ ff-shrink < image.ff > image-shrinked.ff                                 \n"
		"   $ ff-shrink 4 < image.ff > image-shrinked.ff                               \n"
		"   $ ff-shrink 3 3 a < image.ff > image-shrinked.ff                           \n"
		"\n"
	);
	exit(1);
}

int main(int argc,char**argv) {
	int i,j,k;
	if (argc>1 && (!strcmp(argv[1],"-h") || !strcmp(argv[1],"--help"))) {
		usage();
	}
	xlarge=argc>1?strtol(argv[1],0,0):2;
	ylarge=argc>2?strtol(argv[2],0,0):xlarge;
	mode=argc>3?argv[3][0]:'c';
	fread(buf,1,16,stdin);
	width=(buf[8]<<24)|(buf[9]<<16)|(buf[10]<<8)|buf[11];
	height=(buf[12]<<24)|(buf[13]<<16)|(buf[14]<<8)|buf[15];
	row=calloc(8,width*ylarge);
	if(!row) {
		fprintf(stderr,"Out of memory\n");
		return 1;
	}
	buf[8]=(width/xlarge)>>24;
	buf[9]=(width/xlarge)>>16;
	buf[10]=(width/xlarge)>>8;
	buf[11]=(width/xlarge);
	buf[12]=(height/ylarge)>>24;
	buf[13]=(height/ylarge)>>16;
	buf[14]=(height/ylarge)>>8;
	buf[15]=(height/ylarge);
	fwrite(buf,1,16,stdout);
	while(height--) {
		fread(row,8,width*ylarge,stdin);
		for(i=0;i<width/xlarge;i++) {
			switch(mode) {
				case 'a':
					red=green=blue=alpha=0;
					for(j=0;j<xlarge;j++) for(k=0;k<ylarge;k++) {
						red+=(row[i*xlarge+j+k*width].data[0]<<8)|row[i*xlarge+j+k*width].data[1];
						green+=(row[i*xlarge+j+k*width].data[2]<<8)|row[i*xlarge+j+k*width].data[3];
						blue+=(row[i*xlarge+j+k*width].data[4]<<8)|row[i*xlarge+j+k*width].data[5];
						alpha+=(row[i*xlarge+j+k*width].data[6]<<8)|row[i*xlarge+j+k*width].data[6];
					}
					red/=xlarge*ylarge;
					green/=xlarge*ylarge;
					blue/=xlarge*ylarge;
					alpha/=xlarge*ylarge;
					break;
				case 'c':
					red=(row[i*xlarge].data[0]<<8)|row[i*xlarge].data[1];
					green=(row[i*xlarge].data[2]<<8)|row[i*xlarge].data[3];
					blue=(row[i*xlarge].data[4]<<8)|row[i*xlarge].data[5];
					alpha=(row[i*xlarge].data[6]<<8)|row[i*xlarge].data[7];
					break;
				case 'm':
					red=green=blue=alpha=0;
					for(j=0;j<xlarge;j++) for(k=0;k<ylarge;k++) {
						if(red<((row[i*xlarge+j+k*width].data[0]<<8)|row[i*xlarge+j+k*width].data[1])) red=(row[i*xlarge+j+k*width].data[0]<<8)|row[i*xlarge+j+k*width].data[1];
						if(green<((row[i*xlarge+j+k*width].data[2]<<8)|row[i*xlarge+j+k*width].data[3])) green=(row[i*xlarge+j+k*width].data[2]<<8)|row[i*xlarge+j+k*width].data[3];
						if(blue<((row[i*xlarge+j+k*width].data[4]<<8)|row[i*xlarge+j+k*width].data[5])) blue=(row[i*xlarge+j+k*width].data[4]<<8)|row[i*xlarge+j+k*width].data[5];
						if(alpha<((row[i*xlarge+j+k*width].data[6]<<8)|row[i*xlarge+j+k*width].data[7])) alpha=(row[i*xlarge+j+k*width].data[6]<<8)|row[i*xlarge+j+k*width].data[6];
					}
					break;
			}
			putchar(red>>8);
			putchar(red);
			putchar(green>>8);
			putchar(green);
			putchar(blue>>8);
			putchar(blue);
			putchar(alpha>>8);
			putchar(alpha);
		}
	}
	return 0;
}
