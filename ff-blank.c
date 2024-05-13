#if 0
gcc -s -O2 -o ~/bin/ff-blank ff-blank.c
exit
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define fatal(...) do{ fprintf(stderr,__VA_ARGS__); exit(1); }while(0)

static unsigned char buf[8];

static void
usage(void)
{
	fprintf(stderr,
		"Usage: ff-blank <width> <height> <?colour?>                                   \n"
		"                                                                              \n"
		"This creates a blank picture with a solid colour.                             \n"
		"                                                                              \n"
		"The first two arguments are width and height. You can have zero, one, three,  \n"
		"or four more arguments.                                                       \n"
		"                                                                              \n"
		"With no extra arguments the picture is fully transparent.                     \n"
		"                                                                              \n"
		"One extra argument specifies the background colour which can be defined using \n"
		"one of these patterns:                                                        \n"
		"   rrggbb            three pairs of 8-bit hex values                          \n"
		"   rrggbbaa          four pairs of 8-bit hex values                           \n"
		"   rrrrggggbbbb      three pairs of 16-bit hex values                         \n"
		"   rrrrggggbbbbaaaa  four pairs of 16-bit hex values                          \n"
		"                                                                              \n"
		"Three or four extra arguments specifies the individual 16-bit values for the  \n"
		"red, green, blue, and the alpha channel.                                      \n"
		"If the alpha channel is not specified then 65535 is used.                     \n"
		"                                                                              \n"
		"This program does not read from standard in.                                  \n"
		"                                                                              \n"
		"Example usage:                                                                \n"
		"   $ ff-blank 640 480 > image-transparent.ff                                  \n"
		"   $ ff-blank 640 480 FF9090 > image-blank.ff                                 \n"
		"   $ ff-blank 640 480 FFFF90009000FFFF > image-blank.ff                       \n"
		"   $ ff-blank 640 480 65535 36864 36864 > image-blank.ff                      \n"
		"   $ ff-blank 640 480 65535 36864 36864 65535 > image-blank.ff                \n"
		"\n"
	);
	exit(1);
}

int main(int argc,char**argv) {
	int i;
	long s;
	if (argc>1 && (!strcmp(argv[1],"-h") || !strcmp(argv[1],"--help"))) {
		usage();
	}
	if(argc!=3 && argc!=4 && argc!=6 && argc!=7) fatal("Incorrect number of arguments\n");
	if(argc==4) {
		i=strlen(argv[3]);
		if(i!=6 && i!=8 && i!=12 && i!=16) fatal("Invalid color specification\n");
	}
	s=i=strtol(argv[1],0,0);
	buf[0]=i>>24; buf[1]=i>>16; buf[2]=i>>8; buf[3]=i;
	s*=i=strtol(argv[2],0,0);
	buf[4]=i>>24; buf[5]=i>>16; buf[6]=i>>8; buf[7]=i;
	fwrite("farbfeld",1,8,stdout);
	fwrite(buf,1,8,stdout);
	if(argc==3) {
		memset(buf,0,8);
	} else if(argc==4) {
		i=strlen(argv[3]);
		if(i==6) {
			sscanf(argv[3],"%2hhX%2hhX%2hhX",buf+0,buf+2,buf+4);
			buf[1]=buf[0];
			buf[3]=buf[2];
			buf[5]=buf[4];
			buf[7]=buf[6]=255;
		} else if(i==8) {
			sscanf(argv[3],"%2hhX%2hhX%2hhX%2hhX",buf+0,buf+2,buf+4,buf+6);
			buf[1]=buf[0];
			buf[3]=buf[2];
			buf[5]=buf[4];
			buf[7]=buf[6];
		} else if(i==12) {
			sscanf(argv[3],"%2hhX%2hhX%2hhX%2hhX%2hhX%2hhX",buf+0,buf+1,buf+2,buf+3,buf+4,buf+5);
			buf[7]=buf[6]=255;
		} else if(i==16) {
			sscanf(argv[3],"%2hhX%2hhX%2hhX%2hhX%2hhX%2hhX%2hhX%2hhX",buf+0,buf+1,buf+2,buf+3,buf+4,buf+5,buf+6,buf+7);
		}
	} else if (argc==6) {
		i=strtol(argv[3],0,0);
		buf[0]=i>>8; buf[1]=i;
		i=strtol(argv[4],0,0);
		buf[2]=i>>8; buf[3]=i;
		i=strtol(argv[5],0,0);
		buf[4]=i>>8; buf[5]=i;
		buf[6]=buf[7]=255;
	} else if(argc==7) {
		i=strtol(argv[3],0,0);
		buf[0]=i>>8; buf[1]=i;
		i=strtol(argv[4],0,0);
		buf[2]=i>>8; buf[3]=i;
		i=strtol(argv[5],0,0);
		buf[4]=i>>8; buf[5]=i;
		i=strtol(argv[6],0,0);
		buf[6]=i>>8; buf[7]=i;
	}
	while(s--) fwrite(buf,1,8,stdout);
	return 0;
}
