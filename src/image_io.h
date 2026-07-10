#ifndef IMAGE_IO_H
#define IMAGE_IO_H

/*
    Image struct.

    Pixels are stored as unsigned short (16-bit) instead of
    unsigned char (8-bit) to preserve more of the original
    Sentinel-1 dynamic range after normalization.
*/
typedef struct {
    int width;
    int height;
    unsigned short *data;
} Image;

Image read_pgm(const char *filename);
void write_pgm(const char *filename, Image image);
Image create_empty_image(int width, int height);
int images_are_equal(Image a, Image b);
void free_image(Image image);

#endif