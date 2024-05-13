#if 0
gcc -s -O2 -o ~/bin/ff-psycho ff-psycho.c -lm
exit
#endif

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int w,h;

static void
usage(void)
{
	fprintf(stderr,
		"Usage: ff-psycho <width> <height> <code> <code> <code>                        \n"
		"                                                                              \n"
		"This farbfeld program is used to generate psychedelic art. It can also        \n"
		"generate soft gradients.                                                      \n"
		"                                                                              \n"
		"This program does not read from standard in.                                  \n"
		"                                                                              \n"
		"The filter takes five arguments:                                              \n"
		"   - width                                                                    \n"
		"   - height                                                                   \n"
		"   - code for red channel                                                     \n"
		"   - code for green channel                                                   \n"
		"   - code for blue channel                                                    \n"
		"                                                                              \n"
		"The codes are strings of digits, and are a stack-based code, where each one   \n"
		"pops zero or more items and pushes a single item. Digits 0 and 1 pop nothing, \n"
		"6 and 7 pop one item, 2 and 8 and 9 pop two items, and 3 and 4 and 5 pop that \n"
		"many items.                                                                   \n"
		"                                                                              \n"
		"Example usage:                                                                \n"
		"   $ ff-psycho 640 480 4444 1111 77777 > psycho.ff                            \n"
		"   $ ff-psycho 640 480 9923401103 11323492200 111442342010 > gradients.ff     \n"
		"   $ ff-psycho 640 480 12320078782020302303 032010291025922345667 \\          \n"
		"                       020234447921001027101243 > psycho.ff                   \n"
		"\n"
	);
	exit(1);
}

#define PUSH(z) (stack[sp=(sp+1)&31]=(z))
#define POP(z) (z=stack[sp],sp=(sp-1)&31)
static int calc(const char*s,int xi,int yi) {
	static double stack[32];
	double a,b,c,d,e;
	int sp=1;
	double xf=stack[0]=(2.0*xi)/w-1.0;
	double yf=stack[1]=(2.0*(h-yi))/h-1.0;
	while(*s) switch(*s++) {
		case '0': PUSH(xf); break;
		case '1': PUSH(yf); break;
		case '2': POP(a); POP(b); PUSH((a+b)/2.0); break;
		case '3': POP(a); POP(b); POP(c); PUSH((a+b+c)/3.0); break;
		case '4': POP(a); POP(b); POP(c); POP(d); PUSH((a+b+c+d)/4.0); break;
		case '5': POP(a); POP(b); POP(c); POP(d); POP(e); PUSH((a+b+c+d+e)/5.0); break;
		case '6': POP(a); PUSH(sin(M_PI*a)); break;
		case '7': POP(a); PUSH(cos(M_PI*a)); break;
		case '8': POP(a); POP(b); PUSH(a*b); break;
		case '9': POP(a); POP(b); PUSH(0.5*hypot(a,b)); break;
	}
	POP(a);
	return a>=1.0?65535:a<=-1.0?0:(a+1.0)*32767.5;
}

int main(int argc,char**argv) {
	static int x,y,z;
	if (argc!=6) {
		usage();
	}
	w=strtol(argv[1],0,0);
	h=strtol(argv[2],0,0);
	fwrite("farbfeld",1,8,stdout);
	putchar(w>>24); putchar(w>>16); putchar(w>>8); putchar(w);
	putchar(h>>24); putchar(h>>16); putchar(h>>8); putchar(h);
	for(y=0;y<h;y++) for(x=0;x<w;x++) {
		z=calc(argv[3],x,y); putchar(z>>8); putchar(z);
		z=calc(argv[4],x,y); putchar(z>>8); putchar(z);
		z=calc(argv[5],x,y); putchar(z>>8); putchar(z);
		putchar(255); putchar(255);
	}
	return 0;
}
