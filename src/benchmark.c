#include "benchmark.h"

/*
    Writes the first row of the CSV file.
*/
void write_csv_header(FILE *csv) {
    fprintf(csv,
            "filter,version,width,height,kernel,threads,schedule,runtime,speedup,efficiency,memory_mb\n");
}

/*
    Writes one result row to the CSV file.
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

/*
    Estimates memory usage in MB.

    Formula:
        width * height * bytes_per_pixel * number_of_images

    bytes_per_pixel = 2, since pixels are now 16-bit (unsigned short)
    instead of 8-bit (unsigned char). This roughly doubles the
    reported memory footprint compared to the old 8-bit pipeline,
    which is expected and correct.
*/
double estimate_memory_mb(Image image, int number_of_images) {
    double bytes = (double)image.width *
                   (double)image.height *
                   sizeof(unsigned short) *
                   number_of_images;

    return bytes / (1024.0 * 1024.0);
}