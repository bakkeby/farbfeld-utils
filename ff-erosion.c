#if 0
gcc -s -O2 -o ~/bin/ff-erosion -Wno-unused-result ff-erosion.c -lm
exit
#endif

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define TAU (2.0*M_PI)

typedef struct {
  double x,y,z;
} Erosion;

static unsigned char buf[16];
static int width,height;
static Erosion*grid;
static int iopt[128];
static double fopt[128];

#define HMAP_INDEX(X,Y) (((X)+width)%width+(((Y)+height)%height)*width)
#define HMAP(X,Y) (grid[HMAP_INDEX(X,Y)].z)
#define XMAP(X,Y) (grid[HMAP_INDEX(X,Y)].x)
#define YMAP(X,Y) (grid[HMAP_INDEX(X,Y)].y)

static void
usage(void)
{
  fprintf(stderr,
    "Usage: ff-erosion <?opt=value?> <?opt=value?> ...                             \n"
    "                                                                              \n"
    "This farbfeld filter is used to generate erosion by raining.                  \n"
    "                                                                              \n"
    "Only the red channel of the input is used; the output is same red, green, blue\n"
    "channels, and all pixels are opaque.                                          \n"
    "                                                                              \n"
    "The filter takes zero or more arguments, where each argument is a letter      \n"
    "followed by an equal sign and a value. Many values can be floating numbers,   \n"
    "but not all.                                                                  \n"
    "                                                                              \n"
    "The possible options are:                                                     \n"
    "   b -- output level (default 1.0)                                            \n"
    "   c -- output colours (default 0)                                            \n"
    "   d -- deposit speed (default 0.2)                                           \n"
    "   e -- threshold for random directions (default 1.0e-8)                      \n"
    "   g -- gravity (default 40.0)                                                \n"
    "   i -- inertia (default 0.1)                                                 \n"
    "   m -- slope limit (default 0.05)                                            \n"
    "   n -- number of iterations (default 1)                                      \n"
    "   q -- carrying capacity (default 10.0)                                      \n"
    "   r -- erosion speed (default 0.9)                                           \n"
    "   s -- random number seed (default 1)                                        \n"
    "   w -- water evaporation speed (default 0.001)                               \n"
    "   x -- erosion multiplier (default 0.1591549430918953)                       \n"
    "   z -- maximum steps per iteration (default 4 * MAX(width,height))           \n"
    "                                                                              \n"
    "Example usage:                                                                \n"
    "   $ ff-erosion < image.ff > image-erosion.ff                                 \n"
    "   $ ff-erosion c=1 b=1.5 < image.ff > image-erosion.ff                       \n"
    "\n"
  );
  exit(1);
}

static inline void deposit(int x,int z,double f) {
  YMAP(x,z)+=f;
  HMAP(x,z)+=f;
}
#define DEPOSIT() ( \
  deposit(xi,zi,(1.0-xf)*(1.0-zf)*ds), \
  deposit(xi+1,zi,xf*(1.0-zf)*ds), \
  deposit(xi,zi+1,(1.0-xf)*zf*ds), \
  deposit(xi+1,zi+1,xf*zf*ds), \
0 )

static inline void process(void) {
  int xi,zi,mov;
  double xp,zp,xf,zf,s,v,w,h,h00,h10,h01,h11,dx,dz;
  xp=xi=((unsigned)random())%width;
  zp=zi=((unsigned)random())%height;
  xf=zf=s=v=dx=dz=0.0;
  w=1.0;
  h00=h=HMAP(xi,zi);
  h10=HMAP(xi+1,zi);
  h01=HMAP(xi,zi+1);
  h11=HMAP(xi+1,zi+1);
  for(mov=0;mov<iopt['z'];mov++) {
    double gx=h00+h01-h10-h11;
    double gz=h00+h10-h01-h11;
    dx=(dx-gx)*fopt['i']+gx;
    dz=(dz-gz)*fopt['i']+gz;
    double dl=hypot(dx,dz);
    if(dl<=fopt['e']) {
      float a=(random()/(double)RAND_MAX)*TAU;
      dx=cos(a);
      dz=sin(a);
    } else {
      dx/=dl;
      dz/=dl;
    }
    float nxp=xp+dx;
    float nzp=zp+dz;
    int nxi=nxp;
    int nzi=nzp;
    float nxf=nxp-nxi;
    float nzf=nzp-nzi;
    float nh00=HMAP(nxi,nzi);
    float nh10=HMAP(nxi+1,nzi);
    float nh01=HMAP(nxi,nzi+1);
    float nh11=HMAP(nxi+1,nzi+1);
    float nh=(nh00*(1-nxf)+nh10*nxf)*(1-nzf)+(nh01*(1-nxf)+nh11*nxf)*nzf;
    if(nh>=h) {
      double ds=(nh-h)+0.001;
      if(ds>=s) {
        ds=s;
        DEPOSIT();
        break;
      } else {
        DEPOSIT();
        h+=ds;
        s-=ds;
        v=0.0;
      }
    }
    double dh=h-nh;
    double slope=dh;
    double q=fmax(slope,fopt['m'])*v*w*fopt['q'];
    double ds=s-q;
    if(ds>=0.0) {
      ds*=fopt['d'];
      DEPOSIT();
      dh+=ds;
      s-=ds;
    } else {
      ds=fmin(ds*-fopt['r'],dh*0.99);
      int x,z;
      for(z=zi-1;z<=zi+2;z++) {
        double zo=z-zp;
        double zo2=zo*zo;
        for(x=xi-1;x<=xi+2;x++) {
          double xo=x-xp;
          double ww=1.0-(xo*xo+zo2)*0.25;
          if(ww>0.0) {
            double delta=ds*ww*fopt['x'];
            HMAP(x,z)-=delta;
            double r=XMAP(x,z);
            double d=YMAP(x,z);
            if(delta<=d) d-=delta; else r+=delta-d,d=0.0;
            XMAP(x,z)=r;
            YMAP(x,z)=d;
          }
        }
      }
      dh-=ds;
      s+=ds;
    }
    v=sqrt(v*v+dh*fopt['g']);
    w*=1.0-fopt['w'];
    xp=nxp; zp=nzp;
    xi=nxi; zi=nzi;
    xf=nxf; zf=nzf;
    h=nh;
    h00=nh00;
    h10=nh10;
    h01=nh01;
    h11=nh11;
  }
}

int main(int argc,char**argv) {
  int i,j;
  if (argc>1 && (!strcmp(argv[1],"-h") || !strcmp(argv[1],"--help"))) {
    usage();
  }
  iopt['s']=1; iopt['n']=1; iopt['z']=0; iopt['c']=0;
  fopt['q']=10.0; fopt['w']=0.001; fopt['r']=0.9; fopt['d']=0.2;
  fopt['i']=0.1; fopt['m']=0.05; fopt['g']=40.0; fopt['e']=1.0e-8;
  fopt['x']=0.1591549430918953; fopt['b']=1.0;
  for(i=1;i<argc;i++) {
    if(argv[i][0] && argv[i][1]=='=') {
      iopt[argv[i][0]&127]=strtol(argv[i]+2,0,0);
      fopt[argv[i][0]&127]=strtod(argv[i]+2,0);
    }
  }
  srandom(iopt['s']);
  fread(buf,1,16,stdin);
  width=(buf[8]<<24)|(buf[9]<<16)|(buf[10]<<8)|buf[11];
  height=(buf[12]<<24)|(buf[13]<<16)|(buf[14]<<8)|buf[15];
  if(!iopt['z']) iopt['z']=(width>height?width:height)*4;
  grid=malloc(width*height*sizeof(Erosion));
  if(!grid) {
    fprintf(stderr,"Allocation failed\n");
    return 1;
  }
  fwrite(buf,1,16,stdout);
  if(iopt['c']) {
    for(i=width*height-1;i>=0;i--) {
      fread(buf,1,8,stdin);
      grid[i].z=((buf[0]<<8)|buf[1])/65535.0;
      grid[i].x=((buf[2]<<8)|buf[3])/65535.0;
      grid[i].y=((buf[4]<<8)|buf[4])/65535.0;
    }
    while(iopt['n']--) process();
    for(i=width*height-1;i>=0;i--) {
      grid[i].x*=fopt['b'];
      grid[i].y*=fopt['b'];
      grid[i].z*=fopt['b'];
      j=grid[i].z>=1.0?65535:grid[i].z<=0.0?0:grid[i].z*65535.0;
      if(j>65535) j=65535;
      buf[0]=j>>8;
      buf[1]=j;
      j=grid[i].x>=1.0?65535:grid[i].x<=0.0?0:grid[i].x*65535.0;
      if(j>65535) j=65535;
      buf[2]=j>>8;
      buf[3]=j;
      j=grid[i].y>=1.0?65535:grid[i].y<=0.0?0:grid[i].y*65535.0;
      if(j>65535) j=65535;
      buf[4]=j>>8;
      buf[5]=j;
      buf[6]=buf[7]=255;
      fwrite(buf,1,8,stdout);
    }
  } else {
    for(i=width*height-1;i>=0;i--) {
      fread(buf,1,8,stdin);
      grid[i].z=((buf[0]<<8)|buf[1])/65535.0;
      grid[i].x=grid[i].y=0.0;
    }
    while(iopt['n']--) process();
    for(i=width*height-1;i>=0;i--) {
      grid[i].z*=fopt['b'];
      j=grid[i].z>=1.0?65535:grid[i].z<=0.0?0:grid[i].z*65535.0;
      if(j>65535) j=65535;
      buf[0]=buf[2]=buf[4]=j>>8;
      buf[1]=buf[3]=buf[5]=j;
      buf[6]=buf[7]=255;
      fwrite(buf,1,8,stdout);
    }
  }
  return 0;
}
