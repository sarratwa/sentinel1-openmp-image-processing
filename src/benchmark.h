#ifndef BENCHMARK_H
#define BENCHMARK_H

#include <stdio.h>

#include "image_io.h"

/*
    Writes the CSV header.
*/
void write_csv_header(FILE *csv);

/*
    Writes one benchmark result row.
*/
void write_csv_row(
    FILE *csv,
    const char *filter,
    const char *version,
    Image image,
    int kernel_size,
    int threads,
    const char *schedule,
    double runtime,
    double speedup,
    double efficiency
);

/*
    Calculates approximate memory usage in megabytes.

    We currently store:
        input image
        sequential output image
        OpenMP output image

    Each pixel is unsigned char = 1 byte.
*/
double estimate_memory_mb(Image image, int number_of_images);

#endif