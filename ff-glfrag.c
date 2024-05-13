#if 0
gcc -s -O2 -o ~/bin/ff-glfrag -Wno-unused-result -Wno-implicit-function-declaration ff-glfrag.c -lGL -lSDL
exit
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <GL/gl.h>
#include <SDL/SDL.h>

#define fatal(...) do { fprintf(stderr,__VA_ARGS__); fputc(10,stderr); exit(1); } while(0)
#define MAXSIZE 0x8000

static char program_text[MAXSIZE+1];
static unsigned char*pixbuf;
static GLuint framebuf,texture,program,image;
static const char endian_check[8]={0,0,0,0,0,0,0,1};
static int width,height,minlinear,maglinear,progsize;
static int target=GL_TEXTURE_2D;
static int use_shader=1;

static void
usage(void)
{
	fprintf(stderr,
		"Usage: ff-glfrag <width> <height> <options> <?file?> ...                      \n"
		"                                                                              \n"
		"This farbfeld program that executes an OpenGL fragment program.               \n"
		"                                                                              \n"
		"The OpenGL fragment program is read from standard in.                         \n"
		"                                                                              \n"
		"The program takes three or more arguments:                                    \n"
		"   width - output image width                                                 \n"
		"   height - output image height                                               \n"
		"   options - one or more letters (no spaces)                                  \n"
		"   file - zero or more texture files to load                                  \n"
		"                                                                              \n"
		"The availble options are:                                                     \n"
		"   D -- disable OpenGL dither                                                 \n"
		"   M -- disable OpenGL multisample                                            \n"
		"   S -- enable OpenGL smooth points - draw points with proper filtering       \n"
		"        (by default draw aliased points)                                      \n"
		"   U -- disable the use of shader (option provided for performance testing)   \n"
		"   g -- enable maglinear (use GL_LINEAR for GL_TEXTURE_MAG_FILTER rather than \n"
		"        GL_NEAREST)                                                           \n"
		"   n -- enable minlinear (use GL_LINEAR for GL_TEXTURE_MIN_FILTER rather than \n"
		"        GL_NEAREST)                                                           \n"
		"   r -- enable rectangle textures                                             \n"
		"        (use GL_TEXTURE_RECTANGLE rather than GL_TEXTURE_2D)                  \n"
		"                                                                              \n"
		"Example usage:                                                                \n"
		"   $ ff-glfrag 640 480 DSg texture.ff < shader.gl > image.ff                  \n"
		"\n"
	);
	exit(1);
}

static void load_image(char*name) {
	unsigned char header[16];
	FILE*fp=fopen(name,"r");
	int w,h;
	static unsigned char*imgbuf;
	if(!fp) fatal("Cannot open texture input: %s",name);
	fread(header,1,16,fp);
	w=(header[8]<<24)|(header[9]<<16)|(header[10]<<8)|header[11];
	h=(header[12]<<24)|(header[13]<<16)|(header[14]<<8)|header[15];
	imgbuf=malloc(8*w*h);
	if(!imgbuf) fatal("Allocation failed");
	fread(imgbuf,8,w*h,fp);
	glTexImage2D(target,0,GL_RGBA16,w,h,0,GL_RGBA,GL_UNSIGNED_SHORT,imgbuf);
	free(imgbuf);
	fclose(fp);
}

int main(int argc,char**argv) {
	int i;
	if (argc<4 || !strcmp(argv[1],"-h") || !strcmp(argv[1],"--help")) {
		usage();
	}
	width=strtol(argv[1],0,0);
	height=strtol(argv[2],0,0);
	pixbuf=malloc(8*width*height);
	if(!pixbuf) fatal("Allocation failed");
	progsize=fread(program_text,1,MAXSIZE,stdin);
	if(progsize>=MAXSIZE || progsize<0) fatal("Program too large");
	program_text[progsize]=0;
	SDL_Init(SDL_INIT_VIDEO);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER,0);
	SDL_GL_SetAttribute(SDL_GL_RED_SIZE,8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE,8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE,8);
	SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE,8);
	SDL_SetVideoMode(1,1,32,SDL_OPENGL);
	for(i=0;argv[3][i];i++) {
		switch(argv[3][i]) {
			case 'D': glDisable(GL_DITHER); break;
			case 'M': glDisable(GL_MULTISAMPLE); break;
			case 'S': glEnable(GL_POINT_SMOOTH); break;
			case 'U': use_shader=0; break; // this option is provided for testing the speed
			case 'g': maglinear=1; break;
			case 'n': minlinear=1; break;
			case 'r': target=GL_TEXTURE_RECTANGLE;
		}
	}
	glPixelStorei(GL_PACK_SWAP_BYTES,(*(const short*)(endian_check+8-sizeof(short)))>>1);
	glPixelStorei(GL_UNPACK_SWAP_BYTES,(*(const short*)(endian_check+8-sizeof(short)))>>1);
	glEnable(target);
	if(use_shader) {
		glEnable(GL_FRAGMENT_PROGRAM_ARB);
		glGenProgramsARB(1,&program);
		glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB,program);
		glProgramStringARB(GL_FRAGMENT_PROGRAM_ARB,GL_PROGRAM_FORMAT_ASCII_ARB,progsize,(void*)program_text);
		if(glGetError()) fatal("OpenGL error: %s",(char*)glGetString(GL_PROGRAM_ERROR_STRING_ARB));
	}
	for(i=4;i<argc;i++) {
		glGenTextures(1,&image);
		glActiveTextureARB(GL_TEXTURE1_ARB+i-4);
		glBindTexture(target,image);
		load_image(argv[i]);
		glTexParameteri(target,GL_TEXTURE_MIN_FILTER,minlinear?GL_LINEAR:GL_NEAREST);
		glTexParameteri(target,GL_TEXTURE_MAG_FILTER,maglinear?GL_LINEAR:GL_NEAREST);
	}
	glActiveTextureARB(GL_TEXTURE0_ARB);
	glViewport(0,0,width,height);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0.0,1.0,0.0,1.0,0.0,1.0);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glGenFramebuffers(1,&framebuf);
	glBindFramebuffer(GL_FRAMEBUFFER,framebuf);
	glGenTextures(1,&texture);
	glBindTexture(GL_TEXTURE_2D,texture);
	glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA16,width,height,0,GL_RGBA,GL_UNSIGNED_SHORT,0);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,minlinear?GL_LINEAR:GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,maglinear?GL_LINEAR:GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,texture,0);
	glClear(GL_COLOR_BUFFER_BIT);
	glRecti(0,0,1,1);
	glReadPixels(0,0,width,height,GL_RGBA,GL_UNSIGNED_SHORT,pixbuf);
	fwrite("farbfeld",1,8,stdout);
	putchar(width>>24);
	putchar(width>>16);
	putchar(width>>8);
	putchar(width);
	putchar(height>>24);
	putchar(height>>16);
	putchar(height>>8);
	putchar(height);
	fwrite(pixbuf,8,width*height,stdout);
	SDL_Quit();
	return 0;
}
// Some hardware won't do 16-bits precision?
