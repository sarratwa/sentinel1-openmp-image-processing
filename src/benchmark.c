#include <math.h>
#include <omp.h>
#include <stdlib.h>

#include "benchmark.h"

TimingResult time_filter(
    FilterFn filter_fn,
    Image input,
    Image output,
    Kernel kernel,
    int repetitions
) {
    double *times = malloc((size_t)repetitions * sizeof(double));
    double sum = 0.0;
    double min_time = 1e18;

    for (int r = 0; r < repetitions; r++) {
        double start = omp_get_wtime();
        filter_fn(input, output, kernel);
        double end = omp_get_wtime();

        double elapsed = end - start;
        times[r] = elapsed;
        sum += elapsed;

        if (elapsed < min_time) {
            min_time = elapsed;
        }
    }

    double mean = sum / (double)repetitions;
    double variance_sum = 0.0;

    for (int r = 0; r < repetitions; r++) {
        double diff = times[r] - mean;
        variance_sum += diff * diff;
    }

    double stddev = sqrt(variance_sum / (double)repetitions);

    free(times);

    TimingResult result;
    result.min = min_time;
    result.mean = mean;
    result.stddev = stddev;

    return result;
}

TimingResult time_lee_filter(
    LeeFilterFn filter_fn,
    Image input,
    Image output,
    LeeFilterParams params,
    int repetitions
) {
    double *times = malloc((size_t)repetitions * sizeof(double));
    double sum = 0.0;
    double min_time = 1e18;

    for (int r = 0; r < repetitions; r++) {
        double start = omp_get_wtime();
        filter_fn(input, output, params);
        double end = omp_get_wtime();

        double elapsed = end - start;
        times[r] = elapsed;
        sum += elapsed;

        if (elapsed < min_time) {
            min_time = elapsed;
        }
    }

    double mean = sum / (double)repetitions;
    double variance_sum = 0.0;

    for (int r = 0; r < repetitions; r++) {
        double diff = times[r] - mean;
        variance_sum += diff * diff;
    }

    double stddev = sqrt(variance_sum / (double)repetitions);

    free(times);

    TimingResult result;
    result.min = min_time;
    result.mean = mean;
    result.stddev = stddev;

    return result;
}

void write_csv_header(FILE *csv) {
    fprintf(csv,
            "filter,version,benchmark_type,width,height,kernel,threads,schedule,"
            "runtime_min,runtime_mean,runtime_stddev,speedup,efficiency,memory_mb\n");
}

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
) {
    double memory_mb = estimate_memory_mb(image, 3);

    fprintf(csv,
            "%s,%s,%s,%d,%d,%d,%d,%s,%.6f,%.6f,%.6f,%.3f,%.3f,%.2f\n",
            filter,
            version,
            benchmark_type,
            image.width,
            image.height,
            kernel_size,
            threads,
            schedule,
            timing.min,
            timing.mean,
            timing.stddev,
            speedup,
            efficiency,
            memory_mb);
}

double estimate_memory_mb(Image image, int number_of_images) {
    double bytes = (double)image.width *
                   (double)image.height *
                   (double)sizeof(float) *
                   (double)number_of_images;

    return bytes / (1024.0 * 1024.0);
}