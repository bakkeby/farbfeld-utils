#if 0
gcc -s -O2 -o ~/bin/xpm2ff -Wno-unused-result xpm2ff.c
exit 0
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
	unsigned char data[8];
	const char*name;
} Color;

static int width,height,ncolors,cpp,form;
static int tok,tokv,nxt;
static char*buf;
static int bufsize;
static char opt[128];
static Color*palette;
static Color*colordb;
static Color*symbolic;
static int ndb,nsym;

static void
usage(void)
{
	fprintf(stderr,
		"Usage: xpm2ff <?options...?>                                                  \n"
		"                                                                              \n"
		"Converts X pixmap image to farbfeld. xpm2ff reads an X pixmap image from      \n"
		"stdin, converts that to farbfeld and writes the result to stdout.             \n"
		"                                                                              \n"
		"Command-line arguments consists of any number of options followed by any      \n"
		"number of key/value pairs for symbolic colours.                               \n"
		"                                                                              \n"
		"Valid options are:                                                            \n"
		"   -f -- The next argument is a filename, which is a colour name database     \n"
		"         (such as /usr/lib/X11/rgb.txt (which may be in a different directory \n"
		"         on your system) or another file of the same format). If not          \n"
		"         specified, colour names other than \"black\" and \"white\" will not  \n"
		"         be understood (but hex RGB colours will still work).                 \n"
		"   -g -- Use the grey palette                                                 \n"
		"   -m -- Use the monochrome palette                                           \n"
		"                                                                              \n"
		"The key/value pairs are pairs of arguments:                                   \n"
		"   - The key is a symbolic name (given after \"s\" in a XPM palette).         \n"
		"   - The value is a colour (a Farbfeld Utilities colour, not a XPM colour) to \n"
		"     use instead of the colour specified in the XPM file.                     \n"
		"                                                                              \n"
		"Example usage:                                                                \n"
		"   $ xpm2ff < image.xpm > image.ff                                            \n"
		"   $ xpm2ff -m < image.xpm > image.ff                                         \n"
		"\n"
	);
	exit(1);
}

static int pal_compare(const void*a,const void*b) {
	const Color*x=a;
	const Color*y=b;
	return strcmp(x->name,y->name);
}

static int pal_compare_cpp(const void*a,const void*b) {
	const Color*x=a;
	const Color*y=b;
	return strncmp(x->name,y->name,cpp);
}

static int decode_hex(char a,char b) {
	int x;
	if(a>='0' && a<='9') x=a-'0';
	if(a>='A' && a<='F') x=a+10-'A';
	if(a>='a' && a<='f') x=a+10-'a';
	x<<=4;
	a=b;
	if(a>='0' && a<='9') x|=a-'0';
	if(a>='A' && a<='F') x|=a+10-'A';
	if(a>='a' && a<='f') x|=a+10-'a';
	return x;
}

static void make_color_data(unsigned char*d,const char*s) {
	switch(strlen(s)) {
		case 6:
			d[0]=d[1]=decode_hex(s[0],s[1]);
			d[2]=d[3]=decode_hex(s[2],s[3]);
			d[4]=d[5]=decode_hex(s[4],s[5]);
			d[6]=d[7]=255;
			break;
		case 8:
			d[0]=d[1]=decode_hex(s[0],s[1]);
			d[2]=d[3]=decode_hex(s[2],s[3]);
			d[4]=d[5]=decode_hex(s[4],s[5]);
			d[6]=d[7]=decode_hex(s[6],s[7]);
			break;
		case 12:
			d[0]=decode_hex(s[0],s[1]);
			d[1]=decode_hex(s[2],s[3]);
			d[2]=decode_hex(s[4],s[5]);
			d[3]=decode_hex(s[6],s[7]);
			d[4]=decode_hex(s[8],s[9]);
			d[5]=decode_hex(s[10],s[11]);
			d[6]=d[7]=255;
			break;
		case 16:
			d[0]=decode_hex(s[0],s[1]);
			d[1]=decode_hex(s[2],s[3]);
			d[2]=decode_hex(s[4],s[5]);
			d[3]=decode_hex(s[6],s[7]);
			d[4]=decode_hex(s[8],s[9]);
			d[5]=decode_hex(s[10],s[11]);
			d[6]=decode_hex(s[12],s[13]);
			d[7]=decode_hex(s[14],s[15]);
			break;
		default:
			fprintf(stderr,"Invalid color format\n");
			exit(1);
	}
}

static Color*findcolor(const char*a,Color*b,int n,int e) {
	static Color own;
	Color*q;
	if(*a=='#') {
		switch(strlen(a+1)) {
			case 3:
				own.data[0]=own.data[1]=decode_hex(a[1],a[1]);
				own.data[2]=own.data[3]=decode_hex(a[2],a[2]);
				own.data[4]=own.data[5]=decode_hex(a[3],a[3]);
				break;
			case 6:
				own.data[0]=own.data[1]=decode_hex(a[1],a[2]);
				own.data[2]=own.data[3]=decode_hex(a[3],a[4]);
				own.data[4]=own.data[5]=decode_hex(a[5],a[6]);
				break;
			case 9:
				own.data[0]=decode_hex(a[1],a[2]);
				own.data[1]=decode_hex(a[3],a[1]);
				own.data[2]=decode_hex(a[4],a[5]);
				own.data[3]=decode_hex(a[6],a[4]);
				own.data[4]=decode_hex(a[7],a[8]);
				own.data[5]=decode_hex(a[9],a[7]);
				break;
			case 12:
				own.data[0]=decode_hex(a[1],a[2]);
				own.data[1]=decode_hex(a[3],a[4]);
				own.data[2]=decode_hex(a[5],a[6]);
				own.data[3]=decode_hex(a[7],a[8]);
				own.data[4]=decode_hex(a[9],a[10]);
				own.data[5]=decode_hex(a[11],a[12]);
				break;
			default:
				if(e) return 0;
				fprintf(stderr,"Color '%s' not valid\n",a);
				exit(1);
		}
		own.data[6]=own.data[7]=255;
		return &own;
	} else if(b!=symbolic && !strcasecmp(a,"None")) {
		memset(own.data,0,8);
		return &own;
	} else {
		if(n) {
			own.name=a;
			q=bsearch(&own,b,n,sizeof(Color),pal_compare);
			if(q) return q;
		}
		if(b!=symbolic) {
			if(!strcasecmp(a,"black")) {
				memset(own.data,0,6);
				own.data[6]=own.data[7]=255;
				return &own;
			}
			if(!strcasecmp(a,"white")) {
				memset(own.data,255,8);
				return &own;
			}
		}
		if(e) return 0;
		fprintf(stderr,"Color '%s' not found\n",a);
		exit(1);
	}
}

static void alloc_buf(int x) {
	if(bufsize>x) return;
	if(buf) free(buf);
	buf=malloc((bufsize=x)+2);
	if(!buf) {
		fprintf(stderr,"Allocation failed\n");
		exit(1);
	}
	*buf=0;
}

static Color*alloc_pal(int x) {
	Color*o=malloc(x*sizeof(Color));
	if(!x) o=malloc(1);
	if(!o) {
		fprintf(stderr,"Allocation failed\n");
		exit(1);
	}
	while(x--) o[x].name="\x1A";
	return o;
}

static void load_colordb(const char*filename) {
	FILE*fp=fopen(filename,"r");
	int i,j,r,g,b;
	if(!fp) {
		perror(filename);
		exit(1);
	}
	while(!feof(fp)) {
		r=g=b=-1;
		j=fscanf(fp,"%i %i %i %255[^\r\n!]",&r,&g,&b,buf);
		if(j==EOF) break;
		if(b!=-1 && *buf) ndb++;
		while(j=fgetc(fp)) if(j==EOF || j=='\n') break;
	}
	fclose(fp);
	colordb=alloc_pal(ndb);
	fp=fopen(filename,"r");
	if(!fp) {
		perror(filename);
		exit(1);
	}
	i=0;
	while(!feof(fp) && i<ndb) {
		r=g=b=-1;
		j=fscanf(fp,"%i %i %i %255[^\r\n!]",&r,&g,&b,buf);
		if(j==EOF) break;
		if(b!=-1 && *buf) {
			j=strlen(buf);
			while(j && buf[j-1]<=' ') buf[--j]=0;
			colordb[i].name=strdup(buf);
			colordb[i].data[0]=colordb[i].data[1]=r;
			colordb[i].data[2]=colordb[i].data[3]=g;
			colordb[i].data[4]=colordb[i].data[5]=b;
			colordb[i].data[6]=colordb[i].data[7]=255;
			i++;
		}
		while(j=fgetc(fp)) if(j==EOF || j=='\n') break;
	}
	fclose(fp);
	if(i!=ndb) {
		fprintf(stderr,"Unexpected error\n");
		exit(1);
	}
	qsort(colordb,ndb,sizeof(Color),pal_compare);
}

static int gettoken(void) {
	static const char hex[256]={
		['0']=1,2,3,4,5,6,7,8,9,10,
		['A']=11,12,13,14,15,16,
		['a']=11,12,13,14,15,16,
	};
	static int iscom=0;
	char*s;
	again:
	if(feof(stdin)) {
		eof:
		fprintf(stderr,"Unexpected end of file\n");
		exit(1);
	}
	while(nxt!=EOF && nxt<=' ') nxt=fgetc(stdin);
	if(nxt=='/') {
		nxt=fgetc(stdin);
		if(nxt=='*') {
			for(;;) {
				nxt=fgetc(stdin);
				if(nxt==EOF) goto eof;
				if(nxt=='*') {
					while(nxt=='*') nxt=fgetc(stdin);
					if(nxt=='/') {
						nxt=0;
						goto again;
					}
				}
			}
		} else if(nxt=='/') {
			for(;;) {
				nxt=fgetc(stdin);
				if(nxt==EOF || nxt=='\n') break;
			}
			nxt=0;
			goto again;
		} else {
			nxt='/';
			goto unex;
		}
	} else if(iscom) {
		iscom=0;
		return 0;
	} else if(nxt=='#') {
		if(!fscanf(stdin,"define %255s %i ",buf,&tokv)) {
			fprintf(stderr,"Invalid directive\n");
			exit(1);
		}
		if(s=strrchr(buf,'_')) s++; else s=buf;
		if(!strcmp(s,"format")) form=tokv;
		if(!strcmp(s,"width")) width=tokv;
		if(!strcmp(s,"height")) height=tokv;
		if(!strcmp(s,"ncolors")) ncolors=tokv;
		if(s>buf+10 && !strcmp(s-10,"chars_per_pixel")) cpp=tokv;
		nxt=0;
		goto again;
	} else if(nxt=='-' || nxt=='+' || (nxt>='0' && nxt<='9')) {
		ungetc(nxt,stdin);
		if(!fscanf(stdin,"%i",&tokv)) goto unex;
		nxt=0;
		return tok=0;
	} else if(nxt=='{' || nxt=='}' || nxt==';' || nxt=='=' || nxt=='[' || nxt==']' || nxt==',' || nxt=='*') {
		tok=nxt;
		return nxt=0;
	} else if(nxt=='"') {
		int i=0;
		for(;;) {
			nxt=fgetc(stdin);
			nxtq:
			if(nxt==EOF) {
				goto eof;
			} else if(nxt=='"') {
				do nxt=fgetc(stdin); while(nxt<=' ' && nxt!=EOF);
				if(nxt=='/') iscom=1,gettoken();
				if(nxt!='"') break;
			} else if(i==bufsize) {
				fprintf(stderr,"String literal too long\n");
				exit(1);
			} else if(nxt=='\\') {
				nxt=fgetc(stdin);
				if(nxt==EOF) {
					goto eof;
				} else if(nxt=='\\' || nxt=='"' || nxt=='?' || nxt=='\'') {
					buf[i++]=nxt;
				} else if(nxt=='x') {
					buf[i]=0;
					for(;;) {
						nxt=fgetc(stdin);
						if(!hex[nxt]) break;
						buf[i]=(buf[i]<<4)+hex[nxt]-1;
					}
					i++;
					goto nxtq;
				} else if(nxt>='0' && nxt<'8') {
					buf[i]=nxt-'0';
					nxt=fgetc(stdin);
					if(nxt<'0' || nxt>='8') { i++; goto nxtq; }
					buf[i]=(buf[i]<<3)|(nxt-'0');
					nxt=fgetc(stdin);
					if(nxt<'0' || nxt>='8') { i++; goto nxtq; }
					buf[i]=(buf[i]<<3)|(nxt-'0');
					i++;
				} else {
					if(nxt>='a' && nxt<='v') {
						buf[i++]="\a\b..\x1B\f.......\n...\r.\t.\v"[nxt-'a'];
						if(buf[i-1]=='.') goto unex;
					} else {
						goto unex;
					}
				}
			} else {
				buf[i++]=nxt;
			}
		}
		tok='"';
		return buf[tokv=i]=0;
	} else if(fscanf(stdin,"%255[A-Za-z0-9_$]",buf+1)) {
		*buf=nxt;
		nxt=0;
		tok=1;
		if(!strcmp(buf,"static")) return tokv=1;
		if(!strcmp(buf,"unsigned")) return tokv=2;
		if(!strcmp(buf,"char")) return tokv=3;
		if(!strcmp(buf,"const")) return tokv=6;
		if(s=strrchr(buf,'_')) s++; else s=buf;
		if(!strcmp(s,"colors")) return tokv=4;
		if(!strcmp(s,"pixels")) return tokv=5;
		return tokv=0;
	} else {
		unex:
		fprintf(stderr,"Unexpected character '%c'\n",nxt);
		exit(1);
	}
}

static void badtoken(void) {
	fprintf(stderr,"Invalid X pixmap file\n");
	exit(1);
}

static void getstring(void) {
	int i=0;
	if(form==2) {
		if(!nxt) nxt=fgetc(stdin);
		while(nxt && nxt!='\n' && nxt!=EOF) {
			if(i<bufsize) buf[i++]=nxt;
			nxt=fgetc(stdin);
		}
		buf[i]=0;
		nxt=0;
	} else {
		gettoken();
		if(tok!=',') badtoken();
		gettoken();
		if(tok!='"') badtoken();
	}
}

static void format_1_colors(void) {
	int i=0;
	palette=alloc_pal(ncolors);
	for(i=0;i<ncolors;i++) {
		gettoken();
		if(tok==',') gettoken();
		if(tok!='"') badtoken();
		if(!(palette[i].name=strdup(buf))) {
			fprintf(stderr,"Allocation failed\n");
			exit(1);
		}
		gettoken();
		if(tok==',') gettoken();
		if(tok!='"') badtoken();
		memcpy(palette[i].data,findcolor(buf,colordb,ndb,0)->data,8);
	}
}

static void heading(void) {
	int c,i,j,k,s,t;
	while(!*buf || *buf=='!') getstring();
	if(sscanf(buf,"%i %i %i %i",&width,&height,&ncolors,&cpp)<4) badtoken();
	if(ncolors<1 || cpp<1) badtoken();
	palette=alloc_pal(ncolors);
	for(i=0;i<ncolors;i++) {
		getstring();
		if(strlen(buf)<=cpp) badtoken();
		j=buf[cpp];
		buf[cpp]=0;
		if(!(palette[i].name=strdup(buf))) {
			fprintf(stderr,"Allocation failed\n");
			exit(1);
		}
		buf[cpp]=j;
		j=cpp;
		s=0;
		nfield:
		while(buf[j]==' ' || buf[j]=='\t') j++;
		if(buf[j]=='!' || !buf[j]) {
			if(s) continue;
			fprintf(stderr,"Color not defined\n");
			exit(1);
		}
		if(buf[j+1]==' ') {
			c=buf[j++];
			while(buf[j]==' ' || buf[j]=='\t') j++;
			if(buf[j]=='!' || !buf[j]) goto nfield;
			for(k=0,t=j;buf[j];j++) {
				if(buf[j]==' ' || buf[j]=='\t') {
					k=j;
				} else if(k && (buf[j+1]==' ' || buf[j+1]=='\t')) {
					break;
				} else if(k && buf[j]=='g' && buf[j+1]=='4' && (buf[j+2]==' ' || buf[j+2]=='\t')) {
					break;
				}
			}
			if(!buf[j]) k=0;
			if(k) j=k+1;
			buf[k]=0;
			switch(c) {
				case 'c':
					if(!s && !opt['m'] && !opt['g']) memcpy(palette[i].data,findcolor(buf+t,colordb,ndb,0)->data,8),s=1;
					break;
				case 'g': case 'm':
					if(opt[c] && !s) memcpy(palette[i].data,findcolor(buf+t,colordb,ndb,0)->data,8),s=1;
					break;
				case 's':
					if(nsym) {
						Color*z=findcolor(buf+t,symbolic,nsym,1);
						if(z) {
							memcpy(palette[i].data,z->data,8);
							s=1;
						}
					}
					break;
			}
		} else {
			while(buf[j] && buf[j]!=' ' && buf[j]!='\t' && buf[j]!='!') j++;
		}
		goto nfield;
	}
}

static void dump_database(Color*c,int n) {
	int i;
	for(i=0;i<n;i++) printf("\"%s\" = #%02X%02X%02X%02X%02X%02X%02X%02X\n",c[i].name
	 ,c[i].data[0],c[i].data[1],c[i].data[2],c[i].data[3],c[i].data[4],c[i].data[5],c[i].data[6],c[i].data[7]);
}

static void out_header(void) {
	fwrite("farbfeld",1,8,stdout);
	putchar(width>>24); putchar(width>>16); putchar(width>>8); putchar(width);
	putchar(height>>24); putchar(height>>16); putchar(height>>8); putchar(height);
}

static void do_picture(void) {
	int x,y,i;
	Color*c;
	Color own;
	for(y=0;y<height;y++) {
		getstring();
		for(x=i=0;x<width;x++) {
			own.name=buf+i;
			c=bsearch(&own,palette,ncolors,sizeof(Color),pal_compare_cpp);
			if(!c) badtoken();
			fwrite(c->data,1,8,stdout);
			i+=cpp;
		}
	}
}

int main(int argc,char**argv) {
	int i;
	alloc_buf(1000);
	if (argc > 1 && (!strcmp(argv[1],"-h") || !strcmp(argv[1],"--help"))) {
		usage();
	}
	symbolic=alloc_pal(argc>>1);
	for(i=1;i<argc;) {
		if(argv[i][0]=='-') {
			opt[argv[i][1]&127]=1;
			if(argv[i][1]=='f' && i<argc-1) load_colordb(argv[++i]);
			i++;
		} else if(argv[i][0]) {
			if(argc==i+1) {
				usage();
			}
			symbolic[nsym].name=argv[i];
			make_color_data(symbolic[nsym].data,argv[i+1]);
			nsym++;
			i+=2;
		} else {
			i++;
		}
	}
	if(nsym) qsort(symbolic,nsym,sizeof(Color),pal_compare);
	if(!colordb) colordb=alloc_pal(0);
	if(opt['D']) {
		if(opt['f']) dump_database(colordb,ndb);
		if(nsym) dump_database(symbolic,nsym);
		return 0;
	}
	do nxt=fgetc(stdin); while(nxt<=' ' && nxt!=EOF);
	if(nxt=='!' || (nxt>='0' && nxt<='9')) form=2; else gettoken();
	if(form==1) {
		if(!width || !height || !cpp) badtoken();
		// Read _colors block
		if(tok==1 && tokv==1) gettoken();
		if(tok==1 && tokv==6) gettoken();
		if(tok==1 && tokv==2) gettoken();
		if(tok!=1 || tokv!=3) badtoken();
		gettoken();
		if(tok!='*') badtoken();
		gettoken();
		if(tok==1 && tokv==6) gettoken();
		if(tok!=1 || tokv!=4) badtoken();
		gettoken();
		if(tok!='[') badtoken();
		gettoken();
		if(!tok) gettoken();
		if(tok!=']') badtoken();
		gettoken();
		if(tok!='=') badtoken();
		gettoken();
		if(tok!='{') badtoken();
		format_1_colors();
		gettoken();
		if(tok==',') gettoken();
		if(tok!='}') badtoken();
		gettoken();
		if(tok!=';') badtoken();
		// Read _pixels block
		gettoken();
		if(tok==1 && tokv==1) gettoken();
		if(tok==1 && tokv==6) gettoken();
		if(tok==1 && tokv==2) gettoken();
		if(tok!=1 || tokv!=3) badtoken();
		gettoken();
		if(tok!='*') badtoken();
		gettoken();
		if(tok==1 && tokv==6) gettoken();
		if(tok!=1 || tokv!=5) badtoken();
		gettoken();
		if(tok!='[') badtoken();
		gettoken();
		if(!tok) gettoken();
		if(tok!=']') badtoken();
		gettoken();
		if(tok!='=') badtoken();
		gettoken();
		if(tok!='{') badtoken();
		nxt=',';
	} else {
		if(form!=2) {
			if(tok==1 && tokv==1) gettoken();
			if(tok==1 && tokv==6) gettoken();
			if(tok==1 && tokv==2) gettoken();
			if(tok!=1 || tokv!=3) badtoken();
			gettoken();
			if(tok!='*') badtoken();
			gettoken();
			if(tok==1 && tokv==6) gettoken();
			if(tok!=1) badtoken();
			gettoken();
			if(tok!='[') badtoken();
			gettoken();
			if(!tok) gettoken();
			if(tok!=']') badtoken();
			gettoken();
			if(tok!='=') badtoken();
			gettoken();
			if(tok!='{') badtoken();
			nxt=',';
			*buf='!';
		}
		heading();
	}
	alloc_buf(width*cpp);
	qsort(palette,ncolors,sizeof(Color),pal_compare_cpp);
	out_header();
	do_picture();
	return 0;
}
