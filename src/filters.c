#include <omp.h>

#include "filters.h"

static void copy_borders(Image input, Image output) {
    int width = input.width;
    int height = input.height;

    for (int x = 0; x < width; x++) {
        output.data[x] = input.data[x];
        output.data[(height - 1) * width + x] =
            input.data[(height - 1) * width + x];
    }

    for (int y = 0; y < height; y++) {
        output.data[y * width] = input.data[y * width];
        output.data[y * width + width - 1] =
            input.data[y * width + width - 1];
    }
}

void gaussian_filter_sequential(Image input, Image output) {
    int width = input.width;
    int height = input.height;

    const int kernel[3][3] = {
        {1, 2, 1},
        {2, 4, 2},
        {1, 2, 1}
    };

    const float kernel_sum = 16.0f;

    copy_borders(input, output);

    for (int y = 1; y < height - 1; y++) {
        for (int x = 1; x < width - 1; x++) {
            float sum = 0.0f;

            for (int ky = -1; ky <= 1; ky++) {
                for (int kx = -1; kx <= 1; kx++) {
                    float pixel = input.data[(y + ky) * width + (x + kx)];
                    int weight = kernel[ky + 1][kx + 1];
                    sum += pixel * (float)weight;
                }
            }

            output.data[y * width + x] = sum / kernel_sum;
        }
    }
}

void gaussian_filter_openmp(Image input, Image output) {
    int width = input.width;
    int height = input.height;

    const int kernel[3][3] = {
        {1, 2, 1},
        {2, 4, 2},
        {1, 2, 1}
    };

    const float kernel_sum = 16.0f;

    copy_borders(input, output);

    #pragma omp parallel for schedule(static)
    for (int y = 1; y < height - 1; y++) {
        for (int x = 1; x < width - 1; x++) {
            float sum = 0.0f;

            for (int ky = -1; ky <= 1; ky++) {
                for (int kx = -1; kx <= 1; kx++) {
                    float pixel = input.data[(y + ky) * width + (x + kx)];
                    int weight = kernel[ky + 1][kx + 1];
                    sum += pixel * (float)weight;
                }
            }

            output.data[y * width + x] = sum / kernel_sum;
        }
    }
}