from pathlib import Path

import imageio.v3 as iio
import numpy as np


def prepare_tiff(input_path: Path, output_path: Path) -> None:
    """
    Loads a Sentinel-1 TIFF image, converts it to an 8-bit grayscale image,
    and saves it as a binary PGM file.

    Input:
        data/sentinel_input.tiff

    Output:
        data/input.pgm
    """

    print(f"Loading TIFF image from: {input_path}")

    image = iio.imread(input_path)

    print("Original image information:")
    print(f"  shape: {image.shape}")
    print(f"  dtype: {image.dtype}")
    print(f"  min:   {np.min(image)}")
    print(f"  max:   {np.max(image)}")

    # If the TIFF has multiple bands/channels, use the first one.
    # For many Sentinel-1 images, the image is already 2D.
    if image.ndim > 2:
        print("Image has multiple channels/bands. Using the first channel.")
        image = image[:, :, 0]

    # Convert to float for safe normalization
    image = image.astype(np.float32)

    # Remove negative values / shift minimum to 0
    image = image - np.min(image)

    # Normalize to range 0.0 - 1.0
    max_value = np.max(image)
    if max_value > 0:
        image = image / max_value

    # Convert to 8-bit grayscale range 0 - 255
    image = image * 255.0
    image = image.astype(np.uint8)

    print("Prepared image information:")
    print(f"  shape: {image.shape}")
    print(f"  dtype: {image.dtype}")
    print(f"  min:   {np.min(image)}")
    print(f"  max:   {np.max(image)}")

    # Make sure output folder exists
    output_path.parent.mkdir(parents=True, exist_ok=True)

    # Save as binary PGM
    iio.imwrite(output_path, image)

    print(f"Saved prepared PGM image to: {output_path}")


def main() -> None:
    input_path = Path("data\s1c-iw-grd-vv-20260710t062658-20260710t062723-008477-010c75-001.tiff")
    output_path = Path("output/input.pgm")

    if not input_path.exists():
        print(f"Error: Input file does not exist: {input_path}")
        print("Put your Sentinel-1 TIFF file into the data folder and name it:")
        print("  sentinel_input.tiff")
        return

    prepare_tiff(input_path, output_path)


if __name__ == "__main__":
    main()