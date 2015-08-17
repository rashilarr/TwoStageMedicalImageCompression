#include <stdio.h>
#include <stdlib.h>

typedef struct {
	unsigned short type;
	unsigned int size;
	unsigned short reserved1;
	unsigned short reserved2;
	unsigned int offsetbits;
} BITMAPFILEHEADER;

typedef struct {
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

void constructOR (unsigned char *OR, unsigned char *GSM, unsigned char *BM, unsigned int imageSize);
void expandBM (unsigned char *BM, unsigned char *compressedBM, unsigned int imageSize);
unsigned int RLDecode(unsigned char *stream, unsigned char *decodedBuffer);

int main (int argc, char *argv[])
{
    FILE *fin, *fout;
    unsigned int width, height;
    unsigned char *imgBuffer, *BM;
    unsigned char *GSM, *compressedBM;
	unsigned char *stage2symbols, *stage2decompressedBuffer;
	unsigned char *CLUT;
    unsigned int imageSize, k, filesize, stage2compressedBytes, stage2decompressedBytes, stage2decompressedBytesWC;
	BITMAPFILEHEADER *fheader;
	BITMAPINFOHEADER *iheader;

	// Checking the command line arguments
    if (argc < 3) {
        fprintf (stderr, "Usage: %s <InputFilenameWithFullPath> <OutputFilename>\n", argv[0]);
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

	// Calculate the filesize
	fseek (fin, 0, SEEK_END);
	filesize = ftell (fin);
	rewind (fin);

	// Calculate the second stage compressed bytes
	stage2compressedBytes = filesize - 54;

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

	// Allocate space for stage-2 compressed and decompressed buffer
	stage2symbols = (unsigned char*) malloc(stage2compressedBytes);
	// Compute the bytes required for stage-2 decompression
	// CLUT(1024 Bytes) + BM(W*H/8 Bytes) + GSM(k Bytes)
	stage2decompressedBytesWC = 1024 + (width * height >> 3) + width * height;
	stage2decompressedBuffer = (unsigned char*) malloc(stage2decompressedBytesWC);

	// Read the stage-2 compressed stream
	fread(stage2symbols, sizeof(unsigned char), stage2compressedBytes, fin);

	// Call the RLDecode() function for second level decompression
	stage2decompressedBytes = RLDecode(stage2symbols, stage2decompressedBuffer);

	// Calculating the value of k
	k = stage2decompressedBytes - 1024 - (imageSize >> 3);

	// Allocating the memory for the buffers
    imgBuffer = (unsigned char*) malloc (imageSize);
    // GSM       = (unsigned char*) malloc (k);
    BM        = (unsigned char*) malloc (imageSize);
	// compressedBM = (unsigned char*) malloc (imageSize >> 3);
	CLUT = (unsigned char*) stage2decompressedBuffer;
	compressedBM = (unsigned char*) (stage2decompressedBuffer + 1024);
	GSM = (unsigned char*) (stage2decompressedBuffer + 1024 + (imageSize >> 3));

	// Reading the Color Look Up Table of 1024 bytes
	// fread (CLUT, sizeof (unsigned char), 1024, fin);
	// Reading the compressed Binary Matrix (BM)
	// fread (compressedBM, sizeof (unsigned char), imageSize >> 3, fin);
	// Read the Gray Scale Matrix data
	// fread (GSM, sizeof (unsigned char), k, fin);

	// Decompress the compressed Binary Matrix
	expandBM (BM, compressedBM, imageSize);
	// Construct the original image
	constructOR (imgBuffer, GSM, BM, imageSize);

	// Storing the compressed image
	fwrite (&fheader->type, sizeof (unsigned short), 1, fout);
	fwrite (&fheader->size, sizeof (unsigned int), 1, fout);
	fwrite (&fheader->reserved1, sizeof (unsigned short), 1, fout);
	fwrite (&fheader->reserved2, sizeof (unsigned short), 1, fout);
	fwrite (&fheader->offsetbits, sizeof (unsigned int), 1, fout);
	fwrite (iheader, sizeof (BITMAPINFOHEADER), 1, fout);
	fwrite (CLUT, sizeof (unsigned char), 1024, fout);
	fwrite (imgBuffer, sizeof (unsigned char), imageSize, fout);

    free (imgBuffer);
    // free (GSM);
    free (BM);

    fclose (fin);
    fclose (fout);

    return 0;
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

void expandBM (unsigned char *BM, unsigned char *compressedBM, unsigned int imageSize)
{
	unsigned int pixel, bit;

	for (pixel = 0; pixel < (imageSize >> 3); pixel++) {
		for (bit = 0; bit < 8; bit++) {
			BM[pixel * 8 + bit] = (compressedBM[pixel] & (1 << (7 - bit))) > 0 ? 1 : 0;
		}
	}

	return;
}

unsigned int RLDecode(unsigned char *stream, unsigned char *decodedBuffer)
{
	unsigned char byte1, byte2, byte3;
	unsigned int count = 0;

	while(1) {
		// Get the first byte
		byte1 = *stream++;
		if (!byte1) {
			byte2 = *stream++;
			if (!byte2)
				break;
			else {
				if (byte2 & 0x80) {
					int L;
					// Calculate the value of L
					L = byte2 & 0x7F;
					// Get the Value of C
					byte3 = *stream++;
					while(L--) {
						decodedBuffer[count++] = byte3;
					}
				}
				else {
					int L;
					// Calculate the value of L
					L = byte2;
					while(L--) {
						decodedBuffer[count++] = 0;
					}
				}
			}
		}
		else {
			decodedBuffer[count++] = byte1;
		}
	}
	return count;
}
