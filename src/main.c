#include <omp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "benchmark.h"
#include "filters.h"
#include "image_io.h"

#define REPETITIONS 5

/*
    Extracts the top-left sub_width x sub_height region of source into a
    new, contiguous Image. Used for the image-size comparison: rather
    than requiring separate pre-cropped TIFF files, sub-images are cut
    directly from the already-loaded full-resolution input in memory.

    Sizes larger than the source are clamped down to the source size
    (relevant only if the source image is smaller than a requested
    benchmark size).
*/
static Image extract_subimage(Image source, int sub_width, int sub_height) {
    int width = (sub_width < source.width) ? sub_width : source.width;
    int height = (sub_height < source.height) ? sub_height : source.height;

    Image sub = create_empty_image(width, height);

    for (int y = 0; y < height; y++) {
        memcpy(
            sub.data + (size_t)y * width,
            source.data + (size_t)y * source.width,
            (size_t)width * sizeof(float)
        );
    }

    return sub;
}

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <input.tif>\n", argv[0]);
        fprintf(stderr, "Example: %s data/sentinel_vv.tiff\n", argv[0]);
        return EXIT_FAILURE;
    }

    // starting with gaussian

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

    // baseline
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

        // output sanity check
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

    /* ------------------------------------------------------------
       Lee filter (SAR speckle filter), optional extension task.

       Unlike the Gaussian filter, this is an adaptive filter based
       on local statistics (mean/variance) rather than a fixed
       convolution kernel, so it gets its own Image outputs and its
       own thread-scaling sweep, mirroring the Gaussian sweep above.

       NOTE ON LIMITATIONS: the noise_variance parameter assumes a
       nominal ENL of 4.9 (typical for Sentinel-1 IW GRDH products),
       not a value calibrated for this specific scene. See the
       README limitations section for why exact radiometric
       calibration isn't attempted in this pipeline.
    */
    const char *lee_seq_path = "output/lee_seq.tif";
    const char *lee_omp_path = "output/lee_omp.tif";

    LeeFilterParams lee_params;
    lee_params.window_size = 7;
    lee_params.noise_variance = 1.0f / 4.9f;

    Image lee_output_seq = create_empty_image(input.width, input.height);
    Image lee_output_omp = create_empty_image(input.width, input.height);

    TimingResult lee_seq_timing = time_lee_filter(
        lee_filter_sequential, input, lee_output_seq, lee_params, REPETITIONS
    );
    double lee_seq_runtime = lee_seq_timing.min;

    printf("\nLee filter (window=%dx%d, sequential) finished.\n",
           lee_params.window_size, lee_params.window_size);
    printf("Sequential runtime: min=%.6f s, mean=%.6f s, stddev=%.6f s (n=%d)\n",
           lee_seq_timing.min, lee_seq_timing.mean, lee_seq_timing.stddev, REPETITIONS);

    write_csv_row(
        csv, "lee", "sequential", "baseline",
        input, lee_params.window_size, 1, "none", lee_seq_timing, 1.0, 1.0
    );

    write_tiff(lee_seq_path, lee_output_seq, input_path);

    printf("\nLee filter OpenMP benchmark (schedule=static, threads varying, %d reps each):\n",
           REPETITIONS);
    printf("Threads,RuntimeMin,RuntimeMean,RuntimeStddev,Speedup,Efficiency\n");

    omp_set_schedule(omp_sched_static, 0);
    omp_set_num_threads(2);
    lee_filter_openmp(input, lee_output_omp, lee_params); /* warm-up */

    int all_lee_thread_counts_correct = 1;

    for (int i = 0; i < number_of_tests; i++) {
        int threads = thread_counts[i];
        omp_set_num_threads(threads);

        TimingResult t = time_lee_filter(
            lee_filter_openmp, input, lee_output_omp, lee_params, REPETITIONS
        );

        double speedup = lee_seq_runtime / t.min;
        double efficiency = speedup / (double)threads;

        printf("%d,%.6f,%.6f,%.6f,%.3f,%.3f\n",
               threads, t.min, t.mean, t.stddev, speedup, efficiency);

        write_csv_row(
            csv, "lee", "openmp", "thread_scaling",
            input, lee_params.window_size, threads, "static", t, speedup, efficiency
        );

        if (!images_are_equal(lee_output_seq, lee_output_omp)) {
            all_lee_thread_counts_correct = 0;
            fprintf(stderr,
                    "Warning: Lee filter output mismatch at %d threads.\n",
                    threads);
        }
    }

    if (all_lee_thread_counts_correct) {
        printf("\nCorrectness check: Lee filter sequential and OpenMP outputs are equal for all thread counts.\n");
    } else {
        printf("\nCorrectness check: Lee filter mismatches found (see warnings above).\n");
    }

    write_tiff(lee_omp_path, lee_output_omp, input_path);

    printf("\nLee filter sequential output saved to: %s\n", lee_seq_path);
    printf("Lee filter OpenMP output saved to: %s\n", lee_omp_path);

    free_image(lee_output_seq);
    free_image(lee_output_omp);

    /* ------------------------------------------------------------
       Image size comparison: fixed threads, fixed 3x3 kernel,
       schedule=static, image size varying. Smaller sizes are cut
       directly from the already-loaded full input (top-left region),
       so no separate cropped TIFF files are needed. The full image
       size is included using the input already in memory.
       ------------------------------------------------------------ */
    int image_size_candidates[] = {2048, 4096, 8192};
    int number_of_size_candidates =
        (int)(sizeof(image_size_candidates) / sizeof(image_size_candidates[0]));
    int image_size_threads = 8;

    Kernel kernel_for_size_comparison = generate_binomial_kernel(3);

    printf("\nImage size comparison (threads=%d, schedule=static, kernel=3x3, %d reps each):\n",
           image_size_threads, REPETITIONS);
    printf("ImageSize,SeqRuntimeMin,OmpRuntimeMin,Speedup,Efficiency,MemoryMB\n");

    omp_set_num_threads(image_size_threads);
    omp_set_schedule(omp_sched_static, 0);

    int all_image_sizes_correct = 1;

    /* Loop runs one extra time (i == number_of_size_candidates) for the
       full image size, reusing `input` directly instead of copying it. */
    for (int i = 0; i <= number_of_size_candidates; i++) {
        Image size_input;
        int owns_size_input;

        if (i < number_of_size_candidates) {
            int size = image_size_candidates[i];
            size_input = extract_subimage(input, size, size);
            owns_size_input = 1;
        } else {
            size_input = input;
            owns_size_input = 0;
        }

        Image size_output_seq = create_empty_image(size_input.width, size_input.height);
        Image size_output_omp = create_empty_image(size_input.width, size_input.height);

        TimingResult seq_t = time_filter(
            gaussian_filter_sequential, size_input, size_output_seq,
            kernel_for_size_comparison, REPETITIONS
        );
        TimingResult omp_t = time_filter(
            gaussian_filter_openmp, size_input, size_output_omp,
            kernel_for_size_comparison, REPETITIONS
        );

        double speedup = seq_t.min / omp_t.min;
        double efficiency = speedup / (double)image_size_threads;
        double memory_mb = estimate_memory_mb(size_input, 3);

        printf("%dx%d,%.6f,%.6f,%.3f,%.3f,%.2f\n",
               size_input.width, size_input.height,
               seq_t.min, omp_t.min, speedup, efficiency, memory_mb);

        write_csv_row(
            csv, "gaussian", "sequential", "image_size_comparison",
            size_input, 3, 1, "none", seq_t, 1.0, 1.0
        );
        write_csv_row(
            csv, "gaussian", "openmp", "image_size_comparison",
            size_input, 3, image_size_threads, "static", omp_t, speedup, efficiency
        );

        if (!images_are_equal(size_output_seq, size_output_omp)) {
            all_image_sizes_correct = 0;
            fprintf(stderr,
                    "Warning: output mismatch at image size %dx%d.\n",
                    size_input.width, size_input.height);
        }

        free_image(size_output_seq);
        free_image(size_output_omp);

        if (owns_size_input) {
            free_image(size_input);
        }
    }

    if (all_image_sizes_correct) {
        printf("\nCorrectness check: all image sizes produced equal sequential/OpenMP outputs.\n");
    } else {
        printf("\nCorrectness check: mismatches found across image sizes (see warnings above).\n");
    }

    free_kernel(kernel_for_size_comparison);

    fclose(csv);
    printf("Benchmark results saved to: %s\n", csv_path);

    free_image(input);
    free_image(output_seq);
    free_image(output_omp);

    return EXIT_SUCCESS;
}
