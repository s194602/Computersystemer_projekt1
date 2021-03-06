
//To compile (win): gcc cbmp.c cellCounter.c -o cellCounter.exe -std=c99
//To run (win): ./cellCounter.exe example.bmp example_result.bmp

#include <stdlib.h>
#include <stdio.h>
#include "cbmp.h"
#include <time.h>

#define coordinateSize 1000
#define bit_width 30 // 32*30 = 960 we need at least 950

#define SetBit(A, x, y) (A[(x / 32)][y] |= (1 << (x % 32)))
#define ClearBit(A, x, y) (A[(x / 32)][y] &= ~(1 << (x % 32)))
#define TestBit(A, x, y) (A[(x / 32)][y] & (1 << (x % 32)))

unsigned char color_image[BMP_WIDTH][BMP_HEIGTH][BMP_CHANNELS];
unsigned int binary_image1[bit_width][BMP_HEIGTH];
unsigned int binary_image2[bit_width][BMP_HEIGTH];

unsigned short coordinates[coordinateSize][2];

unsigned int cellCount = 0;
unsigned char threshold = 95;
unsigned char eroded = 1;
unsigned char flip = 1;

// Combines r-g-b pixels to singular gray pixel.
// Then make image binary meaning alle colors are either 0=black or 1=white
void convertToBinary(unsigned char color_image[BMP_WIDTH][BMP_HEIGTH][BMP_CHANNELS], unsigned int binary_image1[bit_width][BMP_HEIGTH])
{
    unsigned char tmpGray;
    for (int x = 0; x < BMP_WIDTH; x++)
    {
        for (int y = 0; y < BMP_HEIGTH; y++)
        {
            tmpGray = (color_image[x][y][0] + color_image[x][y][1] + color_image[x][y][2]) / 3;
            if (tmpGray < threshold)
            {
                ClearBit(binary_image1, x, y);
            }
            else
            {
                SetBit(binary_image1, x, y);
            }
        }
    }

    // Makes the edge of the color image black
    for (int x = 0; x < BMP_WIDTH; x++)
    {
        ClearBit(binary_image1, x, 0);
        ClearBit(binary_image1, 0, x);
        ClearBit(binary_image1, x, (BMP_HEIGTH - 1));
        ClearBit(binary_image1, (BMP_HEIGTH - 1), x);
    }
}

// Makes a binary image to be output.
void tmpBinaryOut(unsigned int binary_image[bit_width][BMP_HEIGTH], unsigned char color_image[BMP_WIDTH][BMP_HEIGTH][BMP_CHANNELS])
{
    for (int x = 0; x < BMP_WIDTH; x++)
    {
        for (int y = 0; y < BMP_HEIGTH; y++)
        {
            for (int c = 0; c < BMP_CHANNELS; c++)
            {
                color_image[x][y][c] = TestBit(binary_image, x, y) ? 255 : 0;
            }
        }
    }
}

// Take the input image and run though it and count white pixels then removing pixels if criteria is met.
// Save the new image in another memory slot.
void erodeImg(unsigned int binary_image1[bit_width][BMP_HEIGTH], unsigned int binary_image2[bit_width][BMP_HEIGTH])
{
    eroded = 0;

    for (int x = 0; x < BMP_WIDTH; x++)
    {
        for (int y = 0; y < BMP_HEIGTH; y++)
        {
            TestBit(binary_image1, x, y) ? SetBit(binary_image2, x, y) : ClearBit(binary_image2, x, y);
        }
    }

    for (int x = 1; x < BMP_WIDTH - 1; x++)
    {
        for (int y = 1; y < BMP_HEIGTH - 1; y++)
        {
            if (TestBit(binary_image1, x, y))
            {
                unsigned char WhiteCounter = 0;

                for (int x1 = x - 1; x1 < x + 1; x1++)
                {
                    if (TestBit(binary_image1, x1, (y - 1)))
                    {
                        WhiteCounter++;
                    }
                    if (TestBit(binary_image1, x1, (y + 1)))
                    {
                        WhiteCounter++;
                    }
                }

                if (TestBit(binary_image1, (x - 1), y))
                {
                    WhiteCounter++;
                }
                if (TestBit(binary_image1, (x + 1), y))
                {
                    WhiteCounter++;
                }

                if (WhiteCounter <= 5)
                {
                    ClearBit(binary_image2, x, y);
                    eroded = 1;
                }
            }
        }
    }

    //flips the flip
    flip ^= 1;
}

// Check for white pixels (1) in the 14 by 14 exclusion frame.
// Returns 1 if found and 0 if not.
char isEdgeWhite(unsigned int binary_image[bit_width][BMP_HEIGTH], unsigned int x, unsigned int y)
{
    char whiteEdgeFound = 0;
    for (int x1 = x; x1 < x + 15; x1++)
    {
        if (TestBit(binary_image, x1, y) || TestBit(binary_image, x1, (y + 13)))
        {
            whiteEdgeFound = 1;
            break;
        }
    }
    if (!whiteEdgeFound)
    {
        for (int y1 = y; y1 < y + 15; y1++)
        {
            if (TestBit(binary_image, x, y1) || TestBit(binary_image, (x + 13), y1))
            {
                whiteEdgeFound = 1;
                break;
            }
        }
    }
    return whiteEdgeFound;
}

// Use capturing area of 12-12 pixels and a 14-14 exclusion frame around, when a cell is detected count it and remember its
// center (coordinates) and remove the cell from the image.
void detectAndRemoveSpots(unsigned int binary_image[bit_width][BMP_HEIGTH], unsigned short coordinates[coordinateSize][2])
{
    for (int x = 0; x < BMP_WIDTH - 13; x += 2)
    {
        for (int y = 0; y < BMP_HEIGTH - 13; y += 2)
        {
            if (!isEdgeWhite(binary_image,x,y))
            {
                //Count white cells around current index (x,y)
                int whiteCellFound = 0;
                for (int x1 = x + 1; x1 < x + 13; x1++)
                {
                    for (int y1 = y + 1; y1 < y + 13; y1++)
                    {
                        if (TestBit(binary_image, x1, y1))
                        {
                            if (!whiteCellFound)
                            {
                                coordinates[cellCount][0] = x + 7;
                                coordinates[cellCount][1] = y + 7;
                                printf("%d, %d\n", coordinates[cellCount][0], coordinates[cellCount][1]);
                                cellCount++;
                                whiteCellFound = 1;
                            }

                            ClearBit(binary_image, x1, y1);
                        }
                    }
                }
            }
        }
    }
}

void makeRedCross(unsigned char color_image[BMP_WIDTH][BMP_HEIGTH][BMP_CHANNELS], int x, int y)
{
    int x1 = x - 5;
    int y1 = y - 5;
    while (x1 <= x + 5 && y1 <= y + 5)
    {
        color_image[x1][y + 1][0] = 255;
        color_image[x1][y][0] = 255;
        color_image[x1][y - 1][0] = 255;

        for (int c = 1; c < BMP_CHANNELS; c++)
        {
            color_image[x1][y + 1][c] = 0;
            color_image[x1][y][c] = 0;
            color_image[x1][y - 1][c] = 0;
        }

        if (y1 < y - 1 || y1 > y + 1)
        {
            color_image[x + 1][y1][0] = 255;
            color_image[x][y1][0] = 255;
            color_image[x - 1][y1][0] = 255;

            for (int c = 1; c < BMP_CHANNELS; c++)
            {
                color_image[x + 1][y1][c] = 0;
                color_image[x][y1][c] = 0;
                color_image[x - 1][y1][c] = 0;
            }
        }
        x1++;
        y1++;
    }
}

// Take original image and put red x on all coordinates
void constructOutputImg(unsigned char color_image[BMP_WIDTH][BMP_HEIGTH][BMP_CHANNELS], unsigned short coordinates[coordinateSize][2])
{
    for (int x = 0; x < cellCount; x++)
    {
        makeRedCross(color_image, coordinates[x][0], coordinates[x][1]);
    }
}

int main(int argc, char **argv)
{
    //argc counts how may arguments are passed
    //argv[0] is a string with the name of the program
    //argv[1] is the first command line argument (input image)
    //argv[2] is the second command line argument (output image)

    //Checking that 2 arguments are passed
    if (argc != 3)
    {
        fprintf(stderr, "Usage: %s <output file path> <output file path>\n", argv[0]);
        exit(1);
    }

    //Load image from file
    read_bitmap(argv[1], color_image);

    clock_t start, end;
    double cpu_time_used;
    start = clock();
    
    convertToBinary(color_image, binary_image1);

    while (1)
    {
        if (flip)
        {
            erodeImg(binary_image1, binary_image2);
            if (!eroded)
            {
                break;
            }
            detectAndRemoveSpots(binary_image2, coordinates);
        }
        else
        {
            erodeImg(binary_image2, binary_image1);
            if (!eroded)
            {
                break;
            }
            detectAndRemoveSpots(binary_image1, coordinates);
        }
    }

    printf("The cellcount is: %d\n", cellCount);
    constructOutputImg(color_image, coordinates);

    end = clock();
    cpu_time_used = end - start;
    printf("Total time: %f ms\n", cpu_time_used * 1000.0 / CLOCKS_PER_SEC);

    //Save image to file
    //and print coordinates and cellcount here
    write_bitmap(color_image, argv[2]);

    //coordinateComparison(coordinates);
    printf("Done!\n");
    return 0;
}
