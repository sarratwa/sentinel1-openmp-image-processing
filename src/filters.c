#include <omp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "filters.h"

/*
    Computes 1D binomial (Pascal's triangle) coefficients for row n.
    Row n has n+1 coefficients: C(n,0), C(n,1), ..., C(n,n).
*/
static void binomial_row(int n, int *out) {
    out[0] = 1;

    for (int k = 1; k <= n; k++) {
        /* out[k] = out[k-1] * (n - k + 1) / k, computed iteratively
           to avoid overflow from computing full factorials. */
        out[k] = (int)((long long)out[k - 1] * (n - k + 1) / k);
    }
}

Kernel generate_binomial_kernel(int size) {
    if (size < 1 || size % 2 == 0) {
        fprintf(stderr, "Error: kernel size must be a positive odd number (got %d).\n", size);
        exit(EXIT_FAILURE);
    }

    int row_degree = size - 1;

    int *row = malloc((size_t)size * sizeof(int));

    if (!row) {
        fprintf(stderr, "Error: could not allocate binomial row.\n");
        exit(EXIT_FAILURE);
    }

    binomial_row(row_degree, row);

    int *weights = malloc((size_t)size * (size_t)size * sizeof(int));

    if (!weights) {
        fprintf(stderr, "Error: could not allocate kernel weights.\n");
        free(row);
        exit(EXIT_FAILURE);
    }

    float row_sum = 0.0f;

    for (int i = 0; i < size; i++) {
        row_sum += (float)row[i];
    }

    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            weights[y * size + x] = row[y] * row[x];
        }
    }

    free(row);

    Kernel kernel;
    kernel.size = size;
    kernel.sum = row_sum * row_sum;
    kernel.weights = weights;

    return kernel;
}

void free_kernel(Kernel kernel) {
    free(kernel.weights);
}

/*
    Sequential Gaussian filter, generalized to any odd kernel size.

    The entire input is first copied to output (this correctly handles
    borders of any width: border_width = kernel.size / 2). Interior
    pixels are then overwritten with the convolved result.
*/
void gaussian_filter_sequential(Image input, Image output, Kernel kernel) {
    int width = input.width;
    int height = input.height;
    int radius = kernel.size / 2;

    memcpy(output.data, input.data, (size_t)width * (size_t)height * sizeof(float));

    for (int y = radius; y < height - radius; y++) {
        for (int x = radius; x < width - radius; x++) {
            float sum = 0.0f;

            for (int ky = -radius; ky <= radius; ky++) {
                for (int kx = -radius; kx <= radius; kx++) {
                    float pixel = input.data[(y + ky) * width + (x + kx)];
                    int weight = kernel.weights[(ky + radius) * kernel.size + (kx + radius)];
                    sum += pixel * (float)weight;
                }
            }

            output.data[y * width + x] = sum / kernel.sum;
        }
    }
}

/*
    Parallel Gaussian filter with OpenMP, generalized to any odd kernel size.

    schedule(runtime) defers the scheduling decision to whatever was set
    via omp_set_schedule() in main() before this function is called.
*/
void gaussian_filter_openmp(Image input, Image output, Kernel kernel) {
    int width = input.width;
    int height = input.height;
    int radius = kernel.size / 2;

    memcpy(output.data, input.data, (size_t)width * (size_t)height * sizeof(float));

    #pragma omp parallel for schedule(runtime)
    for (int y = radius; y < height - radius; y++) {
        for (int x = radius; x < width - radius; x++) {
            float sum = 0.0f;

            for (int ky = -radius; ky <= radius; ky++) {
                for (int kx = -radius; kx <= radius; kx++) {
                    float pixel = input.data[(y + ky) * width + (x + kx)];
                    int weight = kernel.weights[(ky + radius) * kernel.size + (kx + radius)];
                    sum += pixel * (float)weight;
                }
            }

            output.data[y * width + x] = sum / kernel.sum;
        }
    }
}

/*
    Computes the Lee-filtered value for a single pixel from its local
    window statistics. Shared by the sequential and OpenMP versions so
    the adaptive-weight formula only needs to be correct in one place.

    Lee filter formula:
        Ci^2 = local variance / local mean^2   (local coefficient of variation squared)
        Cu^2 = assumed noise variance (from ENL)
        weight = 0                              if Ci^2 <= Cu^2 (looks like pure noise -> smooth fully)
        weight = 1 - Cu^2 / Ci^2                otherwise        (looks like real structure -> preserve it)
        output = mean + weight * (center_pixel - mean)
*/
static inline float lee_filter_pixel(
    float center_pixel,
    float mean,
    float variance,
    float noise_variance
) {
    if (variance < 0.0f) {
        variance = 0.0f;
    }

    float ci_squared;

    if (mean != 0.0f) {
        ci_squared = variance / (mean * mean);
    } else {
        ci_squared = 0.0f;
    }

    float weight;

    if (ci_squared > noise_variance) {
        weight = 1.0f - (noise_variance / ci_squared);
    } else {
        weight = 0.0f;
    }

    return mean + weight * (center_pixel - mean);
}

void lee_filter_sequential(Image input, Image output, LeeFilterParams params) {
    int width = input.width;
    int height = input.height;
    int radius = params.window_size / 2;
    int window_pixel_count = params.window_size * params.window_size;

    memcpy(output.data, input.data, (size_t)width * (size_t)height * sizeof(float));

    for (int y = radius; y < height - radius; y++) {
        for (int x = radius; x < width - radius; x++) {
            float sum = 0.0f;
            float sum_sq = 0.0f;

            for (int wy = -radius; wy <= radius; wy++) {
                for (int wx = -radius; wx <= radius; wx++) {
                    float pixel = input.data[(y + wy) * width + (x + wx)];
                    sum += pixel;
                    sum_sq += pixel * pixel;
                }
            }

            float mean = sum / (float)window_pixel_count;
            float variance = sum_sq / (float)window_pixel_count - mean * mean;
            float center_pixel = input.data[y * width + x];

            output.data[y * width + x] =
                lee_filter_pixel(center_pixel, mean, variance, params.noise_variance);
        }
    }
}

void lee_filter_openmp(Image input, Image output, LeeFilterParams params) {
    int width = input.width;
    int height = input.height;
    int radius = params.window_size / 2;
    int window_pixel_count = params.window_size * params.window_size;

    memcpy(output.data, input.data, (size_t)width * (size_t)height * sizeof(float));

    #pragma omp parallel for schedule(runtime)
    for (int y = radius; y < height - radius; y++) {
        for (int x = radius; x < width - radius; x++) {
            float sum = 0.0f;
            float sum_sq = 0.0f;

            for (int wy = -radius; wy <= radius; wy++) {
                for (int wx = -radius; wx <= radius; wx++) {
                    float pixel = input.data[(y + wy) * width + (x + wx)];
                    sum += pixel;
                    sum_sq += pixel * pixel;
                }
            }

            float mean = sum / (float)window_pixel_count;
            float variance = sum_sq / (float)window_pixel_count - mean * mean;
            float center_pixel = input.data[y * width + x];

            output.data[y * width + x] =
                lee_filter_pixel(center_pixel, mean, variance, params.noise_variance);
        }
    }
}