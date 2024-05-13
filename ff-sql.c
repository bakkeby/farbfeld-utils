#if 0
if ! test -f ./sqlite3.o; then
	echo "Compilation of this program depends on sqlite3.o being present, download and compile sqlite3 manually to get this."
	exit
fi
gcc -s -O2 -o ~/bin/ff-sql -Wno-unused-result ff-sql.c sqlite3.o -lm -ldl -lpthread
exit
#endif

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sqlite3.h"

static int width,height;
static unsigned char*pic;
static unsigned char head[16];
static sqlite3*db;

static void
usage(void)
{
	fprintf(stderr,
		"Usage: ff-sql <sql>                                                           \n"
		"                                                                              \n"
		"This farbfeld program allows for SQL statements to be used to alter a picture.\n"
		"                                                                              \n"
		"The program uses sqlite3 under the hood.                                      \n"
		"                                                                              \n"
		"There are two virtual tables named F and G.                                   \n"
		"                                                                              \n"
		"Both have the following columns:                                              \n"
		"   X -- X coordinate (zero-based, starting from left)                         \n"
		"   Y -- Y coordinate (zero-based, starting from top)                          \n"
		"   R -- red                                                                   \n"
		"   G -- green                                                                 \n"
		"   B -- blue                                                                  \n"
		"   A -- alpha                                                                 \n"
		"                                                                              \n"
		"The F table uses float numbers 0 to 1, and the G table uses integer numbers 0 \n"
		"to 65535, for the colours. The X and Y coordinates are always integers.       \n"
		"                                                                              \n"
		"Writing values out of range in the colours will be clipped in range.          \n"
		"                                                                              \n"
		"Also provides SQL functions. The functions W() and H() are the width and      \n"
		"height of the picture. The functions atan2, cos, exp, fmod, log, pow, sin,    \n"
		"sqrt, tan are like the corresponding C functions having the same name.        \n"
		"                                                                              \n"
		"DELETE is not allowed, but you can use INSERT or UPDATE; using INSERT replaces\n"
		"the pixel with the same coordinates as the inserted record. You can also      \n"
		"create your own tables; such tables are temporary and are discarded after your\n"
		"SQL statements are executed. SELECT is allowed but currently has no use       \n"
		"because no output is produced.                                                \n"
		"                                                                              \n"
		"The filter takes one argument which is the SQL string. That string can contain\n"
		"more than one SQL statement.                                                  \n"
		"                                                                              \n"
		"Example usage:                                                                \n"
		"   $ ff-sql \"update G set G=R, B=R;\" < image.ff > grayscale.ff              \n"
		"   $ ff-sql \"update G set R=0 where X>W()/2 and Y>H()/2\" < in.ff > out.ff   \n"
		"   $ ff-sql \"update F set B=sin(R)*cos(G)\" < image.ff > example.ff          \n"
		"\n"
	);
	exit(1);
}

static void binary_func(sqlite3_context*cxt,int argc,sqlite3_value**argv) {
	double(*f)(double,double)=sqlite3_user_data(cxt);
	double x=sqlite3_value_double(argv[0]);
	double y=sqlite3_value_double(argv[1]);
	sqlite3_result_double(cxt,f(x,y));
}

static void const_func(sqlite3_context*cxt,int argc,sqlite3_value**argv) {
	int*x=sqlite3_user_data(cxt);
	sqlite3_result_int(cxt,*x);
}

static void unary_func(sqlite3_context*cxt,int argc,sqlite3_value**argv) {
	double(*f)(double)=sqlite3_user_data(cxt);
	double x=sqlite3_value_double(*argv);
	sqlite3_result_double(cxt,f(x));
}

typedef struct {
	sqlite3_vtab v;
	char k;
} Vt;

typedef struct {
	sqlite3_vtab_cursor v;
	int x,y;
	char k;
} Cur;

static int xConnect(sqlite3*db1,void*aux,int argc,const char*const*argv,sqlite3_vtab**v0,char**err) {
	Vt*v=sqlite3_malloc(sizeof(Vt));
	if(!v) return SQLITE_NOMEM;
	v->k=1&~argv[2][0];
	sqlite3_declare_vtab(db1,"CREATE TABLE T(X,Y,R,G,B,A)");
	*v0=(void*)v;
	return SQLITE_OK;
}

static int xBestIndex(sqlite3_vtab*v0,sqlite3_index_info*inf) {
	return SQLITE_OK;
}

static int xDisconnect(sqlite3_vtab*v) {
	sqlite3_free(v);
	return SQLITE_OK;
}

static int xOpen(sqlite3_vtab*v0,sqlite3_vtab_cursor**c0) {
	Vt*v=(void*)v0;
	Cur*c=sqlite3_malloc(sizeof(Cur));
	if(!c) return SQLITE_NOMEM;
	c->k=v->k;
	*c0=(void*)c;
	return SQLITE_OK;
}

static int xClose(sqlite3_vtab_cursor*v) {
	sqlite3_free(v);
	return SQLITE_OK;
}

static int xFilter(sqlite3_vtab_cursor*c0,int idxNum,const char*idxStr,int argc,sqlite3_value**argv) {
	Cur*c=(void*)c0;
	c->x=c->y=0;
	return SQLITE_OK;
}

static int xNext(sqlite3_vtab_cursor*c0) {
	Cur*c=(void*)c0;
	if(++c->x==width) c->x=0,++c->y;
	return SQLITE_OK;
}

static int xEof(sqlite3_vtab_cursor*c0) {
	Cur*c=(void*)c0;
	return (c->y==height);
}

static int xColumn(sqlite3_vtab_cursor*c0,sqlite3_context*cxt,int n) {
	Cur*c=(void*)c0;
	int x;
	switch(n) {
		case 0: sqlite3_result_int(cxt,c->x); break;
		case 1: sqlite3_result_int(cxt,c->y); break;
		default:
			x=pic[(c->x+c->y*width)*8LL+1]+(pic[(c->x+c->y*width)*8LL]<<8);
			if(c->k) sqlite3_result_double(cxt,x/65535.0); else sqlite3_result_int(cxt,x);
	}
	return SQLITE_OK;
}

static int xRowid(sqlite3_vtab_cursor*c0,sqlite3_int64*p) {
	Cur*c=(void*)c0;
	*p=c->x+c->y*width;
	return SQLITE_OK;
}

static int xUpdate(sqlite3_vtab*v0,int argc,sqlite3_value**argv,sqlite3_int64*rowid) {
	Vt*v=(void*)v0;
	int x,y,i;
	sqlite3_int64 q;
	unsigned char*p;
	if(argc!=8) return SQLITE_CONSTRAINT_VTAB;
	x=sqlite3_value_int(argv[2]);
	y=sqlite3_value_int(argv[3]);
	if(x<0 || y<0 || x>=width || y>=height) return SQLITE_CONSTRAINT_VTAB;
	if(rowid) *rowid=x+y*width;
	p=pic+(x+y*width)*8LL;
	for(i=0;i<4;i++) {
		if(v->k) {
			q=sqlite3_value_double(argv[i+4])*65535.0;
		} else {
			q=sqlite3_value_int64(argv[i+4]);
		}
		if(q<0) q=0;
		if(q>65535) q=65535;
		p[0]=q>>8;
		p[1]=q;
		p+=2;
	}
	return SQLITE_OK;
}

static const sqlite3_module the_module={
	.iVersion=1,
	.xConnect=xConnect,
	.xDisconnect=xDisconnect,
	.xBestIndex=xBestIndex,
	.xOpen=xOpen,
	.xClose=xClose,
	.xFilter=xFilter,
	.xNext=xNext,
	.xEof=xEof,
	.xColumn=xColumn,
	.xRowid=xRowid,
	.xUpdate=xUpdate,
};

int main(int argc,char**argv) {
	if (argc<2 || !strcmp(argv[1],"-h") || !strcmp(argv[1],"--help")) {
		usage();
	}
	sqlite3_config(SQLITE_CONFIG_SINGLETHREAD);
	sqlite3_config(SQLITE_CONFIG_MEMSTATUS,(int)0);
	if(sqlite3_initialize()) {
		fprintf(stderr,"Error initializing SQLite\n");
		return 1;
	}
	fread(head,1,16,stdin);
	width=(head[8]<<24)|(head[9]<<16)|(head[10]<<8)|head[11];
	height=(head[12]<<24)|(head[13]<<16)|(head[14]<<8)|head[15];
	pic=malloc((width*height)<<3);
	if(!pic) {
		fprintf(stderr,"Allocation failed\n");
		return 1;
	}
	fread(pic,width,height<<3,stdin);
	if(sqlite3_open(":memory:",&db)) {
		fprintf(stderr,"Error opening database\n");
		return 1;
	}
	sqlite3_create_function(db,"ATAN2",1,SQLITE_UTF8|SQLITE_DETERMINISTIC,atan2,binary_func,0,0);
	sqlite3_create_function(db,"COS",1,SQLITE_UTF8|SQLITE_DETERMINISTIC,cos,unary_func,0,0);
	sqlite3_create_function(db,"EXP",1,SQLITE_UTF8|SQLITE_DETERMINISTIC,exp,unary_func,0,0);
	sqlite3_create_function(db,"FMOD",1,SQLITE_UTF8|SQLITE_DETERMINISTIC,fmod,binary_func,0,0);
	sqlite3_create_function(db,"H",0,SQLITE_UTF8|SQLITE_DETERMINISTIC,&height,const_func,0,0);
	sqlite3_create_function(db,"LOG",1,SQLITE_UTF8|SQLITE_DETERMINISTIC,log,unary_func,0,0);
	sqlite3_create_function(db,"POW",1,SQLITE_UTF8|SQLITE_DETERMINISTIC,pow,binary_func,0,0);
	sqlite3_create_function(db,"SIN",1,SQLITE_UTF8|SQLITE_DETERMINISTIC,sin,unary_func,0,0);
	sqlite3_create_function(db,"SQRT",1,SQLITE_UTF8|SQLITE_DETERMINISTIC,sqrt,unary_func,0,0);
	sqlite3_create_function(db,"TAN",1,SQLITE_UTF8|SQLITE_DETERMINISTIC,tan,unary_func,0,0);
	sqlite3_create_function(db,"W",0,SQLITE_UTF8|SQLITE_DETERMINISTIC,&width,const_func,0,0);
	sqlite3_create_module(db,"F",&the_module,0);
	sqlite3_create_module(db,"G",&the_module,0);
	if(sqlite3_exec(db,argv[1],0,0,0)) {
		fprintf(stderr,"%s\n",sqlite3_errmsg(db));
		return 1;
	}
	fwrite(head,1,16,stdout);
	fwrite(pic,width,height<<3,stdout);
	return 0;
}
