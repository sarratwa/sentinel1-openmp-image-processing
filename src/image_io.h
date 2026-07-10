#ifndef IMAGE_IO_H
#define IMAGE_IO_H

typedef struct {
    int width;
    int height;
    float *data;
} Image;

/* Read band 1 from a TIFF/GeoTIFF and convert its pixels to float. */
Image read_tiff(const char *filename);

/* Write a Float32 GeoTIFF and copy georeferencing from the input file. */
void write_tiff(
    const char *filename,
    Image image,
    const char *reference_filename
);

/* Allocate a zero-initialized float image. */
Image create_empty_image(int width, int height);

/* Exact comparison, suitable because both filters perform identical operations. */
int images_are_equal(Image a, Image b);

/* Release the pixel buffer. */
void free_image(Image image);

#endif