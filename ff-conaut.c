#if 0
gcc -s -O2 -o ~/bin/ff-conaut -Wno-unused-result ff-conaut.c
exit
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned char buf[16];
static int rule[3];
static unsigned char*rowin;
static unsigned char*rowout;
static int width;

static void
usage(void)
{
	fprintf(stderr,
		"Usage: ff-conaut <int> <?int?> <?int?>                                        \n"
		"                                                                              \n"
		"This farbfeld filter implements continuous cellular automata.                 \n"
		"                                                                              \n"
		"Technically this creates pseudo continuous automaton (which is actually using \n"
		"discrete values, but act like continuous values).                             \n"
		"                                                                              \n"
		"This is similar to ff-elementary, but the automata is different in that:      \n"
		"   - all bits of each input channel are used, and not only the high bit (they \n"
		"     are not reduced to one bit per channel)                                  \n"
		"   - the one or three arguments are not Wolfram codes but are the number 0 to \n"
		"     65535 to add to the average pixel value                                  \n"
		"   - opaque pixels are unchanged while transparent pixels are altered, which  \n"
		"     is opposite to ff-elementary                                             \n"
		"                                                                              \n"
		"The filter takes one or three arguments which are the number 0 to 65535 to add\n"
		"to the average pixel value for the red, green and blue channels.              \n"
		"                                                                              \n"
		"If only one value is passed then that same value will be used for all three   \n"
		"channels.                                                                     \n"
		"                                                                              \n"
		"Example usage:                                                                \n"
		"   $ ff-conaut 2750 < image.ff > image-conaut.ff                              \n"
		"   $ ff-conaut 52750 13921 52 < image.ff > image-conaut.ff                    \n"
		"\n"
	);
	exit(1);
}

static inline void process(int x) {
	unsigned char*p;
	unsigned char*q=rowout+x*8;
	unsigned long long c[4]={(buf[0]<<8)|buf[1],(buf[2]<<8)|buf[3],(buf[4]<<8)|buf[5],(buf[6]<<8)|buf[7]};
	int v=0;
	unsigned long long o[3];
	p=rowin+(x?x-1:width-1)*8; v+=(p[0]<<8)|p[1];
	p=rowin+x*8+2; v+=(p[0]<<8)|p[1];
	p=rowin+(x==width-1?0:x+1)*8+4; v+=(p[0]<<8)|p[1];
	o[0]=((v+rule[0])/3)&0xFFFF;
	o[1]=((v+rule[1])/3)&0xFFFF;
	o[2]=((v+rule[2])/3)&0xFFFF;
	o[0]=(o[0]*(65535-c[3])+c[0]*c[3])/65535;
	o[1]=(o[1]*(65535-c[3])+c[1]*c[3])/65535;
	o[2]=(o[2]*(65535-c[3])+c[2]*c[3])/65535;
	q[0]=o[0]>>8; q[1]=o[0];
	q[2]=o[1]>>8; q[3]=o[1];
	q[4]=o[2]>>8; q[5]=o[2];
	q[6]=buf[6]; q[7]=buf[7];
}

int main(int argc,char**argv) {
	int i;
	if (argc>1 && (!strcmp(argv[1],"-h") || !strcmp(argv[1],"--help"))) {
		usage();
	}
	if(argc==2) {
		rule[0]=rule[1]=rule[2]=strtol(argv[1],0,0);
	} else if(argc==4) {
		rule[0]=strtol(argv[1],0,0);
		rule[1]=strtol(argv[2],0,0);
		rule[2]=strtol(argv[3],0,0);
	} else {
		fprintf(stderr,"Improper number of arguments\n");
		return 1;
	}
	fread(buf,1,16,stdin);
	width=(buf[8]<<24)|(buf[9]<<16)|(buf[10]<<8)|buf[11];
	if(!(rowout=malloc(width<<4))) {
		fprintf(stderr,"Allocation failed\n");
		return 1;
	}
	rowin=rowout+width*8;
	fwrite(buf,1,16,stdout);
	fread(rowout,width,8,stdin);
	for(;;) {
		fwrite(rowout,width,8,stdout);
		memcpy(rowin,rowout,width*8);
		for(i=0;i<width;i++) {
			if(fread(buf,1,8,stdin)<=0) return 0;
			process(i);
		}
	}
}

