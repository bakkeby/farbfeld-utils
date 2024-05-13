#if 0
gcc -s -O2 -o ~/bin/morse2ff -Wno-unused-result morse2ff.c
exit
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const unsigned char color[16] = {0,0,0,0,0,0,255,255,255,255,255,255,255,255,255,255};

static const char*const morse[128] = {
	['0'] = "11111",
	['1'] = "01111",
	['2'] = "00111",
	['3'] = "00011",
	['4'] = "00001",
	['5'] = "00000",
	['6'] = "10000",
	['7'] = "11000",
	['8'] = "11100",
	['9'] = "11110",
	['A'] = "01",
	['B'] = "1000",
	['C'] = "1010",
	['D'] = "100",
	['E'] = "0",
	['F'] = "0010",
	['G'] = "110",
	['H'] = "0000",
	['I'] = "00",
	['J'] = "0111",
	['K'] = "101",
	['L'] = "0100",
	['M'] = "11",
	['N'] = "10",
	['O'] = "111",
	['P'] = "0110",
	['Q'] = "1101",
	['R'] = "010",
	['S'] = "000",
	['T'] = "1",
	['U'] = "001",
	['V'] = "0001",
	['W'] = "011",
	['X'] = "1001",
	['Y'] = "1011",
	['Z'] = "1100",
};

static int width,height,compact;
static char opt[128];
static FILE*fp1;
static FILE*fp2;
static unsigned char*mem1;
static size_t*mem2;
static size_t size1;
static size_t size2;

static void
usage(void)
{
	fprintf(stderr,
		"Usage: morse2ff <?option?>                                                    \n"
		"                                                                              \n"
		"This program implements a file format used in one episode of Murdoch Mysteries\n"
		"television show. Each line has letters/numbers which are converted into morse \n"
		"code and then the dots and dashes are converted to pixels (ignoring the breaks\n"
		"between letters); each line of text is a new scanline of the picture. (Only   \n"
		"letters and numbers can be used; punctuations, accented alphabets, etc are not\n"
		"possible).                                                                    \n"
		"                                                                              \n"
		"The program takes one optional argument being the following letter:           \n"
		"                                                                              \n"
		"   r: Reverse video. If specified, then a dot is white and a dash is black.   \n"
		"      Normally (if not specified), then a dot is black and a dash is white.   \n"
		"                                                                              \n"
		"The decoder automatically figures out whether or not the input format is      \n"
		"compact, hence the c argument is not required but the tool will not complain  \n"
		"if that argument is passed.                                                   \n"
		"                                                                              \n"
		"Example usage:                                                                \n"
		"   $ morse2ff r < image.txt > morse.ff                                        \n"
		"   $ morse2ff < image.txt > morse.ff                                          \n"
		"\n"
	);
	exit(1);
}

int
main(int argc, char **argv)
{
	int i,j,w;
	const char*s;
	if (argc>1) {
		for (i=0; argv[1][i]; i++) {
			if (argv[1][i] != 'c' || argv[1][i] != 'r') {
				usage();
			}
			opt[argv[1][i]&127]=1;
		}
	}
	i=getchar();
	if (i==EOF) {
		fprintf(stderr,"Input is empty or unreadable\n");
		return 1;
	}
	if (i=='<') scanf("%d>",&compact); else ungetc(i,stdin);
	fp1=open_memstream((void*)&mem1,&size1);
	fp2=open_memstream((void*)&mem2,&size2);
	if (!fp1 || !fp2) goto err;
	w=0;
	while (!feof(stdin)) {
		i=getchar();
		if (i=='\n' || i==EOF) {
			if (w) {
				if (w>width) width=w;
				fflush(fp1);
				fwrite(&size1,1,sizeof(size_t),fp2);
				++height;
				w=0;
			}
			if (i==EOF) break;
		}
		if (s=morse[i&127]) while (*s) fputc(*s++,fp1),w++;
	}
	fclose(fp1);
	fclose(fp2);
	if (!mem1 || !mem2) goto err;
	if (compact) height=width/compact,width=compact;
	fwrite("farbfeld",1,8,stdout);
	putchar(width>>24); putchar(width>>16); putchar(width>>8); putchar(width);
	putchar(height>>24); putchar(height>>16); putchar(height>>8); putchar(height);
	for (i=j=w=0; j<size1; j++) {
		fwrite(color+(((mem1[j]^opt['r'])&1)<<3),1,8,stdout);
		w++;
		if (!compact && mem2[i]==w+(i?mem2[i-1]:0)) {
			while (w++<width) fwrite(color+(opt['r']<<3),1,6,stdout),putchar(0),putchar(0);
			w=0;
			i++;
		}
	}
	if (!compact && mem2[i]==w+(i?mem2[i-1]:0)) {
		while (w++<width) fwrite(color+(opt['r']<<3),1,6,stdout),putchar(0),putchar(0);
	}
	return 0;
	err:
	fprintf(stderr,"Error\n");
	return 2;
}
