#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "image_io.h"

/*
    Reads a binary 16-bit PGM image file.

    Supported format:
        P5
        width height
        65535
        binary pixel data (big-endian, 2 bytes per pixel)

    The PGM specification requires big-endian byte order
    (most significant byte first) whenever maxval > 255.
    Since x86 is little-endian internally, each pixel must
    be byte-swapped after reading.
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

    if (max_value != 65535) {
        fprintf(stderr,
                "Error: Only 16-bit PGM images with max value 65535 are supported.\n");
        fclose(file);
        exit(1);
    }

    // Consume one whitespace character after the header
    fgetc(file);

    unsigned short *data = malloc((size_t)width * (size_t)height * sizeof(unsigned short));

    if (!data) {
        fprintf(stderr, "Error: Memory allocation failed for input image.\n");
        fclose(file);
        exit(1);
    }

    // Read raw big-endian bytes first, two bytes per pixel
    size_t total_pixels = (size_t)width * (size_t)height;
    unsigned char *raw_bytes = malloc(total_pixels * 2);

    if (!raw_bytes) {
        fprintf(stderr, "Error: Memory allocation failed for raw byte buffer.\n");
        free(data);
        fclose(file);
        exit(1);
    }

    size_t bytes_read = fread(raw_bytes, 1, total_pixels * 2, file);

    if (bytes_read != total_pixels * 2) {
        fprintf(stderr, "Error: Could not read all pixels from file.\n");
        free(raw_bytes);
        free(data);
        fclose(file);
        exit(1);
    }

    fclose(file);

    // Byte-swap: big-endian (MSB first) -> host uint16
    for (size_t i = 0; i < total_pixels; i++) {
        unsigned char high_byte = raw_bytes[i * 2];
        unsigned char low_byte = raw_bytes[i * 2 + 1];
        data[i] = (unsigned short)((high_byte << 8) | low_byte);
    }

    free(raw_bytes);

    Image image;
    image.width = width;
    image.height = height;
    image.data = data;

    return image;
}

/*
    Writes an image as a binary 16-bit PGM file (P5, maxval 65535).

    Pixels must be written big-endian (most significant byte first)
    per the PGM specification.
*/
void write_pgm(const char *filename, Image image) {
    FILE *file = fopen(filename, "wb");

    if (!file) {
        fprintf(stderr, "Error: Could not write file %s\n", filename);
        exit(1);
    }

    fprintf(file, "P5\n%d %d\n65535\n", image.width, image.height);

    size_t total_pixels = (size_t)image.width * (size_t)image.height;
    unsigned char *raw_bytes = malloc(total_pixels * 2);

    if (!raw_bytes) {
        fprintf(stderr, "Error: Memory allocation failed while writing image.\n");
        fclose(file);
        exit(1);
    }

    for (size_t i = 0; i < total_pixels; i++) {
        unsigned short pixel = image.data[i];
        raw_bytes[i * 2] = (unsigned char)(pixel >> 8);       // MSB first
        raw_bytes[i * 2 + 1] = (unsigned char)(pixel & 0xFF); // LSB second
    }

    fwrite(raw_bytes, 1, total_pixels * 2, file);

    free(raw_bytes);
    fclose(file);
}

/*
    Creates an empty image with allocated memory.

    The buffer is zero-initialized so that physical pages are
    touched before benchmark timing starts (avoids first-touch
    page-fault overhead skewing the first measured run).
*/
Image create_empty_image(int width, int height) {
    Image image;

    image.width = width;
    image.height = height;
    image.data = calloc((size_t)width * (size_t)height, sizeof(unsigned short));

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

    size_t total_pixels = (size_t)a.width * (size_t)a.height;

    for (size_t i = 0; i < total_pixels; i++) {
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