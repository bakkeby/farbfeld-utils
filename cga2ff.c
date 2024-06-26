#if 0
gcc -s -O2 -o ~/bin/cga2ff -Wno-unused-result cga2ff.c
exit
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define fatal(...) do { fprintf(stderr,__VA_ARGS__); fputc('\n',stderr); exit(1); } while(0)

static const unsigned char rom[]={
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7E, 0x81, 0xA5, 0x81,
	0xBD, 0x99, 0x81, 0x7E, 0x7E, 0xFF, 0xDB, 0xFF, 0xC3, 0xE7, 0xFF, 0x7E,
	0x6C, 0xFE, 0xFE, 0xFE, 0x7C, 0x38, 0x10, 0x00, 0x10, 0x38, 0x7C, 0xFE,
	0x7C, 0x38, 0x10, 0x00, 0x38, 0x7C, 0x38, 0xFE, 0xFE, 0xD6, 0x10, 0x38,
	0x10, 0x10, 0x38, 0x7C, 0xFE, 0x7C, 0x10, 0x38, 0x00, 0x00, 0x18, 0x3C,
	0x3C, 0x18, 0x00, 0x00, 0xFF, 0xFF, 0xE7, 0xC3, 0xC3, 0xE7, 0xFF, 0xFF,
	0x00, 0x3C, 0x66, 0x42, 0x42, 0x66, 0x3C, 0x00, 0xFF, 0xC3, 0x99, 0xBD,
	0xBD, 0x99, 0xC3, 0xFF, 0x0F, 0x03, 0x05, 0x7D, 0x84, 0x84, 0x84, 0x78,
	0x3C, 0x42, 0x42, 0x42, 0x3C, 0x18, 0x7E, 0x18, 0x3F, 0x21, 0x3F, 0x20,
	0x20, 0x60, 0xE0, 0xC0, 0x3F, 0x21, 0x3F, 0x21, 0x23, 0x67, 0xE6, 0xC0,
	0x18, 0xDB, 0x3C, 0xE7, 0xE7, 0x3C, 0xDB, 0x18, 0x80, 0xE0, 0xF8, 0xFE,
	0xF8, 0xE0, 0x80, 0x00, 0x02, 0x0E, 0x3E, 0xFE, 0x3E, 0x0E, 0x02, 0x00,
	0x18, 0x3C, 0x7E, 0x18, 0x18, 0x7E, 0x3C, 0x18, 0x24, 0x24, 0x24, 0x24,
	0x24, 0x00, 0x24, 0x00, 0x7F, 0x92, 0x92, 0x72, 0x12, 0x12, 0x12, 0x00,
	0x3E, 0x63, 0x38, 0x44, 0x44, 0x38, 0xCC, 0x78, 0x00, 0x00, 0x00, 0x00,
	0x7E, 0x7E, 0x7E, 0x00, 0x18, 0x3C, 0x7E, 0x18, 0x7E, 0x3C, 0x18, 0xFF,
	0x10, 0x38, 0x7C, 0x54, 0x10, 0x10, 0x10, 0x00, 0x10, 0x10, 0x10, 0x54,
	0x7C, 0x38, 0x10, 0x00, 0x00, 0x18, 0x0C, 0xFE, 0x0C, 0x18, 0x00, 0x00,
	0x00, 0x30, 0x60, 0xFE, 0x60, 0x30, 0x00, 0x00, 0x00, 0x00, 0x40, 0x40,
	0x40, 0x7E, 0x00, 0x00, 0x00, 0x24, 0x66, 0xFF, 0x66, 0x24, 0x00, 0x00,
	0x00, 0x10, 0x38, 0x7C, 0xFE, 0xFE, 0x00, 0x00, 0x00, 0xFE, 0xFE, 0x7C,
	0x38, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x10, 0x38, 0x38, 0x10, 0x10, 0x00, 0x10, 0x00, 0x24, 0x24, 0x24, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x24, 0x24, 0x7E, 0x24, 0x7E, 0x24, 0x24, 0x00,
	0x18, 0x3E, 0x40, 0x3C, 0x02, 0x7C, 0x18, 0x00, 0x00, 0x62, 0x64, 0x08,
	0x10, 0x26, 0x46, 0x00, 0x30, 0x48, 0x30, 0x56, 0x88, 0x88, 0x76, 0x00,
	0x10, 0x10, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x20, 0x40, 0x40,
	0x40, 0x20, 0x10, 0x00, 0x20, 0x10, 0x08, 0x08, 0x08, 0x10, 0x20, 0x00,
	0x00, 0x44, 0x38, 0xFE, 0x38, 0x44, 0x00, 0x00, 0x00, 0x10, 0x10, 0x7C,
	0x10, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x10, 0x20,
	0x00, 0x00, 0x00, 0x7E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x10, 0x10, 0x00, 0x00, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x00,
	0x3C, 0x42, 0x46, 0x4A, 0x52, 0x62, 0x3C, 0x00, 0x10, 0x30, 0x50, 0x10,
	0x10, 0x10, 0x7C, 0x00, 0x3C, 0x42, 0x02, 0x0C, 0x30, 0x42, 0x7E, 0x00,
	0x3C, 0x42, 0x02, 0x1C, 0x02, 0x42, 0x3C, 0x00, 0x08, 0x18, 0x28, 0x48,
	0xFE, 0x08, 0x1C, 0x00, 0x7E, 0x40, 0x7C, 0x02, 0x02, 0x42, 0x3C, 0x00,
	0x1C, 0x20, 0x40, 0x7C, 0x42, 0x42, 0x3C, 0x00, 0x7E, 0x42, 0x04, 0x08,
	0x10, 0x10, 0x10, 0x00, 0x3C, 0x42, 0x42, 0x3C, 0x42, 0x42, 0x3C, 0x00,
	0x3C, 0x42, 0x42, 0x3E, 0x02, 0x04, 0x38, 0x00, 0x00, 0x10, 0x10, 0x00,
	0x00, 0x10, 0x10, 0x00, 0x00, 0x10, 0x10, 0x00, 0x00, 0x10, 0x10, 0x20,
	0x08, 0x10, 0x20, 0x40, 0x20, 0x10, 0x08, 0x00, 0x00, 0x00, 0x7E, 0x00,
	0x00, 0x7E, 0x00, 0x00, 0x10, 0x08, 0x04, 0x02, 0x04, 0x08, 0x10, 0x00,
	0x3C, 0x42, 0x02, 0x04, 0x08, 0x00, 0x08, 0x00, 0x3C, 0x42, 0x5E, 0x52,
	0x5E, 0x40, 0x3C, 0x00, 0x18, 0x24, 0x42, 0x42, 0x7E, 0x42, 0x42, 0x00,
	0x7C, 0x22, 0x22, 0x3C, 0x22, 0x22, 0x7C, 0x00, 0x1C, 0x22, 0x40, 0x40,
	0x40, 0x22, 0x1C, 0x00, 0x78, 0x24, 0x22, 0x22, 0x22, 0x24, 0x78, 0x00,
	0x7E, 0x22, 0x28, 0x38, 0x28, 0x22, 0x7E, 0x00, 0x7E, 0x22, 0x28, 0x38,
	0x28, 0x20, 0x70, 0x00, 0x1C, 0x22, 0x40, 0x40, 0x4E, 0x22, 0x1E, 0x00,
	0x42, 0x42, 0x42, 0x7E, 0x42, 0x42, 0x42, 0x00, 0x38, 0x10, 0x10, 0x10,
	0x10, 0x10, 0x38, 0x00, 0x0E, 0x04, 0x04, 0x04, 0x44, 0x44, 0x38, 0x00,
	0x62, 0x24, 0x28, 0x30, 0x28, 0x24, 0x63, 0x00, 0x70, 0x20, 0x20, 0x20,
	0x20, 0x22, 0x7E, 0x00, 0x63, 0x55, 0x49, 0x41, 0x41, 0x41, 0x41, 0x00,
	0x62, 0x52, 0x4A, 0x46, 0x42, 0x42, 0x42, 0x00, 0x18, 0x24, 0x42, 0x42,
	0x42, 0x24, 0x18, 0x00, 0x7C, 0x22, 0x22, 0x3C, 0x20, 0x20, 0x70, 0x00,
	0x3C, 0x42, 0x42, 0x42, 0x4A, 0x3C, 0x03, 0x00, 0x7C, 0x22, 0x22, 0x3C,
	0x28, 0x24, 0x72, 0x00, 0x3C, 0x42, 0x40, 0x3C, 0x02, 0x42, 0x3C, 0x00,
	0x7F, 0x49, 0x08, 0x08, 0x08, 0x08, 0x1C, 0x00, 0x42, 0x42, 0x42, 0x42,
	0x42, 0x42, 0x3C, 0x00, 0x41, 0x41, 0x41, 0x41, 0x22, 0x14, 0x08, 0x00,
	0x41, 0x41, 0x41, 0x49, 0x49, 0x49, 0x36, 0x00, 0x41, 0x22, 0x14, 0x08,
	0x14, 0x22, 0x41, 0x00, 0x41, 0x22, 0x14, 0x08, 0x08, 0x08, 0x1C, 0x00,
	0x7F, 0x42, 0x04, 0x08, 0x10, 0x21, 0x7F, 0x00, 0x78, 0x40, 0x40, 0x40,
	0x40, 0x40, 0x78, 0x00, 0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x00,
	0x78, 0x08, 0x08, 0x08, 0x08, 0x08, 0x78, 0x00, 0x10, 0x28, 0x44, 0x82,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF,
	0x10, 0x10, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3C, 0x02,
	0x3E, 0x42, 0x3F, 0x00, 0x60, 0x20, 0x20, 0x2E, 0x31, 0x31, 0x2E, 0x00,
	0x00, 0x00, 0x3C, 0x42, 0x40, 0x42, 0x3C, 0x00, 0x06, 0x02, 0x02, 0x3A,
	0x46, 0x46, 0x3B, 0x00, 0x00, 0x00, 0x3C, 0x42, 0x7E, 0x40, 0x3C, 0x00,
	0x0C, 0x12, 0x10, 0x38, 0x10, 0x10, 0x38, 0x00, 0x00, 0x00, 0x3D, 0x42,
	0x42, 0x3E, 0x02, 0x7C, 0x60, 0x20, 0x2C, 0x32, 0x22, 0x22, 0x62, 0x00,
	0x10, 0x00, 0x30, 0x10, 0x10, 0x10, 0x38, 0x00, 0x02, 0x00, 0x06, 0x02,
	0x02, 0x42, 0x42, 0x3C, 0x60, 0x20, 0x24, 0x28, 0x30, 0x28, 0x26, 0x00,
	0x30, 0x10, 0x10, 0x10, 0x10, 0x10, 0x38, 0x00, 0x00, 0x00, 0x76, 0x49,
	0x49, 0x49, 0x49, 0x00, 0x00, 0x00, 0x5C, 0x62, 0x42, 0x42, 0x42, 0x00,
	0x00, 0x00, 0x3C, 0x42, 0x42, 0x42, 0x3C, 0x00, 0x00, 0x00, 0x6C, 0x32,
	0x32, 0x2C, 0x20, 0x70, 0x00, 0x00, 0x36, 0x4C, 0x4C, 0x34, 0x04, 0x0E,
	0x00, 0x00, 0x6C, 0x32, 0x22, 0x20, 0x70, 0x00, 0x00, 0x00, 0x3E, 0x40,
	0x3C, 0x02, 0x7C, 0x00, 0x10, 0x10, 0x7C, 0x10, 0x10, 0x12, 0x0C, 0x00,
	0x00, 0x00, 0x42, 0x42, 0x42, 0x46, 0x3A, 0x00, 0x00, 0x00, 0x41, 0x41,
	0x22, 0x14, 0x08, 0x00, 0x00, 0x00, 0x41, 0x49, 0x49, 0x49, 0x36, 0x00,
	0x00, 0x00, 0x44, 0x28, 0x10, 0x28, 0x44, 0x00, 0x00, 0x00, 0x42, 0x42,
	0x42, 0x3E, 0x02, 0x7C, 0x00, 0x00, 0x7C, 0x08, 0x10, 0x20, 0x7C, 0x00,
	0x0C, 0x10, 0x10, 0x60, 0x10, 0x10, 0x0C, 0x00, 0x10, 0x10, 0x10, 0x00,
	0x10, 0x10, 0x10, 0x00, 0x30, 0x08, 0x08, 0x06, 0x08, 0x08, 0x30, 0x00,
	0x32, 0x4C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x14, 0x22,
	0x41, 0x41, 0x7F, 0x00, 0x3C, 0x42, 0x40, 0x42, 0x3C, 0x0C, 0x02, 0x3C,
	0x00, 0x44, 0x00, 0x44, 0x44, 0x44, 0x3E, 0x00, 0x0C, 0x00, 0x3C, 0x42,
	0x7E, 0x40, 0x3C, 0x00, 0x3C, 0x42, 0x38, 0x04, 0x3C, 0x44, 0x3E, 0x00,
	0x42, 0x00, 0x38, 0x04, 0x3C, 0x44, 0x3E, 0x00, 0x30, 0x00, 0x38, 0x04,
	0x3C, 0x44, 0x3E, 0x00, 0x10, 0x00, 0x38, 0x04, 0x3C, 0x44, 0x3E, 0x00,
	0x00, 0x00, 0x3C, 0x40, 0x40, 0x3C, 0x06, 0x1C, 0x3C, 0x42, 0x3C, 0x42,
	0x7E, 0x40, 0x3C, 0x00, 0x42, 0x00, 0x3C, 0x42, 0x7E, 0x40, 0x3C, 0x00,
	0x30, 0x00, 0x3C, 0x42, 0x7E, 0x40, 0x3C, 0x00, 0x24, 0x00, 0x18, 0x08,
	0x08, 0x08, 0x1C, 0x00, 0x7C, 0x82, 0x30, 0x10, 0x10, 0x10, 0x38, 0x00,
	0x30, 0x00, 0x18, 0x08, 0x08, 0x08, 0x1C, 0x00, 0x42, 0x18, 0x24, 0x42,
	0x7E, 0x42, 0x42, 0x00, 0x18, 0x18, 0x00, 0x3C, 0x42, 0x7E, 0x42, 0x00,
	0x0C, 0x00, 0x7C, 0x20, 0x38, 0x20, 0x7C, 0x00, 0x00, 0x00, 0x33, 0x0C,
	0x3F, 0x44, 0x3B, 0x00, 0x1F, 0x24, 0x44, 0x7F, 0x44, 0x44, 0x47, 0x00,
	0x18, 0x24, 0x00, 0x3C, 0x42, 0x42, 0x3C, 0x00, 0x00, 0x42, 0x00, 0x3C,
	0x42, 0x42, 0x3C, 0x00, 0x20, 0x10, 0x00, 0x3C, 0x42, 0x42, 0x3C, 0x00,
	0x18, 0x24, 0x00, 0x42, 0x42, 0x42, 0x3C, 0x00, 0x20, 0x10, 0x00, 0x42,
	0x42, 0x42, 0x3C, 0x00, 0x00, 0x42, 0x00, 0x42, 0x42, 0x3E, 0x02, 0x3C,
	0x42, 0x18, 0x24, 0x42, 0x42, 0x24, 0x18, 0x00, 0x42, 0x00, 0x42, 0x42,
	0x42, 0x42, 0x3C, 0x00, 0x08, 0x08, 0x3E, 0x40, 0x40, 0x3E, 0x08, 0x08,
	0x18, 0x24, 0x20, 0x70, 0x20, 0x42, 0x7C, 0x00, 0x44, 0x28, 0x7C, 0x10,
	0x7C, 0x10, 0x10, 0x00, 0xF8, 0x4C, 0x78, 0x44, 0x4F, 0x44, 0x45, 0xE6,
	0x1C, 0x12, 0x10, 0x7C, 0x10, 0x10, 0x90, 0x60, 0x0C, 0x00, 0x38, 0x04,
	0x3C, 0x44, 0x3E, 0x00, 0x0C, 0x00, 0x18, 0x08, 0x08, 0x08, 0x1C, 0x00,
	0x04, 0x08, 0x00, 0x3C, 0x42, 0x42, 0x3C, 0x00, 0x00, 0x04, 0x08, 0x42,
	0x42, 0x42, 0x3C, 0x00, 0x32, 0x4C, 0x00, 0x7C, 0x42, 0x42, 0x42, 0x00,
	0x34, 0x4C, 0x00, 0x62, 0x52, 0x4A, 0x46, 0x00, 0x3C, 0x44, 0x44, 0x3E,
	0x00, 0x7E, 0x00, 0x00, 0x38, 0x44, 0x44, 0x38, 0x00, 0x7C, 0x00, 0x00,
	0x10, 0x00, 0x10, 0x20, 0x40, 0x42, 0x3C, 0x00, 0x00, 0x00, 0x00, 0x7E,
	0x40, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7E, 0x02, 0x02, 0x00, 0x00,
	0x42, 0xC4, 0x48, 0xF6, 0x29, 0x43, 0x8C, 0x1F, 0x42, 0xC4, 0x4A, 0xF6,
	0x2A, 0x5F, 0x82, 0x02, 0x00, 0x10, 0x00, 0x10, 0x10, 0x10, 0x10, 0x00,
	0x00, 0x12, 0x24, 0x48, 0x24, 0x12, 0x00, 0x00, 0x00, 0x48, 0x24, 0x12,
	0x24, 0x48, 0x00, 0x00, 0x22, 0x88, 0x22, 0x88, 0x22, 0x88, 0x22, 0x88,
	0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0xDB, 0x77, 0xDB, 0xEE,
	0xDB, 0x77, 0xDB, 0xEE, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10,
	0x10, 0x10, 0x10, 0x10, 0xF0, 0x10, 0x10, 0x10, 0x10, 0x10, 0xF0, 0x10,
	0xF0, 0x10, 0x10, 0x10, 0x14, 0x14, 0x14, 0x14, 0xF4, 0x14, 0x14, 0x14,
	0x00, 0x00, 0x00, 0x00, 0xFC, 0x14, 0x14, 0x14, 0x00, 0x00, 0xF0, 0x10,
	0xF0, 0x10, 0x10, 0x10, 0x14, 0x14, 0xF4, 0x04, 0xF4, 0x14, 0x14, 0x14,
	0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x00, 0x00, 0xFC, 0x04,
	0xF4, 0x14, 0x14, 0x14, 0x14, 0x14, 0xF4, 0x04, 0xFC, 0x00, 0x00, 0x00,
	0x14, 0x14, 0x14, 0x14, 0xFC, 0x00, 0x00, 0x00, 0x10, 0x10, 0xF0, 0x10,
	0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF0, 0x10, 0x10, 0x10,
	0x10, 0x10, 0x10, 0x10, 0x1F, 0x00, 0x00, 0x00, 0x10, 0x10, 0x10, 0x10,
	0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x10, 0x10, 0x10,
	0x10, 0x10, 0x10, 0x10, 0x1F, 0x10, 0x10, 0x10, 0x00, 0x00, 0x00, 0x00,
	0xFF, 0x00, 0x00, 0x00, 0x10, 0x10, 0x10, 0x10, 0xFF, 0x10, 0x10, 0x10,
	0x10, 0x10, 0x1F, 0x10, 0x1F, 0x10, 0x10, 0x10, 0x14, 0x14, 0x14, 0x14,
	0x17, 0x14, 0x14, 0x14, 0x14, 0x14, 0x17, 0x10, 0x1F, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x1F, 0x10, 0x17, 0x14, 0x14, 0x14, 0x14, 0x14, 0xF7, 0x00,
	0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0xF7, 0x14, 0x14, 0x14,
	0x14, 0x14, 0x17, 0x10, 0x17, 0x14, 0x14, 0x14, 0x00, 0x00, 0xFF, 0x00,
	0xFF, 0x00, 0x00, 0x00, 0x14, 0x14, 0xF7, 0x00, 0xF7, 0x14, 0x14, 0x14,
	0x10, 0x10, 0xFF, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x14, 0x14, 0x14, 0x14,
	0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0xFF, 0x10, 0x10, 0x10,
	0x00, 0x00, 0x00, 0x00, 0xFF, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14,
	0x1F, 0x00, 0x00, 0x00, 0x10, 0x10, 0x1F, 0x10, 0x1F, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x1F, 0x10, 0x1F, 0x10, 0x10, 0x10, 0x00, 0x00, 0x00, 0x00,
	0x1F, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0xFF, 0x14, 0x14, 0x14,
	0x10, 0x10, 0xFF, 0x10, 0xFF, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10,
	0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1F, 0x10, 0x10, 0x10,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00,
	0xFF, 0xFF, 0xFF, 0xFF, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0,
	0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0xFF, 0xFF, 0xFF, 0xFF,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x31, 0x4A, 0x44, 0x4A, 0x31, 0x00,
	0x00, 0x3C, 0x42, 0x7C, 0x42, 0x7C, 0x40, 0x40, 0x00, 0x7E, 0x42, 0x40,
	0x40, 0x40, 0x40, 0x00, 0x00, 0x3F, 0x54, 0x14, 0x14, 0x14, 0x14, 0x00,
	0x7E, 0x42, 0x20, 0x18, 0x20, 0x42, 0x7E, 0x00, 0x00, 0x00, 0x3E, 0x48,
	0x48, 0x48, 0x30, 0x00, 0x00, 0x44, 0x44, 0x44, 0x7A, 0x40, 0x40, 0x80,
	0x00, 0x33, 0x4C, 0x08, 0x08, 0x08, 0x08, 0x00, 0x7C, 0x10, 0x38, 0x44,
	0x44, 0x38, 0x10, 0x7C, 0x18, 0x24, 0x42, 0x7E, 0x42, 0x24, 0x18, 0x00,
	0x18, 0x24, 0x42, 0x42, 0x24, 0x24, 0x66, 0x00, 0x1C, 0x20, 0x18, 0x3C,
	0x42, 0x42, 0x3C, 0x00, 0x00, 0x62, 0x95, 0x89, 0x95, 0x62, 0x00, 0x00,
	0x02, 0x04, 0x3C, 0x4A, 0x52, 0x3C, 0x40, 0x80, 0x0C, 0x10, 0x20, 0x3C,
	0x20, 0x10, 0x0C, 0x00, 0x3C, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x00,
	0x00, 0x7E, 0x00, 0x7E, 0x00, 0x7E, 0x00, 0x00, 0x10, 0x10, 0x7C, 0x10,
	0x10, 0x00, 0x7C, 0x00, 0x10, 0x08, 0x04, 0x08, 0x10, 0x00, 0x7E, 0x00,
	0x08, 0x10, 0x20, 0x10, 0x08, 0x00, 0x7E, 0x00, 0x0C, 0x12, 0x12, 0x10,
	0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x90, 0x90, 0x60,
	0x18, 0x18, 0x00, 0x7E, 0x00, 0x18, 0x18, 0x00, 0x00, 0x32, 0x4C, 0x00,
	0x32, 0x4C, 0x00, 0x00, 0x30, 0x48, 0x48, 0x30, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x18, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x18, 0x00, 0x00, 0x00, 0x0F, 0x08, 0x08, 0x08, 0x08, 0xC8, 0x28, 0x18,
	0x78, 0x44, 0x44, 0x44, 0x44, 0x00, 0x00, 0x00, 0x30, 0x48, 0x10, 0x20,
	0x78, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3C, 0x3C, 0x3C, 0x3C, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x7E, 0x81, 0xA5, 0x81, 0xBD, 0x99, 0x81, 0x7E,
	0x7E, 0xFF, 0xDB, 0xFF, 0xC3, 0xE7, 0xFF, 0x7E, 0x6C, 0xFE, 0xFE, 0xFE,
	0x7C, 0x38, 0x10, 0x00, 0x10, 0x38, 0x7C, 0xFE, 0x7C, 0x38, 0x10, 0x00,
	0x38, 0x7C, 0x38, 0xFE, 0xFE, 0xD6, 0x10, 0x38, 0x10, 0x10, 0x38, 0x7C,
	0xFE, 0x7C, 0x10, 0x38, 0x00, 0x00, 0x18, 0x3C, 0x3C, 0x18, 0x00, 0x00,
	0xFF, 0xFF, 0xE7, 0xC3, 0xC3, 0xE7, 0xFF, 0xFF, 0x00, 0x3C, 0x66, 0x42,
	0x42, 0x66, 0x3C, 0x00, 0xFF, 0xC3, 0x99, 0xBD, 0xBD, 0x99, 0xC3, 0xFF,
	0x0F, 0x07, 0x0F, 0x7D, 0xCC, 0xCC, 0xCC, 0x78, 0x3C, 0x66, 0x66, 0x66,
	0x3C, 0x18, 0x7E, 0x18, 0x3F, 0x33, 0x3F, 0x30, 0x30, 0x70, 0xF0, 0xE0,
	0x7F, 0x63, 0x7F, 0x63, 0x63, 0x67, 0xE6, 0xC0, 0x18, 0xDB, 0x3C, 0xE7,
	0xE7, 0x3C, 0xDB, 0x18, 0x80, 0xE0, 0xF8, 0xFE, 0xF8, 0xE0, 0x80, 0x00,
	0x02, 0x0E, 0x3E, 0xFE, 0x3E, 0x0E, 0x02, 0x00, 0x18, 0x3C, 0x7E, 0x18,
	0x18, 0x7E, 0x3C, 0x18, 0x66, 0x66, 0x66, 0x66, 0x66, 0x00, 0x66, 0x00,
	0x7F, 0xDB, 0xDB, 0x7B, 0x1B, 0x1B, 0x1B, 0x00, 0x3E, 0x63, 0x38, 0x6C,
	0x6C, 0x38, 0xCC, 0x78, 0x00, 0x00, 0x00, 0x00, 0x7E, 0x7E, 0x7E, 0x00,
	0x18, 0x3C, 0x7E, 0x18, 0x7E, 0x3C, 0x18, 0xFF, 0x18, 0x3C, 0x7E, 0x18,
	0x18, 0x18, 0x18, 0x00, 0x18, 0x18, 0x18, 0x18, 0x7E, 0x3C, 0x18, 0x00,
	0x00, 0x18, 0x0C, 0xFE, 0x0C, 0x18, 0x00, 0x00, 0x00, 0x30, 0x60, 0xFE,
	0x60, 0x30, 0x00, 0x00, 0x00, 0x00, 0xC0, 0xC0, 0xC0, 0xFE, 0x00, 0x00,
	0x00, 0x24, 0x66, 0xFF, 0x66, 0x24, 0x00, 0x00, 0x00, 0x18, 0x3C, 0x7E,
	0xFF, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x7E, 0x3C, 0x18, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x78, 0x78, 0x30,
	0x30, 0x00, 0x30, 0x00, 0x6C, 0x6C, 0x6C, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x6C, 0x6C, 0xFE, 0x6C, 0xFE, 0x6C, 0x6C, 0x00, 0x30, 0x7C, 0xC0, 0x78,
	0x0C, 0xF8, 0x30, 0x00, 0x00, 0xC6, 0xCC, 0x18, 0x30, 0x66, 0xC6, 0x00,
	0x38, 0x6C, 0x38, 0x76, 0xDC, 0xCC, 0x76, 0x00, 0x60, 0x60, 0xC0, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x18, 0x30, 0x60, 0x60, 0x60, 0x30, 0x18, 0x00,
	0x60, 0x30, 0x18, 0x18, 0x18, 0x30, 0x60, 0x00, 0x00, 0x66, 0x3C, 0xFF,
	0x3C, 0x66, 0x00, 0x00, 0x00, 0x30, 0x30, 0xFC, 0x30, 0x30, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x30, 0x60, 0x00, 0x00, 0x00, 0xFC,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x30, 0x00,
	0x06, 0x0C, 0x18, 0x30, 0x60, 0xC0, 0x80, 0x00, 0x7C, 0xC6, 0xCE, 0xDE,
	0xF6, 0xE6, 0x7C, 0x00, 0x30, 0x70, 0x30, 0x30, 0x30, 0x30, 0xFC, 0x00,
	0x78, 0xCC, 0x0C, 0x38, 0x60, 0xCC, 0xFC, 0x00, 0x78, 0xCC, 0x0C, 0x38,
	0x0C, 0xCC, 0x78, 0x00, 0x1C, 0x3C, 0x6C, 0xCC, 0xFE, 0x0C, 0x1E, 0x00,
	0xFC, 0xC0, 0xF8, 0x0C, 0x0C, 0xCC, 0x78, 0x00, 0x38, 0x60, 0xC0, 0xF8,
	0xCC, 0xCC, 0x78, 0x00, 0xFC, 0xCC, 0x0C, 0x18, 0x30, 0x30, 0x30, 0x00,
	0x78, 0xCC, 0xCC, 0x78, 0xCC, 0xCC, 0x78, 0x00, 0x78, 0xCC, 0xCC, 0x7C,
	0x0C, 0x18, 0x70, 0x00, 0x00, 0x30, 0x30, 0x00, 0x00, 0x30, 0x30, 0x00,
	0x00, 0x30, 0x30, 0x00, 0x00, 0x30, 0x30, 0x60, 0x18, 0x30, 0x60, 0xC0,
	0x60, 0x30, 0x18, 0x00, 0x00, 0x00, 0xFC, 0x00, 0x00, 0xFC, 0x00, 0x00,
	0x60, 0x30, 0x18, 0x0C, 0x18, 0x30, 0x60, 0x00, 0x78, 0xCC, 0x0C, 0x18,
	0x30, 0x00, 0x30, 0x00, 0x7C, 0xC6, 0xDE, 0xDE, 0xDE, 0xC0, 0x78, 0x00,
	0x30, 0x78, 0xCC, 0xCC, 0xFC, 0xCC, 0xCC, 0x00, 0xFC, 0x66, 0x66, 0x7C,
	0x66, 0x66, 0xFC, 0x00, 0x3C, 0x66, 0xC0, 0xC0, 0xC0, 0x66, 0x3C, 0x00,
	0xF8, 0x6C, 0x66, 0x66, 0x66, 0x6C, 0xF8, 0x00, 0xFE, 0x62, 0x68, 0x78,
	0x68, 0x62, 0xFE, 0x00, 0xFE, 0x62, 0x68, 0x78, 0x68, 0x60, 0xF0, 0x00,
	0x3C, 0x66, 0xC0, 0xC0, 0xCE, 0x66, 0x3E, 0x00, 0xCC, 0xCC, 0xCC, 0xFC,
	0xCC, 0xCC, 0xCC, 0x00, 0x78, 0x30, 0x30, 0x30, 0x30, 0x30, 0x78, 0x00,
	0x1E, 0x0C, 0x0C, 0x0C, 0xCC, 0xCC, 0x78, 0x00, 0xE6, 0x66, 0x6C, 0x78,
	0x6C, 0x66, 0xE6, 0x00, 0xF0, 0x60, 0x60, 0x60, 0x62, 0x66, 0xFE, 0x00,
	0xC6, 0xEE, 0xFE, 0xFE, 0xD6, 0xC6, 0xC6, 0x00, 0xC6, 0xE6, 0xF6, 0xDE,
	0xCE, 0xC6, 0xC6, 0x00, 0x38, 0x6C, 0xC6, 0xC6, 0xC6, 0x6C, 0x38, 0x00,
	0xFC, 0x66, 0x66, 0x7C, 0x60, 0x60, 0xF0, 0x00, 0x78, 0xCC, 0xCC, 0xCC,
	0xDC, 0x78, 0x1C, 0x00, 0xFC, 0x66, 0x66, 0x7C, 0x6C, 0x66, 0xE6, 0x00,
	0x78, 0xCC, 0x60, 0x30, 0x18, 0xCC, 0x78, 0x00, 0xFC, 0xB4, 0x30, 0x30,
	0x30, 0x30, 0x78, 0x00, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xFC, 0x00,
	0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0x78, 0x30, 0x00, 0xC6, 0xC6, 0xC6, 0xD6,
	0xFE, 0xEE, 0xC6, 0x00, 0xC6, 0xC6, 0x6C, 0x38, 0x38, 0x6C, 0xC6, 0x00,
	0xCC, 0xCC, 0xCC, 0x78, 0x30, 0x30, 0x78, 0x00, 0xFE, 0xC6, 0x8C, 0x18,
	0x32, 0x66, 0xFE, 0x00, 0x78, 0x60, 0x60, 0x60, 0x60, 0x60, 0x78, 0x00,
	0xC0, 0x60, 0x30, 0x18, 0x0C, 0x06, 0x02, 0x00, 0x78, 0x18, 0x18, 0x18,
	0x18, 0x18, 0x78, 0x00, 0x10, 0x38, 0x6C, 0xC6, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x30, 0x30, 0x18, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x78, 0x0C, 0x7C, 0xCC, 0x76, 0x00,
	0xE0, 0x60, 0x60, 0x7C, 0x66, 0x66, 0xDC, 0x00, 0x00, 0x00, 0x78, 0xCC,
	0xC0, 0xCC, 0x78, 0x00, 0x1C, 0x0C, 0x0C, 0x7C, 0xCC, 0xCC, 0x76, 0x00,
	0x00, 0x00, 0x78, 0xCC, 0xFC, 0xC0, 0x78, 0x00, 0x38, 0x6C, 0x60, 0xF0,
	0x60, 0x60, 0xF0, 0x00, 0x00, 0x00, 0x76, 0xCC, 0xCC, 0x7C, 0x0C, 0xF8,
	0xE0, 0x60, 0x6C, 0x76, 0x66, 0x66, 0xE6, 0x00, 0x30, 0x00, 0x70, 0x30,
	0x30, 0x30, 0x78, 0x00, 0x0C, 0x00, 0x0C, 0x0C, 0x0C, 0xCC, 0xCC, 0x78,
	0xE0, 0x60, 0x66, 0x6C, 0x78, 0x6C, 0xE6, 0x00, 0x70, 0x30, 0x30, 0x30,
	0x30, 0x30, 0x78, 0x00, 0x00, 0x00, 0xCC, 0xFE, 0xFE, 0xD6, 0xC6, 0x00,
	0x00, 0x00, 0xF8, 0xCC, 0xCC, 0xCC, 0xCC, 0x00, 0x00, 0x00, 0x78, 0xCC,
	0xCC, 0xCC, 0x78, 0x00, 0x00, 0x00, 0xDC, 0x66, 0x66, 0x7C, 0x60, 0xF0,
	0x00, 0x00, 0x76, 0xCC, 0xCC, 0x7C, 0x0C, 0x1E, 0x00, 0x00, 0xDC, 0x76,
	0x66, 0x60, 0xF0, 0x00, 0x00, 0x00, 0x7C, 0xC0, 0x78, 0x0C, 0xF8, 0x00,
	0x10, 0x30, 0x7C, 0x30, 0x30, 0x34, 0x18, 0x00, 0x00, 0x00, 0xCC, 0xCC,
	0xCC, 0xCC, 0x76, 0x00, 0x00, 0x00, 0xCC, 0xCC, 0xCC, 0x78, 0x30, 0x00,
	0x00, 0x00, 0xC6, 0xD6, 0xFE, 0xFE, 0x6C, 0x00, 0x00, 0x00, 0xC6, 0x6C,
	0x38, 0x6C, 0xC6, 0x00, 0x00, 0x00, 0xCC, 0xCC, 0xCC, 0x7C, 0x0C, 0xF8,
	0x00, 0x00, 0xFC, 0x98, 0x30, 0x64, 0xFC, 0x00, 0x1C, 0x30, 0x30, 0xE0,
	0x30, 0x30, 0x1C, 0x00, 0x18, 0x18, 0x18, 0x00, 0x18, 0x18, 0x18, 0x00,
	0xE0, 0x30, 0x30, 0x1C, 0x30, 0x30, 0xE0, 0x00, 0x76, 0xDC, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x38, 0x6C, 0xC6, 0xC6, 0xFE, 0x00,
	0x78, 0xCC, 0xC0, 0xCC, 0x78, 0x18, 0x0C, 0x78, 0x00, 0xCC, 0x00, 0xCC,
	0xCC, 0xCC, 0x7E, 0x00, 0x1C, 0x00, 0x78, 0xCC, 0xFC, 0xC0, 0x78, 0x00,
	0x7E, 0xC3, 0x3C, 0x06, 0x3E, 0x66, 0x3F, 0x00, 0xCC, 0x00, 0x78, 0x0C,
	0x7C, 0xCC, 0x7E, 0x00, 0xE0, 0x00, 0x78, 0x0C, 0x7C, 0xCC, 0x7E, 0x00,
	0x30, 0x30, 0x78, 0x0C, 0x7C, 0xCC, 0x7E, 0x00, 0x00, 0x00, 0x78, 0xC0,
	0xC0, 0x78, 0x0C, 0x38, 0x7E, 0xC3, 0x3C, 0x66, 0x7E, 0x60, 0x3C, 0x00,
	0xCC, 0x00, 0x78, 0xCC, 0xFC, 0xC0, 0x78, 0x00, 0xE0, 0x00, 0x78, 0xCC,
	0xFC, 0xC0, 0x78, 0x00, 0xCC, 0x00, 0x70, 0x30, 0x30, 0x30, 0x78, 0x00,
	0x7C, 0xC6, 0x38, 0x18, 0x18, 0x18, 0x3C, 0x00, 0xE0, 0x00, 0x70, 0x30,
	0x30, 0x30, 0x78, 0x00, 0xC6, 0x38, 0x6C, 0xC6, 0xFE, 0xC6, 0xC6, 0x00,
	0x30, 0x30, 0x00, 0x78, 0xCC, 0xFC, 0xCC, 0x00, 0x1C, 0x00, 0xFC, 0x60,
	0x78, 0x60, 0xFC, 0x00, 0x00, 0x00, 0x7F, 0x0C, 0x7F, 0xCC, 0x7F, 0x00,
	0x3E, 0x6C, 0xCC, 0xFE, 0xCC, 0xCC, 0xCE, 0x00, 0x78, 0xCC, 0x00, 0x78,
	0xCC, 0xCC, 0x78, 0x00, 0x00, 0xCC, 0x00, 0x78, 0xCC, 0xCC, 0x78, 0x00,
	0x00, 0xE0, 0x00, 0x78, 0xCC, 0xCC, 0x78, 0x00, 0x78, 0xCC, 0x00, 0xCC,
	0xCC, 0xCC, 0x7E, 0x00, 0x00, 0xE0, 0x00, 0xCC, 0xCC, 0xCC, 0x7E, 0x00,
	0x00, 0xCC, 0x00, 0xCC, 0xCC, 0x7C, 0x0C, 0xF8, 0xC3, 0x18, 0x3C, 0x66,
	0x66, 0x3C, 0x18, 0x00, 0xCC, 0x00, 0xCC, 0xCC, 0xCC, 0xCC, 0x78, 0x00,
	0x18, 0x18, 0x7E, 0xC0, 0xC0, 0x7E, 0x18, 0x18, 0x38, 0x6C, 0x64, 0xF0,
	0x60, 0xE6, 0xFC, 0x00, 0xCC, 0xCC, 0x78, 0xFC, 0x30, 0xFC, 0x30, 0x30,
	0xF8, 0xCC, 0xCC, 0xFA, 0xC6, 0xCF, 0xC6, 0xC7, 0x0E, 0x1B, 0x18, 0x3C,
	0x18, 0x18, 0xD8, 0x70, 0x1C, 0x00, 0x78, 0x0C, 0x7C, 0xCC, 0x7E, 0x00,
	0x38, 0x00, 0x70, 0x30, 0x30, 0x30, 0x78, 0x00, 0x00, 0x1C, 0x00, 0x78,
	0xCC, 0xCC, 0x78, 0x00, 0x00, 0x1C, 0x00, 0xCC, 0xCC, 0xCC, 0x7E, 0x00,
	0x00, 0xF8, 0x00, 0xF8, 0xCC, 0xCC, 0xCC, 0x00, 0xFC, 0x00, 0xCC, 0xEC,
	0xFC, 0xDC, 0xCC, 0x00, 0x3C, 0x6C, 0x6C, 0x3E, 0x00, 0x7E, 0x00, 0x00,
	0x38, 0x6C, 0x6C, 0x38, 0x00, 0x7C, 0x00, 0x00, 0x30, 0x00, 0x30, 0x60,
	0xC0, 0xCC, 0x78, 0x00, 0x00, 0x00, 0x00, 0xFC, 0xC0, 0xC0, 0x00, 0x00,
	0x00, 0x00, 0x00, 0xFC, 0x0C, 0x0C, 0x00, 0x00, 0xC3, 0xC6, 0xCC, 0xDE,
	0x33, 0x66, 0xCC, 0x0F, 0xC3, 0xC6, 0xCC, 0xDB, 0x37, 0x6F, 0xCF, 0x03,
	0x18, 0x18, 0x00, 0x18, 0x18, 0x18, 0x18, 0x00, 0x00, 0x33, 0x66, 0xCC,
	0x66, 0x33, 0x00, 0x00, 0x00, 0xCC, 0x66, 0x33, 0x66, 0xCC, 0x00, 0x00,
	0x22, 0x88, 0x22, 0x88, 0x22, 0x88, 0x22, 0x88, 0x55, 0xAA, 0x55, 0xAA,
	0x55, 0xAA, 0x55, 0xAA, 0xDB, 0x77, 0xDB, 0xEE, 0xDB, 0x77, 0xDB, 0xEE,
	0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18,
	0xF8, 0x18, 0x18, 0x18, 0x18, 0x18, 0xF8, 0x18, 0xF8, 0x18, 0x18, 0x18,
	0x36, 0x36, 0x36, 0x36, 0xF6, 0x36, 0x36, 0x36, 0x00, 0x00, 0x00, 0x00,
	0xFE, 0x36, 0x36, 0x36, 0x00, 0x00, 0xF8, 0x18, 0xF8, 0x18, 0x18, 0x18,
	0x36, 0x36, 0xF6, 0x06, 0xF6, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36,
	0x36, 0x36, 0x36, 0x36, 0x00, 0x00, 0xFE, 0x06, 0xF6, 0x36, 0x36, 0x36,
	0x36, 0x36, 0xF6, 0x06, 0xFE, 0x00, 0x00, 0x00, 0x36, 0x36, 0x36, 0x36,
	0xFE, 0x00, 0x00, 0x00, 0x18, 0x18, 0xF8, 0x18, 0xF8, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0xF8, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18,
	0x1F, 0x00, 0x00, 0x00, 0x18, 0x18, 0x18, 0x18, 0xFF, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0xFF, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18,
	0x1F, 0x18, 0x18, 0x18, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00,
	0x18, 0x18, 0x18, 0x18, 0xFF, 0x18, 0x18, 0x18, 0x18, 0x18, 0x1F, 0x18,
	0x1F, 0x18, 0x18, 0x18, 0x36, 0x36, 0x36, 0x36, 0x37, 0x36, 0x36, 0x36,
	0x36, 0x36, 0x37, 0x30, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3F, 0x30,
	0x37, 0x36, 0x36, 0x36, 0x36, 0x36, 0xF7, 0x00, 0xFF, 0x00, 0x00, 0x00,
	0x00, 0x00, 0xFF, 0x00, 0xF7, 0x36, 0x36, 0x36, 0x36, 0x36, 0x37, 0x30,
	0x37, 0x36, 0x36, 0x36, 0x00, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0x00, 0x00,
	0x36, 0x36, 0xF7, 0x00, 0xF7, 0x36, 0x36, 0x36, 0x18, 0x18, 0xFF, 0x00,
	0xFF, 0x00, 0x00, 0x00, 0x36, 0x36, 0x36, 0x36, 0xFF, 0x00, 0x00, 0x00,
	0x00, 0x00, 0xFF, 0x00, 0xFF, 0x18, 0x18, 0x18, 0x00, 0x00, 0x00, 0x00,
	0xFF, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x3F, 0x00, 0x00, 0x00,
	0x18, 0x18, 0x1F, 0x18, 0x1F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1F, 0x18,
	0x1F, 0x18, 0x18, 0x18, 0x00, 0x00, 0x00, 0x00, 0x3F, 0x36, 0x36, 0x36,
	0x36, 0x36, 0x36, 0x36, 0xFF, 0x36, 0x36, 0x36, 0x18, 0x18, 0xFF, 0x18,
	0xFF, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0xF8, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x1F, 0x18, 0x18, 0x18, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF,
	0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0x0F, 0x0F, 0x0F, 0x0F,
	0x0F, 0x0F, 0x0F, 0x0F, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x76, 0xDC, 0xC8, 0xDC, 0x76, 0x00, 0x00, 0x78, 0xCC, 0xF8,
	0xCC, 0xF8, 0xC0, 0xC0, 0x00, 0xFC, 0xCC, 0xC0, 0xC0, 0xC0, 0xC0, 0x00,
	0x00, 0xFE, 0x6C, 0x6C, 0x6C, 0x6C, 0x6C, 0x00, 0xFC, 0xCC, 0x60, 0x30,
	0x60, 0xCC, 0xFC, 0x00, 0x00, 0x00, 0x7E, 0xD8, 0xD8, 0xD8, 0x70, 0x00,
	0x00, 0x66, 0x66, 0x66, 0x66, 0x7C, 0x60, 0xC0, 0x00, 0x76, 0xDC, 0x18,
	0x18, 0x18, 0x18, 0x00, 0xFC, 0x30, 0x78, 0xCC, 0xCC, 0x78, 0x30, 0xFC,
	0x38, 0x6C, 0xC6, 0xFE, 0xC6, 0x6C, 0x38, 0x00, 0x38, 0x6C, 0xC6, 0xC6,
	0x6C, 0x6C, 0xEE, 0x00, 0x1C, 0x30, 0x18, 0x7C, 0xCC, 0xCC, 0x78, 0x00,
	0x00, 0x00, 0x7E, 0xDB, 0xDB, 0x7E, 0x00, 0x00, 0x06, 0x0C, 0x7E, 0xDB,
	0xDB, 0x7E, 0x60, 0xC0, 0x38, 0x60, 0xC0, 0xF8, 0xC0, 0x60, 0x38, 0x00,
	0x78, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0x00, 0x00, 0xFC, 0x00, 0xFC,
	0x00, 0xFC, 0x00, 0x00, 0x30, 0x30, 0xFC, 0x30, 0x30, 0x00, 0xFC, 0x00,
	0x60, 0x30, 0x18, 0x30, 0x60, 0x00, 0xFC, 0x00, 0x18, 0x30, 0x60, 0x30,
	0x18, 0x00, 0xFC, 0x00, 0x0E, 0x1B, 0x1B, 0x18, 0x18, 0x18, 0x18, 0x18,
	0x18, 0x18, 0x18, 0x18, 0x18, 0xD8, 0xD8, 0x70, 0x30, 0x30, 0x00, 0xFC,
	0x00, 0x30, 0x30, 0x00, 0x00, 0x76, 0xDC, 0x00, 0x76, 0xDC, 0x00, 0x00,
	0x38, 0x6C, 0x6C, 0x38, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x18,
	0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x00, 0x00, 0x00,
	0x0F, 0x0C, 0x0C, 0x0C, 0xEC, 0x6C, 0x3C, 0x1C, 0x78, 0x6C, 0x6C, 0x6C,
	0x6C, 0x00, 0x00, 0x00, 0x70, 0x18, 0x30, 0x60, 0x78, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x3C, 0x3C, 0x3C, 0x3C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
};

static unsigned char reg[256];
static unsigned char mem[0x4000];
static char pal[4] = {0};
static signed long curaddr=0;
static unsigned int datasize=0x4000;
static int width,height;

#define htotal reg['0']
#define hdisp reg['1']
#define hsyncp reg['2']
#define hsyncw reg['3']
#define vtotal reg['4']
#define vadjust reg['5']
#define vdisp reg['6']
#define vsyncp reg['7']
#define interlace reg['8']
#define maxscan reg['9']
#define curstart reg['A']
#define curend reg['B']
#define starth reg['C']
#define startl reg['D']
#define cursorh reg['E']
#define cursorl reg['F']
#define modectrl reg['X']
#define colorsel reg['Y']
#define extractrl reg['Z']

/*
	extractrl bits:
		0x01 = BSAVE header
		0x02 = Do not apply brown hack
		0x04 = Use thin font
		0x08 = Conceal blinking text
		0x10 = Transparent cursor
		0x20 = Active cursor
		0x40 = (reserved)
		0x80 = Mode header
*/

static void
usage(void)
{
	fprintf(stderr,
		"Usage: cga2ff [0-F] [X] [Y] [Z]                                               \n"
		"                                                                              \n"
		"Convert IBM PC Color Graphics Adapter (CGA) video memory dump to farbfeld.    \n"
		"cga2ff reads raw CGA data from stdin, converts it to farbfeld and writes the  \n"
		"result to stdout.                                                             \n"
		"                                                                              \n"
		"Use of this tool assumes intimate familiarity with the CGA graphics format and\n"
		"hardware.                                                                     \n"
		"                                                                              \n"
		"Arguments are all in hex, each three characters long.                         \n"
		"                                                                              \n"
		"   0-F     for one of the 6845 registers                                      \n"
		"   X       the CGA \"mode control\" register                                  \n"
		"   Y       the CGA \"color select\" register                                  \n"
		"   Z       additional options                                                 \n"
		"                                                                              \n"
		"Refer to the man page for more details on the above arguments.                \n"
		"                                                                              \n"
		"Example usage:                                                                \n"
		"   $ cga2ff F35 X33 Y14 Z2E < memory.dump > image.ff                          \n"
		"\n"
	);
	exit(1);
}

static inline void
make_header(void)
{
	int i;
	fwrite("farbfeld",1,8,stdout);
	width=i=(htotal+1-hsyncw)<<(modectrl&1?3:4);
	putchar(i>>24); putchar(i>>16); putchar(i>>8); putchar(i);
	height=i=(vtotal+1)*(maxscan+1)+vadjust-16;
	putchar(i>>24); putchar(i>>16); putchar(i>>8); putchar(i);
}

static void
out_pixel(char v, char c)
{
	int i;
	putchar(i=(v&4?0xAA:0)|(v&8?0x55:0)); putchar(i);
	putchar(i=v==6&&!(extractrl&2)?0x55:(v&2?0xAA:0)|(v&8?0x55:0)); putchar(i);
	putchar(i=(v&1?0xAA:0)|(v&8?0x55:0)); putchar(i);
	putchar((extractrl&16)&&c?0:255); putchar(i);
}

static inline void
display_tile(int addr, int row, int curs)
{
	int a,c,i,p,v,x;
	if (modectrl & 2) addr=(addr&0xFFF)|((row&1)<<12); else addr &= 0x1FFF;
	c=mem[addr<<1]; a=mem[(addr<<1)|1];
	p=rom[row|(c<<3)|(extractrl&4?0:0x800)];
	for (x = 0; x < 8; x++) {
		if (modectrl & 2) {
			i=(x&4?a:c)>>((3&~x)<<1);
			v=pal[i&3];
		} else {
			i=(p<<x)&128;
			if ((extractrl & 8) && (a & 128) && (modectrl & 32)) i=0;
			if (modectrl & 32) a &= 127;
			if (curs && (extractrl & 32)) i=128;
			v=(a>>(i?0:4))&15;
			i=(x&4?a:c)>>((3&~x)<<1);
		}
		if (modectrl & 16) {
			out_pixel(i&2?v:0,curs);
			if (!(i&1)) v=0;
		} else {
			out_pixel(v,curs);
		}
		if (!(modectrl & 1)) out_pixel(v,curs);
	}
}

static inline void
make_picture(void)
{
	int b=modectrl&16?0:*pal;
	int x,y,z;
	// Top border
	y=(vtotal-vsyncp)*(maxscan+1)+vadjust-16;
	while (y--) for (x = 0; x < width; x++) out_pixel(b,0);
	// Display area
	for (y = 0; y < vdisp; y++) {
		for (z = 0; z <= maxscan; z++) {
			// Left border
			x=(htotal+1-hsyncp-hsyncw)<<(modectrl&1?3:4);
			while (x--) out_pixel(b,0);
			// Display area
			for (x = 0; x < hdisp; x++) display_tile(curaddr+x,z&7,curaddr+x==(cursorh<<8)+cursorl && z>=curstart && z<=curend);
			// Right border
			x=(hsyncp-hdisp)<<(modectrl&1?3:4);
			while (x--) out_pixel(b,0);
		}
		curaddr+=hdisp;
	}
	// Bottom border
	y=(vsyncp-vdisp)*(maxscan+1);
	while (y--) for (x = 0; x < width; x++) out_pixel(b,0);
}

int
main(int argc, char **argv)
{
	int i;

	for (i = 1; i < argc; i++) {
		if (!strcmp(argv[i],"-h") || !strcmp(argv[i], "--help")) {
			usage();
		}

		if (argv[i][0]) {
			reg[argv[i][0]&255]=strtol(argv[i]+1,0,16);
		}
	}
	for (i = 0; i < 26; i++) reg[i+'A']|=reg[i+'a'];
	if (extractrl & 128) {
		reg['X']=fgetc(stdin);
		reg['Y']=fgetc(stdin);
		for (i = '0'; i <= '9'; i++) reg[i]=fgetc(stdin);
		for (i = 'A'; i <= 'F'; i++) reg[i]=fgetc(stdin);
	}
	if (extractrl & 1) {
		// BSAVE
		if (fgetc(stdin) != 0xFD) fatal("Not a BSAVE file");
		curaddr=fgetc(stdin)<<4; curaddr|=fgetc(stdin)<<12;
		curaddr+=fgetc(stdin)-0xB8000; curaddr+=fgetc(stdin)<<8;
		datasize=fgetc(stdin); datasize|=fgetc(stdin)<<8;
	}
	while (datasize-- && curaddr < 0x8000) {
		i=fgetc(stdin);
		if (i == EOF) break;
		if (curaddr >= 0) mem[curaddr&0x3FFF]=i;
		curaddr++;
	}
	hsyncw&=0x0F;
	vtotal&=0x7F;
	vadjust&=0x1F;
	vdisp&=0x7F;
	vsyncp&=0x7F;
	interlace&=0x03;
	maxscan&=0x1F;
	curstart&=0x3F;
	curend&=0x1F;
	starth&=0x3F;
	cursorh&=0x3F;
	make_header();
	curaddr=(starth<<8)|startl;
	pal[0]=colorsel & 15;
	for (i = 1; i < 4; i++) {
		pal[i] = modectrl & 16 ? *pal : (i<<1) | (modectrl & 4 ? i & 1 : (colorsel>>5) & 1) | (colorsel & 16 ? 8 : 0);
	}
	make_picture();
	return 0;
}
