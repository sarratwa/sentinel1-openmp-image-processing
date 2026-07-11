#ifndef BENCHMARK_H
#define BENCHMARK_H

#include <stdio.h>

#include "filters.h"
#include "image_io.h"

/*
    Result of running a filter multiple times.

    min:    fastest observed run. Used for speedup/efficiency, since
            it best approximates the noise-free runtime (noise can
            only add delay, never subtract it).
    mean:   average across all repetitions.
    stddev: standard deviation across all repetitions, reported
            alongside mean so run-to-run variance is visible rather
            than hidden behind a single number.
*/
typedef struct {
    double min;
    double mean;
    double stddev;
} TimingResult;

typedef void (*FilterFn)(Image input, Image output, Kernel kernel);

/*
    Runs the given filter function `repetitions` times and returns
    timing statistics. Every repetition writes into the same `output`
    buffer, so after this call `output` holds the result of the last
    repetition (fine, since all repetitions compute the same result
    for a correct, deterministic filter).
*/
TimingResult time_filter(
    FilterFn filter_fn,
    Image input,
    Image output,
    Kernel kernel,
    int repetitions
);

void write_csv_header(FILE *csv);

void write_csv_row(
    FILE *csv,
    const char *filter,
    const char *version,
    const char *benchmark_type,
    Image image,
    int kernel_size,
    int threads,
    const char *schedule,
    TimingResult timing,
    double speedup,
    double efficiency
);

double estimate_memory_mb(Image image, int number_of_images);

#endif