#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "image_io.h"

/*
    Reads a binary PGM image file.

    Supported format:
        P5
        width height
        255
        binary pixel data
*/
Image read_pgm(const char *filename) {
    FILE *file = fopen(filename, "rb");

    if (!file) {
        fprintf(stderr, "Error: Could not open file %s\n", filename);
        exit(1);
    }

    char magic[3];

    fscanf(file, "%2s", magic);

    if (strcmp(magic, "P5") != 0) {
        fprintf(stderr, "Error: Only binary PGM P5 format is supported.\n");
        fclose(file);
        exit(1);
    }

    int width;
    int height;
    int max_value;

    fscanf(file, "%d %d", &width, &height);
    fscanf(file, "%d", &max_value);

    if (max_value != 255) {
        fprintf(stderr, "Error: Only 8-bit PGM images with max value 255 are supported.\n");
        fclose(file);
        exit(1);
    }

    // Consume one whitespace character after the header
    fgetc(file);

    unsigned char *data = malloc(width * height * sizeof(unsigned char));

    if (!data) {
        fprintf(stderr, "Error: Memory allocation failed for input image.\n");
        fclose(file);
        exit(1);
    }

    size_t pixels_read = fread(data, sizeof(unsigned char), width * height, file);

    if (pixels_read != (size_t)(width * height)) {
        fprintf(stderr, "Error: Could not read all pixels from file.\n");
        free(data);
        fclose(file);
        exit(1);
    }

    fclose(file);

    Image image;
    image.width = width;
    image.height = height;
    image.data = data;

    return image;
}

/*
    Writes an image as binary PGM file.
*/
void write_pgm(const char *filename, Image image) {
    FILE *file = fopen(filename, "wb");

    if (!file) {
        fprintf(stderr, "Error: Could not write file %s\n", filename);
        exit(1);
    }

    fprintf(file, "P5\n%d %d\n255\n", image.width, image.height);
    fwrite(image.data, sizeof(unsigned char), image.width * image.height, file);

    fclose(file);
}

/*
    Creates an empty image with allocated memory.
*/
Image create_empty_image(int width, int height) {
    Image image;

    image.width = width;
    image.height = height;
    image.data = malloc(width * height * sizeof(unsigned char));

    if (!image.data) {
        fprintf(stderr, "Error: Could not allocate memory for image.\n");
        exit(1);
    }

    return image;
}

/*
    Compares two images pixel by pixel.

    Return:
        1 = equal
        0 = different
*/
int images_are_equal(Image a, Image b) {
    if (a.width != b.width || a.height != b.height) {
        return 0;
    }

    int total_pixels = a.width * a.height;

    for (int i = 0; i < total_pixels; i++) {
        if (a.data[i] != b.data[i]) {
            return 0;
        }
    }

    return 1;
}

/*
    Frees the image memory.
*/
void free_image(Image image) {
    free(image.data);
}