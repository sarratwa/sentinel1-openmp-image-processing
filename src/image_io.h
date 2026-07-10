#ifndef IMAGE_IO_H
#define IMAGE_IO_H

/*
    Shared image structure.

    width  = number of columns
    height = number of rows
    data   = grayscale pixels stored in one flat array

    Pixel access:
        data[y * width + x]
*/
typedef struct {
    int width;
    int height;
    unsigned char *data;
} Image;

Image read_pgm(const char *filename);
void write_pgm(const char *filename, Image image);
Image create_empty_image(int width, int height);
int images_are_equal(Image a, Image b);
void free_image(Image image);

#endif