#include <gdal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "image_io.h"

// safety helper : we need to reject invalid image dimension
static size_t checked_pixel_count(int width, int height) {
    if (width <= 0 || height <= 0) {
        fprintf(stderr, "Error: Invalid image dimensions %d x %d.\n", width, height);
        exit(EXIT_FAILURE);
    }

    // https://www.geeksforgeeks.org/c/size_t-data-type-c-language/
    // https://en.c.perlzemi.com/blog/20210228091654.html#:~:text=SIZE_MAX%20is%20a%20constant%20macro,a%2064%2Dbit%20Linux%20environment.
    // we need to check if the multiplication is possible & if we can allocated one float per pixel
    size_t w = (size_t)width;
    size_t h = (size_t)height;

    if (w > SIZE_MAX / h) {
        fprintf(stderr, "Error: Image dimensions are too large.\n");
        exit(EXIT_FAILURE);
    }

    size_t pixels = w * h;

    if (pixels > SIZE_MAX / sizeof(float)) {
        fprintf(stderr, "Error: Image buffer would be too large.\n");
        exit(EXIT_FAILURE);
    }

    return pixels;
}

Image read_tiff(const char *filename) {
    if (!filename) {
        fprintf(stderr, "Error: No input filename was provided.\n");
        exit(EXIT_FAILURE);
    }

    // https://gdal.org/en/stable/tutorials/raster_driver_tut.html

    GDALAllRegister();

    // why readonly?
    GDALDatasetH dataset = GDALOpen(filename, GA_ReadOnly);

    if (!dataset) {
        fprintf(stderr, "Error: Could not open TIFF file %s\n", filename);
        exit(EXIT_FAILURE);
    }

    int width = GDALGetRasterXSize(dataset);
    int height = GDALGetRasterYSize(dataset);
    // number of image bands
    int band_count = GDALGetRasterCount(dataset);

    if (band_count < 1) {
        fprintf(stderr, "Error: TIFF file contains no raster bands.\n");
        GDALClose(dataset);
        exit(EXIT_FAILURE);
    }

    // vv measurement file
    GDALRasterBandH band = GDALGetRasterBand(dataset, 1);

    if (!band) {
        fprintf(stderr, "Error: Could not access raster band 1.\n");
        GDALClose(dataset);
        exit(EXIT_FAILURE);
    }

    size_t pixel_count = checked_pixel_count(width, height);
    float *data = malloc(pixel_count * sizeof(float));

    if (!data) {
        fprintf(stderr,
                "Error: Could not allocate %.2f MB for the input image.\n",
                (double)(pixel_count * sizeof(float)) / (1024.0 * 1024.0));
        GDALClose(dataset);
        exit(EXIT_FAILURE);
    }

    CPLErr result = GDALRasterIO(
        band,
        GF_Read,
        0,
        0,
        width,
        height,
        data,
        width,
        height,
        GDT_Float32,
        0,
        0
    );

    if (result != CE_None) {
        fprintf(stderr, "Error: Could not read raster pixels from %s\n", filename);
        free(data);
        GDALClose(dataset);
        exit(EXIT_FAILURE);
    }

    GDALDataType source_type = GDALGetRasterDataType(band);

    printf("Loaded TIFF: %s\n", filename);
    printf("Image size: %d x %d\n", width, height);
    printf("Raster bands: %d\n", band_count);
    printf("Source data type: %s\n", GDALGetDataTypeName(source_type));
    printf("Processing buffer type: Float32\n");

    GDALClose(dataset);

    Image image;
    image.width = width;
    image.height = height;
    image.data = data;

    return image;
}

// now that we have the image.data we need to write it as GeoTIFF
void write_tiff(
    const char *filename,
    Image image,
    const char *reference_filename
) {
    if (!filename || !image.data) {
        fprintf(stderr, "Error: Invalid output image or filename.\n");
        exit(EXIT_FAILURE);
    }

    checked_pixel_count(image.width, image.height);
    GDALAllRegister();

    GDALDriverH driver = GDALGetDriverByName("GTiff");

    if (!driver) {
        fprintf(stderr, "Error: GDAL GeoTIFF driver is unavailable.\n");
        exit(EXIT_FAILURE);
    }

    char *creation_options[] = {
        "COMPRESS=LZW",
        "TILED=YES",
        "BIGTIFF=IF_SAFER",
        NULL
    };

    GDALDatasetH output = GDALCreate(
        driver,
        filename,
        image.width,
        image.height,
        1,
        GDT_Float32,
        creation_options
    );

    if (!output) {
        fprintf(stderr, "Error: Could not create output TIFF %s\n", filename);
        exit(EXIT_FAILURE);
    }

    // we need to connect pixel positions to geographic coordinates by reopening og TIFF 
    // we need the geospacial info as it is a Sentinel 1 image
    if (reference_filename) {
        GDALDatasetH reference = GDALOpen(reference_filename, GA_ReadOnly);

        if (reference) {
            double geo_transform[6];

            if (GDALGetGeoTransform(reference, geo_transform) == CE_None) {
                GDALSetGeoTransform(output, geo_transform);
            }

            const char *projection = GDALGetProjectionRef(reference);

            if (projection && projection[0] != '\0') {
                GDALSetProjection(output, projection);
            }

            GDALRasterBandH reference_band = GDALGetRasterBand(reference, 1);
            GDALRasterBandH output_band = GDALGetRasterBand(output, 1);

            if (reference_band && output_band) {
                int has_nodata = 0;
                double nodata = GDALGetRasterNoDataValue(reference_band, &has_nodata);

                if (has_nodata) {
                    GDALSetRasterNoDataValue(output_band, nodata);
                }
            }

            GDALClose(reference);
        } else {
            fprintf(stderr,
                    "Warning: Could not reopen reference TIFF; output will have no copied georeferencing.\n");
        }
    }

    GDALRasterBandH output_band = GDALGetRasterBand(output, 1);

    CPLErr result = GDALRasterIO(
        output_band,
        GF_Write,
        0,
        0,
        image.width,
        image.height,
        image.data,
        image.width,
        image.height,
        GDT_Float32,
        0,
        0
    );

    if (result != CE_None) {
        fprintf(stderr, "Error: Could not write pixels to %s\n", filename);
        GDALClose(output);
        exit(EXIT_FAILURE);
    }

    GDALFlushCache(output);
    GDALClose(output);
}

// create the empty output image for the filters
Image create_empty_image(int width, int height) {
    size_t pixel_count = checked_pixel_count(width, height);

    Image image;
    image.width = width;
    image.height = height;
    // we allocate the buffer and initializes every pixel to zero (using malloc leaves the memory uninitialized -> also possible)
    image.data = calloc(pixel_count, sizeof(float));

    if (!image.data) {
        fprintf(stderr, "Error: Could not allocate output image.\n");
        exit(EXIT_FAILURE);
    }

    return image;
}

// float point comparison for sequential and parallel implementations
int images_are_equal(Image a, Image b) {
    if (a.width != b.width || a.height != b.height || !a.data || !b.data) {
        return 0;
    }

    size_t pixel_count = (size_t)a.width * (size_t)a.height;

    for (size_t i = 0; i < pixel_count; i++) {
        if (a.data[i] != b.data[i]) {
            return 0;
        }
    }

    return 1;
}

// needed as we allocated memory by calloc  
void free_image(Image image) {
    free(image.data);
}