#include "benchmark.h"

void write_csv_header(FILE *csv) {
    fprintf(csv,
            "filter,version,width,height,kernel,threads,schedule,runtime,speedup,efficiency,memory_mb\n");
}

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
) {
    double memory_mb = estimate_memory_mb(image, 3);

    fprintf(csv,
            "%s,%s,%d,%d,%d,%d,%s,%.6f,%.3f,%.3f,%.2f\n",
            filter,
            version,
            image.width,
            image.height,
            kernel_size,
            threads,
            schedule,
            runtime,
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