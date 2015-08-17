#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct BITMAPFILEHEADER {
	unsigned short type;
	unsigned int size;
	unsigned short reserved1;
	unsigned short reserved2;
	unsigned int offsetbits;
} BITMAPFILEHEADER;

typedef struct BITMAPINFOHEADER {
	unsigned int size;
	unsigned int width;
	unsigned int height;
	unsigned short planes;
	unsigned short bitcount;
	unsigned int compression;
	unsigned int sizeimage;
	int xpelspermeter;
	int ypelspermeter;
	unsigned int colorsused;
	unsigned int colorsimportant;
} BITMAPINFOHEADER;

unsigned int constructGSM (unsigned char *OR, unsigned char *GSM, unsigned int imageSize);
void constructBM  (unsigned char *OR, unsigned char *BM, unsigned int imageSize);
void constructOR (unsigned char *OR, unsigned char *GSM, unsigned char *BM, unsigned int imageSize);
void compressBM (unsigned char *BM, unsigned char *compressedBM, unsigned int imageSize);
unsigned int RLE (unsigned char *outBuffer, unsigned char *symbol, unsigned int NoOfSymbols);

int main (int argc, char *argv[])
{
	FILE *fin, *fout, *frle;
	unsigned int width, height;
	unsigned int inputfilesize, stage1compressedfilesize, stage2compressedfilesize;
	unsigned char *imgBuffer, *outBuffer, *BM;
	unsigned char *GSM, *compressedBM;
	unsigned char *symbols, *compProposedBuffer;
	unsigned char CLUT[1024];
	unsigned int imageSize, k, rleCompressedBytes, NoOfSymbols;
	BITMAPFILEHEADER *fheader;
	BITMAPINFOHEADER *iheader;

	// Checking the command line arguments
	if (argc < 4) {
		fprintf (stderr, "Usage: <exe-name> <InputFilenameWithFullPath> <OutputFilename>\n");
		return -1;
	}
	// Opening the Input image
	if ((fin = fopen (argv[1], "rb")) == NULL) {
		fprintf (stderr, "Error: Opening input image.\n");
		return -1;
	}
	// Opening the file for writing the compressed image
	if ((fout = fopen (argv[2], "wb")) == NULL) {
		fprintf (stderr, "Error: Opening output file.\n");
		return -1;
	}
	// Opening the file for writing the second stage compression output
	if ((frle = fopen(argv[3], "wb")) == NULL) {
		fprintf(stderr, "Error: Opening stage-2 compression output file...\n");
		return -1;
	}

	fseek(fin, 0, SEEK_END);
	inputfilesize = ftell(fin);
	rewind(fin);

	fheader = (BITMAPFILEHEADER*) malloc (sizeof (BITMAPFILEHEADER));
	iheader = (BITMAPINFOHEADER*) malloc (sizeof (BITMAPINFOHEADER));

	// Reading the BMP Header of 54 Bytes
	fread (&fheader->type, sizeof (unsigned short), 1, fin);
	fread (&fheader->size, sizeof (unsigned int), 1, fin);
	fread (&fheader->reserved1, sizeof (unsigned short), 1, fin);
	fread (&fheader->reserved2, sizeof (unsigned short), 1, fin);
	fread (&fheader->offsetbits, sizeof (unsigned int), 1, fin);
	fread (iheader, sizeof (BITMAPINFOHEADER), 1, fin);

	// Retrieving the width & height of the image
	width = iheader->width;
	height = iheader->height;

	// Calculating the image size
	imageSize = width * height;

	// Allocating the memory for the buffers
	imgBuffer = (unsigned char*) malloc (imageSize);
	outBuffer = (unsigned char*) malloc (imageSize);
	GSM       = (unsigned char*) malloc (imageSize);
	BM        = (unsigned char*) malloc (imageSize);
	compressedBM = (unsigned char*) malloc (imageSize >> 3);

	// Reading the Color Look Up Table of 1024 bytes
	fread (CLUT, sizeof (unsigned char), 1024, fin);
	// Reading the Image data
	fread (imgBuffer, sizeof (unsigned char), imageSize, fin);

	// Construct the Binary Matrix (BM)
	constructBM  (imgBuffer, BM, imageSize);
	printf ("Binary Matrix size = %d bytes.\n", imageSize >> 3);
	// Construct the Gray Scale Matrix (GSM)
	k = constructGSM (imgBuffer, GSM, imageSize);
	printf ("GSM Matrix size = %d bytes.\n", k);
	// Compress the Binary Matrix (BM)
	compressBM (BM, compressedBM, imageSize);

	// Allocate CLUT size + Binary Matrix size + GSM size
	// That is 1024 + (imageSize / 8) + k
	NoOfSymbols = 1024 + (imageSize >> 3) + k;
	symbols = (unsigned char*) malloc (NoOfSymbols);
	// Copy all the data
	memcpy (symbols, CLUT, 1024);
	memcpy (symbols + 1024, compressedBM, imageSize >> 3);
	memcpy (symbols + 1024 + imageSize / 8, GSM, k);

	// Allocate memory for the proposed compression buffer
	// Allocate double the amount of memory allocated for symbols(safer side)
	compProposedBuffer = (unsigned char*) malloc (2 * NoOfSymbols);

	// Call the RLE function for the second level of compression
	rleCompressedBytes = RLE(compProposedBuffer, symbols, NoOfSymbols);

	// Storing the stage-1 compressed image
	fwrite (&fheader->type, sizeof (unsigned short), 1, fout);
	fwrite (&fheader->size, sizeof (unsigned int), 1, fout);
	fwrite (&fheader->reserved1, sizeof (unsigned short), 1, fout);
	fwrite (&fheader->reserved2, sizeof (unsigned short), 1, fout);
	fwrite (&fheader->offsetbits, sizeof (unsigned int), 1, fout);
	fwrite (iheader, sizeof (BITMAPINFOHEADER), 1, fout);
	fwrite (CLUT, sizeof (unsigned char), 1024, fout);
	fwrite (compressedBM, sizeof (unsigned char), imageSize >> 3, fout);
	fwrite (GSM, sizeof (unsigned char), k, fout);
	stage1compressedfilesize = 54 + 1024 + (imageSize >> 3) + k;

	// Storing the stage-2 compressed Image
	fwrite(&fheader->type, sizeof(unsigned short), 1, frle);
	fwrite(&fheader->size, sizeof(unsigned int), 1, frle);
	fwrite(&fheader->reserved1, sizeof(unsigned short), 1, frle);
	fwrite(&fheader->reserved2, sizeof(unsigned short), 1, frle);
	fwrite(&fheader->offsetbits, sizeof(unsigned int), 1, frle);
	fwrite(iheader, sizeof(BITMAPINFOHEADER), 1, frle);
	fwrite(compProposedBuffer, sizeof(unsigned char), rleCompressedBytes, frle);
	stage2compressedfilesize = 54 + rleCompressedBytes;

	printf("Stage-1 Compression Ratio = %f\n",
		(double)inputfilesize / (double)stage1compressedfilesize);
	printf("Stage-2 Compression Ratio = %f\n",
		(double)inputfilesize / (double)stage2compressedfilesize);

	free (imgBuffer);
	free (outBuffer);
	free (GSM);
	free (BM);

	fclose (fin);
	fclose (fout);
	fclose (frle);

	return 0;
}

unsigned int constructGSM (unsigned char *OR, unsigned char *GSM, unsigned int imageSize)
{
	unsigned int i, k = 0;

	if (OR == NULL || GSM == NULL) {
		fprintf (stderr, "Error: Improper Buffer . . .\n");
		return 0;
	}

	GSM[0] = OR[0];
	for ( i = 0; i < imageSize - 1; i++ ) {
		if ( OR[i + 1] != OR[i] )
			GSM[++k] = OR[i + 1];
	}

	return k + 1;
}

void constructBM (unsigned char *OR, unsigned char *BM, unsigned int imageSize)
{
	unsigned int i;

	if (OR == NULL || BM == NULL) {
		fprintf (stderr, "Error: Improper Buffer . . .\n");
		return;
	}

	BM[0] = 1;
	for ( i = 0; i < imageSize - 1; i++ ) {
		if ( OR[i] == OR[i + 1] )
			BM[i + 1] = 0;
		else
			BM[i + 1] = 1;
	}

	return;
}

void constructOR (unsigned char *OR, unsigned char *GSM, unsigned char *BM, unsigned int imageSize)
{
	unsigned int pixelNum, k = 0;
	unsigned char pixel;

	if (OR == NULL || GSM == NULL || BM == NULL) {
		fprintf (stderr, "Error: Improper Buffer . . .\n");
		return;
	}

	OR[0] = GSM[0];
	pixel = GSM[0];

	for (pixelNum = 1; pixelNum < imageSize; pixelNum++) {
		if (BM[pixelNum]) {
			pixel = GSM[++k];
		}
		OR[pixelNum] = pixel;
	}

	return;
}

void compressBM (unsigned char *BM, unsigned char *compressedBM, unsigned int imageSize)
{
	unsigned int pixel, bit;
	unsigned char byte;

	for (pixel = 0; pixel < imageSize; pixel += 8) {
		byte = 0;
		for (bit = 0; bit < 8; bit++) {
			byte |= BM[pixel + bit] << (7 - bit);
		}
		compressedBM[pixel >> 3] = byte;
	}

	return;
}

unsigned int RLE (unsigned char *outBuffer, unsigned char *symbol, unsigned int NoOfSymbols)
{
	unsigned char *outPtr = outBuffer;
	unsigned int i;

	for (i = 0; i < NoOfSymbols; i++) {
		unsigned int L = 1, C;
		C = symbol[i];
		if(i + 1 != NoOfSymbols) {
			while(symbol[L + i] == C)
				L++;
		}
		i += L - 1;
		while(L > 0) {
			if (!C) {
				if (L < 128) {
					*outPtr++ = 0x00;
					*outPtr++ = L;
					L = 0;
				}
				else {
					*outPtr++ = 0x00;
					*outPtr++ = 0x7F;
					L -= 127;
				}
			}
			else {
				if (L > 2) {
					if (L < 128) {
						*outPtr++ = 0x00;
						*outPtr++ = (1 << 7) | L;
						*outPtr++ = C;
						L = 0;
					}
					else {
						*outPtr++ = 0x00;
						*outPtr++ = 0xFF;
						*outPtr++ = C;
						L -= 127;
					}
				}
				else {
					*outPtr++ = C;
					L -= 1;
				}
			}
		}
	}

	// Add the End of 8-Bit pixel code string
	*outPtr++ = 0;
	*outPtr++ = 0;

	return (unsigned int) (outPtr - outBuffer);
}
