#include <omp.h>
#include <stdio.h>
#include <stdlib.h>

#include "benchmark.h"
#include "filters.h"
#include "image_io.h"

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <input.tif>\n", argv[0]);
        fprintf(stderr, "Example: %s data/sentinel_vv.tiff\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *input_path = argv[1];
    const char *output_seq_path = "output/gaussian_seq.tif";
    const char *output_omp_path = "output/gaussian_omp.tif";
    const char *csv_path = "results/benchmark_results.csv";

    Image input = read_tiff(input_path);
    Image output_seq = create_empty_image(input.width, input.height);
    Image output_omp = create_empty_image(input.width, input.height);

    FILE *csv = fopen(csv_path, "w");

    if (!csv) {
        fprintf(stderr, "Error: Could not create CSV file %s\n", csv_path);
        free_image(input);
        free_image(output_seq);
        free_image(output_omp);
        return EXIT_FAILURE;
    }

    write_csv_header(csv);

    double start_seq = omp_get_wtime();
    gaussian_filter_sequential(input, output_seq);
    double end_seq = omp_get_wtime();
    double seq_runtime = end_seq - start_seq;

    printf("\nSequential Gaussian filter finished.\n");
    printf("Sequential runtime: %.6f seconds\n", seq_runtime);

    write_csv_row(
        csv,
        "gaussian",
        "sequential",
        input,
        3,
        1,
        "none",
        seq_runtime,
        1.0,
        1.0
    );

    int thread_counts[] = {1, 2, 4, 8, 16};
    int number_of_tests = (int)(sizeof(thread_counts) / sizeof(thread_counts[0]));

    printf("\nOpenMP benchmark:\n");
    printf("Threads,Runtime,Speedup,Efficiency\n");

    /* Warm-up run: initializes the OpenMP runtime before measurement. */
    omp_set_num_threads(2);
    gaussian_filter_openmp(input, output_omp);

    for (int i = 0; i < number_of_tests; i++) {
        int threads = thread_counts[i];
        omp_set_num_threads(threads);

        double start_omp = omp_get_wtime();
        gaussian_filter_openmp(input, output_omp);
        double end_omp = omp_get_wtime();

        double omp_runtime = end_omp - start_omp;
        double speedup = seq_runtime / omp_runtime;
        double efficiency = speedup / (double)threads;

        printf("%d,%.6f,%.3f,%.3f\n",
               threads,
               omp_runtime,
               speedup,
               efficiency);

        write_csv_row(
            csv,
            "gaussian",
            "openmp",
            input,
            3,
            threads,
            "static",
            omp_runtime,
            speedup,
            efficiency
        );
    }

    if (images_are_equal(output_seq, output_omp)) {
        printf("\nCorrectness check: sequential and OpenMP outputs are equal.\n");
    } else {
        printf("\nCorrectness check: outputs are different.\n");
    }

    write_tiff(output_seq_path, output_seq, input_path);
    write_tiff(output_omp_path, output_omp, input_path);

    printf("Sequential output saved to: %s\n", output_seq_path);
    printf("OpenMP output saved to: %s\n", output_omp_path);

    fclose(csv);
    printf("Benchmark results saved to: %s\n", csv_path);

    free_image(input);
    free_image(output_seq);
    free_image(output_omp);

    return EXIT_SUCCESS;
}