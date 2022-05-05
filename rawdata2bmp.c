/*
 * Copyright (C) 2013 - 2016  Xilinx, Inc.  All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL XILINX  BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the name of the Xilinx shall not be used
 * in advertising or otherwise to promote the sale, use or other dealings in this
 * Software without prior written authorization from Xilinx.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
const int BYTES_PER_PIXEL = 3; /// red, green, & blue
const int FILE_HEADER_SIZE = 14;
const int INFO_HEADER_SIZE = 40;

void generateBitmapImage(u_int8_t*** image, int height, int width, char* imageFileName);
unsigned char* createBitmapFileHeader(int height, int stride);
unsigned char* createBitmapInfoHeader(int height, int width);
int roundup(int a){
	if(a<0)
		return 0;
	else if(a>255)
		return 255;
	else 
		return a;
}
int main(int argc, char *argv[]) {
	FILE *fptr;
	if (argc < 3) {
		printf("Usage: %s <phys_addr> <offset>\n", argv[0]);
		return 0;
	}

	off_t offset = strtoul(argv[1], NULL, 0);
	size_t len = strtoul(argv[2], NULL, 0);
	// Truncate offset to a multiple of the page size, or mmap will fail.
	size_t pagesize = sysconf(_SC_PAGE_SIZE);
	off_t page_base = (offset / pagesize) * pagesize;
	off_t page_offset = offset - page_base;
	int height = 2048;
	int width = 2048;
	char* imageFileName = (char*) "bitmapImage.bmp";
	int k, j=0;
	
	   int fd = open("/dev/mem", O_SYNC);
	   unsigned char *mem = mmap(NULL, page_offset + len, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, page_base);
	   if (mem == MAP_FAILED) {
	   perror("Can't map memory");
	   return -1;
	   }
	   
	size_t i;

	u_int8_t* hex_rgb=(u_int8_t*) malloc(width*height*3*sizeof(u_int8_t*));
	if (hex_rgb == NULL)
	{
		fprintf(stderr, "Out of memory");
		exit(0);
	}

	for (i = 0; i < len; i+=4){
		u_int8_t Y1=(u_int8_t)mem[page_offset + (i+3)];
		u_int8_t Cb=(u_int8_t)mem[page_offset + (i+2)];
		u_int8_t Y2=(u_int8_t)mem[page_offset + (i+1)];
		u_int8_t Cr=(u_int8_t)mem[page_offset + (i+0)];
		//u_int8_t Y1=(u_int8_t)29;
		//u_int8_t Cb=(u_int8_t)255;
		//u_int8_t Y2=(u_int8_t)29;
		//u_int8_t Cr=(u_int8_t)107;
		*(hex_rgb+j+2)=roundup((u_int8_t)(Y1 + 1.4075 * (Cr-128)));//R
		*(hex_rgb+j+1)=roundup((u_int8_t)(Y1 - 0.3455 * (Cb-128) - (0.7169 * (Cr-128))));//G
		*(hex_rgb+j+0)=roundup((u_int8_t)(Y1 + 1.7790 * (Cb-128)));//B

		*(hex_rgb+j+5)=roundup((u_int8_t)(Y2 + 1.4075 * (Cr-128)));//R
		*(hex_rgb+j+4)=roundup((u_int8_t)(Y2 - 0.3455 * (Cb-128) - (0.7169 * (Cr-128))));//G
		*(hex_rgb+j+3)=roundup((u_int8_t)(Y2 + 1.7790 * (Cb-128)));//B

		j+=6;
	}
	u_int8_t*** image = (u_int8_t***)malloc(width * sizeof(u_int8_t**));
	if (image == NULL)
	{
		fprintf(stderr, "Out of memory");
		exit(0);
	}

	for (int k = 0; k < width; k++)
	{
		image[k] = (u_int8_t**)malloc(height * sizeof(u_int8_t*));

		if (image[k] == NULL)
		{
			fprintf(stderr, "Out of memory");
			exit(0);
		}

		for (int j = 0; j < width; j++)
		{
			image[k][j] = (u_int8_t*)malloc(BYTES_PER_PIXEL * sizeof(u_int8_t));
			if (image[k][j] == NULL)
			{
				fprintf(stderr, "Out of memory");
				exit(0);
			}
		}
	}
	for (k = 0; k <height; ++k) {
		for (j = 0; j <width; ++j) {
			int off=k*width*BYTES_PER_PIXEL+j*BYTES_PER_PIXEL;
			image[k][j][2] = (u_int8_t)  *(hex_rgb+off+2);          //red
			image[k][j][1] = (u_int8_t) *(hex_rgb+off+1);           //green
			image[k][j][0] = (u_int8_t) *(hex_rgb+off+0);		//blue
		}
	}
	generateBitmapImage((u_int8_t***) image, height, width, imageFileName);
	printf("Image generated!!\n");
	for (int k = 0; k < width; k++)
	{
		for (int j = 0; j < height; j++) {
			free(image[k][j]);
		}
		free(image[k]);
	}
	free(image);

	free(hex_rgb);
	return 0;
}

void generateBitmapImage (u_int8_t*** image, int height, int width, char* imageFileName)
{
	int widthInBytes = width * BYTES_PER_PIXEL;

	unsigned char padding[3] = {0, 0, 0};
	int paddingSize = (4 - (widthInBytes) % 4) % 4;

	int stride = (widthInBytes) + paddingSize;

	FILE* imageFile = fopen(imageFileName, "wb");

	unsigned char* fileHeader = createBitmapFileHeader(height, stride);
	fwrite(fileHeader, 1, FILE_HEADER_SIZE, imageFile);

	unsigned char* infoHeader = createBitmapInfoHeader(height, width);
	fwrite(infoHeader, 1, INFO_HEADER_SIZE, imageFile);

	int i;
	for (i = 0; i < height; i++) {
		for (int j = 0; j <width; ++j) {
			fwrite(image[i][j], BYTES_PER_PIXEL, 1, imageFile);
			fwrite(padding, 1, paddingSize, imageFile);
		}
	}

	fclose(imageFile);
}

unsigned char* createBitmapFileHeader (int height, int stride)
{
	int fileSize = FILE_HEADER_SIZE + INFO_HEADER_SIZE + (stride * height);

	static unsigned char fileHeader[] = {
		0,0,     /// signature
		0,0,0,0, /// image file size in bytes
		0,0,0,0, /// reserved
		0,0,0,0, /// start of pixel array
	};

	fileHeader[ 0] = (unsigned char)('B');
	fileHeader[ 1] = (unsigned char)('M');
	fileHeader[ 2] = (unsigned char)(fileSize      );
	fileHeader[ 3] = (unsigned char)(fileSize >>  8);
	fileHeader[ 4] = (unsigned char)(fileSize >> 16);
	fileHeader[ 5] = (unsigned char)(fileSize >> 24);
	fileHeader[10] = (unsigned char)(FILE_HEADER_SIZE + INFO_HEADER_SIZE);

	return fileHeader;
}

unsigned char* createBitmapInfoHeader (int height, int width)
{
	static unsigned char infoHeader[] = {
		0,0,0,0, /// header size
		0,0,0,0, /// image width
		0,0,0,0, /// image height
		0,0,     /// number of color planes
		0,0,     /// bits per pixel
		0,0,0,0, /// compression
		0,0,0,0, /// image size
		0,0,0,0, /// horizontal resolution
		0,0,0,0, /// vertical resolution
		0,0,0,0, /// colors in color table
		0,0,0,0, /// important color count
	};

	infoHeader[ 0] = (unsigned char)(INFO_HEADER_SIZE);
	infoHeader[ 4] = (unsigned char)(width      );
	infoHeader[ 5] = (unsigned char)(width >>  8);
	infoHeader[ 6] = (unsigned char)(width >> 16);
	infoHeader[ 7] = (unsigned char)(width >> 24);
	infoHeader[ 8] = (unsigned char)(height      );
	infoHeader[ 9] = (unsigned char)(height >>  8);
	infoHeader[10] = (unsigned char)(height >> 16);
	infoHeader[11] = (unsigned char)(height >> 24);
	infoHeader[12] = (unsigned char)(1);
	infoHeader[14] = (unsigned char)(BYTES_PER_PIXEL*8);

	return infoHeader;
}
