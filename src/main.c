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

    /*
        Warm-up: touch input data once before timing the sequential
        baseline. Otherwise this run alone pays for page faults and
        cold cache/TLB state that every later OpenMP run benefits
        from for free, which inflates every downstream speedup number.
    */
    gaussian_filter_sequential(input, output_seq);

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
        "baseline",
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

    printf("\nOpenMP benchmark (schedule=static, threads varying):\n");
    printf("Threads,Runtime,Speedup,Efficiency\n");

    /*
        The filter now uses schedule(runtime), so the actual scheduling
        strategy must be selected explicitly here before each call.
        Without this, the strategy is implementation-defined and not
        guaranteed to be static.
    */
    omp_set_schedule(omp_sched_static, 0);

    /* Warm-up run: initializes the OpenMP runtime before measurement. */
    omp_set_num_threads(2);
    gaussian_filter_openmp(input, output_omp);

    int all_thread_counts_correct = 1;

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
            "thread_scaling",
            input,
            3,
            threads,
            "static",
            omp_runtime,
            speedup,
            efficiency
        );

        /*
            Correctness is checked for every thread count, not just the
            last one, since a race condition could in principle only
            show up at specific chunk boundaries for specific thread counts.
        */
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

    /*
        Scheduling comparison.

        Fixed thread count, fixed image, fixed kernel. Only the
        scheduling strategy changes between runs.
    */
    const char *schedule_names[] = {"static", "dynamic", "guided"};
    omp_sched_t schedule_kinds[] = {omp_sched_static, omp_sched_dynamic, omp_sched_guided};
    int number_of_schedules = (int)(sizeof(schedule_names) / sizeof(schedule_names[0]));
    int schedule_comparison_threads = 8;

    printf("\nOpenMP scheduling comparison (threads=%d):\n", schedule_comparison_threads);
    printf("Schedule,Runtime,Speedup,Efficiency\n");

    omp_set_num_threads(schedule_comparison_threads);

    int all_schedules_correct = 1;

    for (int i = 0; i < number_of_schedules; i++) {
        omp_set_schedule(schedule_kinds[i], 0);

        double start_sched = omp_get_wtime();
        gaussian_filter_openmp(input, output_omp);
        double end_sched = omp_get_wtime();

        double sched_runtime = end_sched - start_sched;
        double speedup = seq_runtime / sched_runtime;
        double efficiency = speedup / (double)schedule_comparison_threads;

        printf("%s,%.6f,%.3f,%.3f\n",
               schedule_names[i],
               sched_runtime,
               speedup,
               efficiency);

        write_csv_row(
            csv,
            "gaussian",
            "openmp",
            "schedule_comparison",
            input,
            3,
            schedule_comparison_threads,
            schedule_names[i],
            sched_runtime,
            speedup,
            efficiency
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