#ifndef IMAGE_IO_H
#define IMAGE_IO_H

// central image structure columns x rows
// the data is a pointer to the array containning the pixels
typedef struct {
    int width;
    int height;
    float *data;
} Image;

// Read band 1 from a TIFF/GeoTIFF and convert its pixels to float (vv file)
Image read_tiff(const char *filename);

// Write a Float32 GeoTIFF and copy georeferencing from the input file
// pixel values are converted to Float32 in memory + result is in the new GeoTIFF (GDAL) 
void write_tiff(
    const char *filename,
    Image image,
    const char *reference_filename
);

// Allocate a zero-initialized float image 
// We need a second image buffer to write the filter results into
Image create_empty_image(int width, int height);

// Exact comparison, suitable because both filters perform identical operations
int images_are_equal(Image a, Image b);

// Release the pixel buffer (the dynamically allocated pixel array)
void free_image(Image image);

#endif