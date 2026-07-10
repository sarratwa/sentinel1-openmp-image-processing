#ifndef BENCHMARK_H
#define BENCHMARK_H

#include <stdio.h>
#include "image_io.h"

void write_csv_header(FILE *csv);

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

double estimate_memory_mb(Image image, int number_of_images);

#endif