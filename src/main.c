#include <stdio.h>
#include <stdlib.h>
#include <omp.h>

#include "image_io.h"
#include "filters.h"
#include "benchmark.h"

/*
    Main program.

    Current pipeline:

        1. Load prepared Sentinel-1 image from output/input.pgm
        2. Run sequential Gaussian filter
        3. Save output/gaussian_seq.pgm
        4. Run OpenMP Gaussian filter with different thread counts
        5. Save output/gaussian_omp.pgm
        6. Write benchmark results to CSV
        7. Check that sequential and OpenMP output images are equal
*/
int main() {
    const char *input_path = "output/input.pgm";
    const char *output_seq_path = "output/gaussian_seq.pgm";
    const char *output_omp_path = "output/gaussian_omp.pgm";
    const char *csv_path = "results/benchmark_results.csv";

    /*
        Load input image.

        This input.pgm was created earlier from the Sentinel-1 TIFF
        using the Python preparation script.
    */
    Image input = read_pgm(input_path);

    /*
        Create output images.

        We keep separate outputs so that the sequential result and
        OpenMP result can be compared.
    */
    Image output_seq = create_empty_image(input.width, input.height);
    Image output_omp = create_empty_image(input.width, input.height);

    printf("Loaded image: %d x %d\n", input.width, input.height);

    /*
        Open CSV file for benchmark results.
    */
    FILE *csv = fopen(csv_path, "w");

    if (!csv) {
        fprintf(stderr, "Error: Could not create CSV file %s\n", csv_path);

        free_image(input);
        free_image(output_seq);
        free_image(output_omp);

        return 1;
    }

    write_csv_header(csv);

    /*
        Sequential Gaussian baseline.

        This run is used as the reference for speedup calculation.
    */
    double start_seq = omp_get_wtime();

    gaussian_filter_sequential(input, output_seq);

    double end_seq = omp_get_wtime();
    double seq_runtime = end_seq - start_seq;

    write_pgm(output_seq_path, output_seq);

    printf("\nSequential Gaussian filter finished.\n");
    printf("Sequential runtime: %.6f seconds\n", seq_runtime);
    printf("Sequential output saved to: %s\n", output_seq_path);

    write_csv_row(
        csv,
        "gaussian",
        "sequential",
        input,
        3,
        1,
        "none",
        seq_runtime,
        1.000,
        1.000
    );

    /*
        OpenMP benchmark.

        We test multiple thread counts with static scheduling.
        Later, this can be extended to dynamic and guided scheduling.
    */
    int thread_counts[] = {1, 2, 4, 8, 16};
    int number_of_tests = 5;

    /*
        Since gaussian_filter_openmp() uses schedule(runtime),
        we choose the actual scheduling strategy here.
    */
    //omp_set_schedule(omp_sched_static, 0);

    printf("\nOpenMP benchmark:\n");
    printf("Threads,Runtime,Speedup,Efficiency\n");

    for (int i = 0; i < number_of_tests; i++) {
        int threads = thread_counts[i];

        omp_set_num_threads(threads);

        double start_omp = omp_get_wtime();

        gaussian_filter_openmp(input, output_omp);

        double end_omp = omp_get_wtime();
        double omp_runtime = end_omp - start_omp;

        double speedup = seq_runtime / omp_runtime;
        double efficiency = speedup / threads;

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

    /*
        Save OpenMP output image.

        The output image is overwritten during every benchmark run,
        but the result should be identical for every thread count.
    */
    write_pgm(output_omp_path, output_omp);

    printf("\nOpenMP output saved to: %s\n", output_omp_path);

    /*
        Correctness check.

        Sequential and OpenMP output should be exactly equal.
    */
    if (images_are_equal(output_seq, output_omp)) {
        printf("Correctness check: sequential and OpenMP outputs are equal.\n");
    } else {
        printf("Correctness check: outputs are different.\n");
    }

    fclose(csv);

    printf("Benchmark results saved to: %s\n", csv_path);

    /*
        Free allocated memory.
    */
    free_image(input);
    free_image(output_seq);
    free_image(output_omp);

    return 0;
}