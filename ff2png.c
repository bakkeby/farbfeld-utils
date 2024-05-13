#if 0
gcc -s -O2 -o ~/bin/ff2png ff2png.c lodepng.c
exit
#endif
/*
	Convert Farbfeld to PNG
	(public domain)
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lodepng.h"

static FILE*in_png;
static int keep_which;

static void
usage(void)
{
	fprintf(stderr,
		"Usage: ff2png <?options...?>                                                  \n"
		"                                                                              \n"
		"Convert farbfeld to PNG. ff2png reads a farbfeld image from stdin, converts it\n"
		"to PNG (16-bit RGBA) and writes the result to stdout.                         \n"
		"                                                                              \n"
		"This encoder can optionally use command-line arguments. If none are specified \n"
		"it will be using express mode which uses the default settings for everything. \n"
		"                                                                              \n"
		"Refer to the man page for command line options.                               \n"
		"                                                                              \n"
		"If you want to reduce bit depth to 8-bits-per-channel and force fully         \n"
		"transparent pixels to become zero then the last example command shows how to  \n"
		"do this by combining ff-poster and ff-transopt.                               \n"
		"                                                                              \n"
		"Example usage:                                                                \n"
		"   $ ff2png < image.ff > image.png                                            \n"
		"   $ bunzip2 < image.ff.bz2 | ff2png > image.png                              \n"
		"   $ ff-poster 8 < image.ff | ff-transopt | ff2png > image.png                \n"
		"\n"
	);
	exit(1);
}

static void
do_in_png(const unsigned char *out)
{
	unsigned char buf[8];
	const unsigned char*ptr=out+8;
	unsigned long len;
	if (fread(buf,1,8,in_png)!=8 || memcmp(out,buf,8)) {
		fprintf(stderr,"Not a PNG file\n");
		exit(1);
	}
	fwrite(out,1,8,stdout);
loop1:
	len=((ptr[0]<<24)|(ptr[1]<<16)|(ptr[2]<<8)|ptr[3])+12;
	fwrite(ptr,1,len,stdout);
	if (!memcmp(ptr+4,"IEND",4)) return;
	ptr+=len;
	if (memcmp(ptr-len+4,"IHDR",4)) goto loop1;
loop2:
	if (fread(buf,1,8,in_png)!=8) {
		perror(0);
		exit(1);
	}
	len=((buf[0]<<24)|(buf[1]<<16)|(buf[2]<<8)|buf[3])+4;
	if ((buf[7]&32) || (keep_which>1 && (buf[4]&32)) || (keep_which==1 && !(memcmp(buf+4,"cHRM",4) && memcmp(buf+4,"gAMA",4)))) {
		fwrite(buf,1,8,stdout);
		while (len--) putchar(fgetc(in_png));
	} else {
		fseek(in_png,len,SEEK_CUR);
	}
	if (memcmp(buf+4,"IEND",4)) goto loop2;
	goto loop1;
}

static void
make_gamma(LodePNGState*st, int ga)
{
	unsigned char buf[4];
	unsigned char*chunk=0;
	size_t len=0;
	buf[0]=ga>>24;
	buf[1]=ga>>16;
	buf[2]=ga>>8;
	buf[3]=ga>>0;
	lodepng_chunk_create(&chunk,&len,4,"gAMA",buf);
	lodepng_chunk_append(st->info_png.unknown_chunks_data,st->info_png.unknown_chunks_size,chunk);
}

int
main(int argc, char **argv)
{
	unsigned char head[16];
	unsigned char*in;
	unsigned char*out;
	unsigned w,h,e;
	if (argc > 1 && (!strcmp(argv[1],"-h") || !strcmp(argv[1],"--help"))) {
		usage();
	}
	size_t rsize=fread(head,1,16,stdin);

	if (rsize!=16 || memcmp("farbfeld",head,8)) {
		fprintf(stderr,"Not farbfeld\n");
		return 1;
	}
	w=(head[8]<<24)|(head[9]<<16)|(head[10]<<8)|head[11];
	h=(head[12]<<24)|(head[13]<<16)|(head[14]<<8)|head[15];
	in=malloc(w*h*8);
	if (!in) {
		fprintf(stderr,"Allocation failed\n");
		return 1;
	}
	rsize=fread(in,8,w*h,stdin);
	if (argc<2) {
		if (e=lodepng_encode_memory(&out,&rsize,in,w,h,LCT_RGBA,16)) {
			fprintf(stderr,"LodePNG: %s\n",lodepng_error_text(e));
			return 1;
		}
	} else {
		LodePNGState st;
		char t[80];
		int i;
		unsigned char*buf;
		lodepng_state_init(&st);
		st.info_raw.colortype=LCT_RGBA;
		st.info_raw.bitdepth=16;
		for (e=1; e<argc; e++) {
			switch(argv[e][0]) {
				case '=':
					strncpy(t,argv[e]+1,79);
					t[strcspn(t,"=")]=0;
					lodepng_add_text(&st.info_png,t,argv[e]+strlen(t)+2);
					break;
				case '+':
					st.info_png.color.palette=buf=malloc(1024);
					if (!buf) {
						fprintf(stderr,"Allocation failed\n");
						return 1;
					}
					st.encoder.force_palette=1;
					e++;
					goto palet;
				case '@':
					in_png=fopen(argv[e]+1,"r");
					if (!in_png) {
						perror(argv[e]+1);
						return 1;
					}
					break;
				case 'a':
					st.encoder.add_id=argv[e][1]-'0';
					break;
				case 'b':
					st.encoder.zlibsettings.btype=argv[e][1]-'0';
					break;
				case 'c':
					st.encoder.auto_convert=0;
					st.info_png.color.colortype=argv[e][1]-'0';
					st.info_png.color.bitdepth=argv[e][2]==','?strtol(argv[e]+3,0,0):8;
					st.info_png.color.palettesize=0;
					st.info_png.color.key_defined=0;
					break;
				case 'e':
					if(argv[e][1]=='s' && !st.encoder.auto_convert) {
						i="\x01\x00\x03\x01\x02\x00\x04"[st.info_png.color.colortype];
						i=(i*w*8LL)/st.info_png.color.bitdepth+1;
						st.encoder.zlibsettings.blocksize=i*strtol(argv[e]+2,(char**)&buf,0);
					} else {
						st.encoder.zlibsettings.blocksize=strtol(argv[e]+1,(char**)&buf,0);
					}
					if (*buf==',') st.encoder.zlibsettings.maxchainlength=strtol(buf+1,(char**)&buf,0);
					if (*buf==',') st.encoder.zlibsettings.maxlazymatch=strtol(buf+1,(char**)&buf,0);
					if (*buf==',') st.encoder.zlibsettings.usezeros=strtol(buf+1,(char**)&buf,0);
					if (*buf==',') st.encoder.zlibsettings.too_far=strtol(buf+1,0,0);
					break;
				case 'f':
					st.encoder.filter_palette_zero=0;
					st.encoder.filter_strategy=strtol(argv[e]+1,0,0);
					break;
				case 'g':
					make_gamma(&st,(int)(strtod(argv[e]+1,0)*100000.0+0.45));
					break;
				case 'i':
					st.info_png.interlace_method=argv[e][1]-'0';
					break;
				case 'k':
					keep_which=argv[e][1]-'0';
					break;
				case 'l':
					st.encoder.zlibsettings.lazymatching=argv[e][1]-'0';
					break;
				case 'm':
					st.encoder.zlibsettings.minmatch=strtol(argv[e]+1,0,0);
					break;
				case 'n':
					st.encoder.zlibsettings.nicematch=strtol(argv[e]+1,0,0);
					break;
				case 'p':
					st.info_png.phys_defined=1;
					st.info_png.phys_x=strtol(argv[e]+1,(char**)&buf,0);
					if (*buf==',') st.info_png.phys_y=strtol(buf+1,(char**)&buf,0);
					if (*buf==',') st.info_png.phys_unit=strtol(buf+1,0,0);
					break;
				case 'q':
					st.encoder.filter_palette_zero=0;
					st.encoder.filter_strategy=LFS_PREDEFINED;
					if (!(buf=malloc(h))) {
						fprintf(stderr,"Allocation failed\n");
						return 1;
					}
					if (argv[e][1]=='-') {
						for (i=0; i<h; i++) buf[i]=i?0:2;
					} else {
						for (i=0; i<h; i++) buf[i]=argv[e][1]-'0';
					}
					st.encoder.predefined_filters=buf;
					break;
				case 't':
					st.encoder.trans_filter=strtol(argv[e]+1,0,0);
					break;
				case 'u':
					st.encoder.zlibsettings.use_lz77=argv[e][1]-'0';
					break;
				case 'w':
					st.encoder.zlibsettings.windowsize=strtol(argv[e]+1,0,0);
					break;
				case 'z':
					st.encoder.text_compression=argv[e][1]-'0';
					break;
				default:
					if (argv[e][0]) {
						fprintf(stderr,"Unknown option: %c\n",argv[e][0]);
						return 1;
					}
			}
		}
		palet:
		for (i=0; e<argc; e++) {
			if (i==256) {
				fprintf(stderr,"Too many palette entries\n");
				return 1;
			}
			if (strlen(argv[e])==8) {
				sscanf(argv[e],"%2hhX%2hhX%2hhX%2hhX",buf+(i<<2)+0,buf+(i<<2)+1,buf+(i<<2)+2,buf+(i<<2)+3);
			} else if (strlen(argv[e])!=6) {
				fprintf(stderr,"Invalid palette specification\n");
				return 1;
			} else {
				sscanf(argv[e],"%2hhX%2hhX%2hhX",buf+(i<<2)+0,buf+(i<<2)+1,buf+(i<<2)+2);
				buf[(i<<2)|3]=255;
			}
			st.info_png.color.palettesize=++i;
		}
		if (st.encoder.zlibsettings.btype!=2 || st.info_png.interlace_method) st.encoder.zlibsettings.custom_deflate=0;
		if (e=lodepng_encode(&out,&rsize,in,w,h,&st)) {
			fprintf(stderr,"LodePNG: %s\n",lodepng_error_text(e));
			return 1;
		}
	}
	if (in_png) do_in_png(out); else fwrite(out,1,rsize,stdout);
	return 0;
}
