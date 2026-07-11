#include <omp.h>
#include <stdio.h>
#include <stdlib.h>

#include "benchmark.h"
#include "filters.h"
#include "image_io.h"

#define REPETITIONS 5

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

    Kernel kernel_3x3 = generate_binomial_kernel(3);

    /* ------------------------------------------------------------
       Sequential baseline (3x3), repeated REPETITIONS times.
       min is used for all downstream speedup/efficiency math since
       it best approximates the noise-free runtime.
       ------------------------------------------------------------ */
    TimingResult seq_timing = time_filter(
        gaussian_filter_sequential, input, output_seq, kernel_3x3, REPETITIONS
    );
    double seq_runtime = seq_timing.min;

    printf("\nSequential Gaussian filter finished.\n");
    printf("Sequential runtime: min=%.6f s, mean=%.6f s, stddev=%.6f s (n=%d)\n",
           seq_timing.min, seq_timing.mean, seq_timing.stddev, REPETITIONS);

    write_csv_row(
        csv, "gaussian", "sequential", "baseline",
        input, 3, 1, "none", seq_timing, 1.0, 1.0
    );

    /* Save the standard 3x3 sequential output now, before later
       sections (different kernel sizes) overwrite this buffer. */
    write_tiff(output_seq_path, output_seq, input_path);

    /* ------------------------------------------------------------
       Thread scaling: schedule=static, threads varying.
       ------------------------------------------------------------ */
    int thread_counts[] = {1, 2, 4, 8, 16};
    int number_of_tests = (int)(sizeof(thread_counts) / sizeof(thread_counts[0]));

    printf("\nOpenMP benchmark (schedule=static, threads varying, %d reps each):\n", REPETITIONS);
    printf("Threads,RuntimeMin,RuntimeMean,RuntimeStddev,Speedup,Efficiency\n");

    omp_set_schedule(omp_sched_static, 0);

    /* Warm-up run: initializes the OpenMP runtime before measurement. */
    omp_set_num_threads(2);
    gaussian_filter_openmp(input, output_omp, kernel_3x3);

    int all_thread_counts_correct = 1;

    for (int i = 0; i < number_of_tests; i++) {
        int threads = thread_counts[i];
        omp_set_num_threads(threads);

        TimingResult t = time_filter(
            gaussian_filter_openmp, input, output_omp, kernel_3x3, REPETITIONS
        );

        double speedup = seq_runtime / t.min;
        double efficiency = speedup / (double)threads;

        printf("%d,%.6f,%.6f,%.6f,%.3f,%.3f\n",
               threads, t.min, t.mean, t.stddev, speedup, efficiency);

        write_csv_row(
            csv, "gaussian", "openmp", "thread_scaling",
            input, 3, threads, "static", t, speedup, efficiency
        );

        if (!images_are_equal(output_seq, output_omp)) {
            all_thread_counts_correct = 0;
            fprintf(stderr,
                    "Warning: output mismatch at %d threads (static schedule).\n",
                    threads);
        }
    }

    if (all_thread_counts_correct) {
        printf("\nCorrectness check: sequential and OpenMP outputs are equal for all thread counts.\n");
    } else {
        printf("\nCorrectness check: mismatches found (see warnings above).\n");
    }

    /* Save the standard 3x3 OpenMP output (last thread count run above,
       already correctness-checked) for the before/after comparison. */
    write_tiff(output_omp_path, output_omp, input_path);

    /* ------------------------------------------------------------
       Scheduling comparison: fixed threads, schedule varying.
       ------------------------------------------------------------ */
    const char *schedule_names[] = {"static", "dynamic", "guided"};
    omp_sched_t schedule_kinds[] = {omp_sched_static, omp_sched_dynamic, omp_sched_guided};
    int number_of_schedules = (int)(sizeof(schedule_names) / sizeof(schedule_names[0]));
    int schedule_comparison_threads = 8;

    printf("\nOpenMP scheduling comparison (threads=%d, %d reps each):\n",
           schedule_comparison_threads, REPETITIONS);
    printf("Schedule,RuntimeMin,RuntimeMean,RuntimeStddev,Speedup,Efficiency\n");

    omp_set_num_threads(schedule_comparison_threads);

    int all_schedules_correct = 1;

    for (int i = 0; i < number_of_schedules; i++) {
        omp_set_schedule(schedule_kinds[i], 0);

        TimingResult t = time_filter(
            gaussian_filter_openmp, input, output_omp, kernel_3x3, REPETITIONS
        );

        double speedup = seq_runtime / t.min;
        double efficiency = speedup / (double)schedule_comparison_threads;

        printf("%s,%.6f,%.6f,%.6f,%.3f,%.3f\n",
               schedule_names[i], t.min, t.mean, t.stddev, speedup, efficiency);

        write_csv_row(
            csv, "gaussian", "openmp", "schedule_comparison",
            input, 3, schedule_comparison_threads, schedule_names[i], t, speedup, efficiency
        );

        if (!images_are_equal(output_seq, output_omp)) {
            all_schedules_correct = 0;
            fprintf(stderr,
                    "Warning: output mismatch with schedule=%s.\n",
                    schedule_names[i]);
        }
    }

    if (all_schedules_correct) {
        printf("\nCorrectness check: all scheduling strategies produced equal outputs.\n");
    } else {
        printf("\nCorrectness check: mismatches found across scheduling strategies (see warnings above).\n");
    }

    free_kernel(kernel_3x3);

    /* ------------------------------------------------------------
       Kernel size comparison: fixed threads, fixed static schedule,
       kernel size varying. Each kernel size gets its own sequential
       baseline, since a bigger kernel means more work per pixel and
       speedup must be computed against a same-sized baseline.
       ------------------------------------------------------------ */
    int kernel_sizes[] = {3, 5, 7};
    int number_of_kernels = (int)(sizeof(kernel_sizes) / sizeof(kernel_sizes[0]));
    int kernel_comparison_threads = 8;

    printf("\nKernel size comparison (threads=%d, schedule=static, %d reps each):\n",
           kernel_comparison_threads, REPETITIONS);
    printf("KernelSize,SeqRuntimeMin,OmpRuntimeMin,Speedup,Efficiency\n");

    omp_set_num_threads(kernel_comparison_threads);
    omp_set_schedule(omp_sched_static, 0);

    int all_kernels_correct = 1;

    for (int i = 0; i < number_of_kernels; i++) {
        int size = kernel_sizes[i];
        Kernel kernel = generate_binomial_kernel(size);

        TimingResult seq_t = time_filter(
            gaussian_filter_sequential, input, output_seq, kernel, REPETITIONS
        );
        TimingResult omp_t = time_filter(
            gaussian_filter_openmp, input, output_omp, kernel, REPETITIONS
        );

        double speedup = seq_t.min / omp_t.min;
        double efficiency = speedup / (double)kernel_comparison_threads;

        printf("%dx%d,%.6f,%.6f,%.3f,%.3f\n",
               size, size, seq_t.min, omp_t.min, speedup, efficiency);

        write_csv_row(
            csv, "gaussian", "sequential", "kernel_scaling",
            input, size, 1, "none", seq_t, 1.0, 1.0
        );
        write_csv_row(
            csv, "gaussian", "openmp", "kernel_scaling",
            input, size, kernel_comparison_threads, "static", omp_t, speedup, efficiency
        );

        if (!images_are_equal(output_seq, output_omp)) {
            all_kernels_correct = 0;
            fprintf(stderr,
                    "Warning: output mismatch at kernel size %dx%d.\n",
                    size, size);
        }

        free_kernel(kernel);
    }

    if (all_kernels_correct) {
        printf("\nCorrectness check: all kernel sizes produced equal sequential/OpenMP outputs.\n");
    } else {
        printf("\nCorrectness check: mismatches found across kernel sizes (see warnings above).\n");
    }

    printf("\nSequential output saved to: %s\n", output_seq_path);
    printf("OpenMP output saved to: %s\n", output_omp_path);

    fclose(csv);
    printf("Benchmark results saved to: %s\n", csv_path);

    free_image(input);
    free_image(output_seq);
    free_image(output_omp);

    return EXIT_SUCCESS;
}