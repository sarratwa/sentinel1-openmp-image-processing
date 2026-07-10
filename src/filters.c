#include <omp.h>

#include "filters.h"

/*
    Sequential Gaussian filter.

    This is the baseline version.
    It uses one thread and no OpenMP parallelization.

    Kernel:

        1 2 1
        2 4 2
        1 2 1

    Kernel sum = 16
*/
void gaussian_filter_sequential(Image input, Image output) {
    int width = input.width;
    int height = input.height;

    int kernel[3][3] = {
        {1, 2, 1},
        {2, 4, 2},
        {1, 2, 1}
    };

    int kernel_sum = 16;

    // Copy top and bottom border unchanged
    for (int x = 0; x < width; x++) {
        output.data[x] = input.data[x];
        output.data[(height - 1) * width + x] =
            input.data[(height - 1) * width + x];
    }

    // Copy left and right border unchanged
    for (int y = 0; y < height; y++) {
        output.data[y * width] = input.data[y * width];
        output.data[y * width + width - 1] =
            input.data[y * width + width - 1];
    }

    // Process all inner pixels sequentially
    for (int y = 1; y < height - 1; y++) {
        for (int x = 1; x < width - 1; x++) {
            int sum = 0;

            for (int ky = -1; ky <= 1; ky++) {
                for (int kx = -1; kx <= 1; kx++) {
                    int pixel = input.data[(y + ky) * width + (x + kx)];
                    int weight = kernel[ky + 1][kx + 1];

                    sum += pixel * weight;
                }
            }

            output.data[y * width + x] =
                (unsigned char)(sum / kernel_sum);
        }
    }
}

/*
    Parallel Gaussian filter with OpenMP.

    This function does the same calculation as the sequential version,
    but the outer image loop is parallelized.

    IMPORTANT:
    schedule(runtime) means the scheduling strategy is selected in main()
    using omp_set_schedule().

    This allows us to compare:
        static
        dynamic
        guided

    without writing three separate filter functions.
*/
void gaussian_filter_openmp(Image input, Image output) {
    int width = input.width;
    int height = input.height;

    int kernel[3][3] = {
        {1, 2, 1},
        {2, 4, 2},
        {1, 2, 1}
    };

    int kernel_sum = 16;

    // Copy top and bottom border unchanged
    for (int x = 0; x < width; x++) {
        output.data[x] = input.data[x];
        output.data[(height - 1) * width + x] =
            input.data[(height - 1) * width + x];
    }

    // Copy left and right border unchanged
    for (int y = 0; y < height; y++) {
        output.data[y * width] = input.data[y * width];
        output.data[y * width + width - 1] =
            input.data[y * width + width - 1];
    }

    /*
        OpenMP parallelization.

        Each thread processes different image rows.
        The input image is only read.
        The output image is written at separate pixel positions.
    */
    #pragma omp parallel for schedule(static)
    for (int y = 1; y < height - 1; y++) {
        for (int x = 1; x < width - 1; x++) {
            int sum = 0;

            for (int ky = -1; ky <= 1; ky++) {
                for (int kx = -1; kx <= 1; kx++) {
                    int pixel = input.data[(y + ky) * width + (x + kx)];
                    int weight = kernel[ky + 1][kx + 1];

                    sum += pixel * weight;
                }
            }

            output.data[y * width + x] =
                (unsigned char)(sum / kernel_sum);
        }
    }
}