from pathlib import Path
import struct

import imageio.v3 as iio
import numpy as np


def write_pgm16(path: Path, image: np.ndarray) -> None:
    """
    Writes a 16-bit binary PGM file (P5, maxval 65535).

    PGM spec requires big-endian byte order when maxval > 255,
    i.e. the most significant byte first for each pixel.
    This must match what the C reader expects.
    """
    if image.dtype != np.uint16:
        raise ValueError(f"Expected uint16 image, got {image.dtype}")

    height, width = image.shape

    path.parent.mkdir(parents=True, exist_ok=True)

    with open(path, "wb") as f:
        # Header: magic number, width/height, maxval
        f.write(b"P5\n")
        f.write(f"{width} {height}\n".encode("ascii"))
        f.write(b"65535\n")

        # Pixel data: big-endian uint16, matches PGM spec for maxval > 255
        big_endian_data = image.astype(">u2")
        f.write(big_endian_data.tobytes())

    print(f"Saved 16-bit PGM to: {path}")


def prepare_tiff(input_path: Path, output_path: Path) -> None:
    """
    Loads a Sentinel-1 TIFF image, converts it to a 16-bit grayscale image,
    and saves it as a binary PGM file (P5, maxval 65535).

    Preserving 16 bits instead of 8 keeps much more of the original
    dynamic range, which matters for SAR backscatter data.

    Input:
        data/sentinel_input.tiff

    Output:
        output/input.pgm
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
    image = image.astype(np.float64)

    # Remove negative values / shift minimum to 0
    image = image - np.min(image)

    # Normalize to range 0.0 - 1.0
    max_value = np.max(image)
    if max_value > 0:
        image = image / max_value

    # Convert to 16-bit grayscale range 0 - 65535
    image = image * 65535.0
    image = np.round(image).astype(np.uint16)

    print("Prepared image information:")
    print(f"  shape: {image.shape}")
    print(f"  dtype: {image.dtype}")
    print(f"  min:   {np.min(image)}")
    print(f"  max:   {np.max(image)}")

    write_pgm16(output_path, image)


def main() -> None:
    input_path = Path("data/s1c-iw-grd-vv-20260710t062658-20260710t062723-008477-010c75-001.tiff")
    output_path = Path("output/input.pgm")

    if not input_path.exists():
        print(f"Error: Input file does not exist: {input_path}")
        print("Put your Sentinel-1 TIFF file into the data folder.")
        return

    prepare_tiff(input_path, output_path)


if __name__ == "__main__":
    main()