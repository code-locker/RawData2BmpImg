**C code to convert Raw data to BMP image**

-In this code I am reading the Raw YUV 4:2:2(2048x2048) data from DDR address and converting to RGB value.

**Steps to compile the code**

	*gcc rawdata2bmp.c -o rawdata2bmp*
	
**Run the code**

	*./rawdata2bmp <start address> <size>*
	*./rawdata2bmp 0x500000 8388608*

size=8388608 beacuse image size is 8MB i.e. 8x1024x1024


