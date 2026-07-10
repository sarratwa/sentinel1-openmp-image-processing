from pathlib import Path

import imageio.v3 as iio
import matplotlib.pyplot as plt
import numpy as np
from PIL import Image


Image.MAX_IMAGE_PIXELS = None


def find_original_tiff(data_dir: Path) -> Path:
    """
    Finds a TIFF file inside the data directory.

    This avoids having to type the very long Sentinel-1 filename
    directly into this script.
    """
    candidates = sorted(
        list(data_dir.glob("*.tif"))
        + list(data_dir.glob("*.tiff"))
        + list(data_dir.glob("*.TIF"))
        + list(data_dir.glob("*.TIFF"))
    )

    if not candidates:
        raise FileNotFoundError(
            f"No TIFF file found inside: {data_dir}"
        )

    if len(candidates) > 1:
        print("Multiple TIFF files found:")

        for index, path in enumerate(candidates, start=1):
            print(f"  {index}: {path}")

        print(f"\nUsing the first TIFF: {candidates[0]}")

    return candidates[0]


def read_image(path: Path) -> np.ndarray:
    if not path.exists():
        raise FileNotFoundError(f"File not found: {path}")

    print(f"Reading: {path}")

    image = iio.imread(path)

    # Some TIFF readers may return a shape such as (1, height, width).
    if image.ndim == 3 and image.shape[0] == 1:
        image = image[0]

    # If a multi-band image is returned, use its first raster band.
    elif image.ndim == 3:
        image = image[..., 0]

    if image.ndim != 2:
        raise ValueError(
            f"Expected a two-dimensional grayscale raster, "
            f"but received shape {image.shape} from {path}"
        )

    return image


def find_interesting_crop_position(
    image: np.ndarray,
    crop_size: int = 2048,
    step: int = 1024,
) -> tuple[int, int, int, int]:
    """
    Finds a crop containing visible texture and contrast.

    The crop position is calculated only from the original image.
    The exact same coordinates are then used for the filtered image.
    """
    height, width = image.shape

    crop_height = min(crop_size, height)
    crop_width = min(crop_size, width)

    max_y = max(0, height - crop_height)
    max_x = max(0, width - crop_width)

    y_positions = list(range(0, max_y + 1, step))
    x_positions = list(range(0, max_x + 1, step))

    # Ensure the final edge is checked as well.
    if y_positions[-1] != max_y:
        y_positions.append(max_y)

    if x_positions[-1] != max_x:
        x_positions.append(max_x)

    best_score = float("-inf")
    best_x = 0
    best_y = 0

    for y in y_positions:
        for x in x_positions:
            crop = image[
                y:y + crop_height,
                x:x + crop_width,
            ].astype(np.float32)

            finite_pixels = crop[np.isfinite(crop)]
            nonzero = finite_pixels[finite_pixels > 0]

            nonzero_ratio = nonzero.size / crop.size

            # Ignore nearly empty areas.
            if nonzero_ratio < 0.05:
                continue

            p50 = np.percentile(nonzero, 50)
            p95 = np.percentile(nonzero, 95)
            p99 = np.percentile(nonzero, 99)
            standard_deviation = np.std(nonzero)

            contrast_score = p95 - p50
            texture_score = standard_deviation

            score = (
                contrast_score
                + texture_score
                + 0.1 * p99
            )

            if score > best_score:
                best_score = score
                best_x = x
                best_y = y

    print("\nSelected crop:")
    print(f"  Full image size: {width} x {height}")
    print(f"  Crop size:       {crop_width} x {crop_height}")
    print(f"  X range:         {best_x} to {best_x + crop_width}")
    print(f"  Y range:         {best_y} to {best_y + crop_height}")
    print(f"  Crop score:      {best_score:.3f}")

    return best_x, best_y, crop_width, crop_height


def crop_image(
    image: np.ndarray,
    x: int,
    y: int,
    crop_width: int,
    crop_height: int,
) -> np.ndarray:
    return image[
        y:y + crop_height,
        x:x + crop_width,
    ]


def calculate_display_range(
    original_crop: np.ndarray,
) -> tuple[float, float]:
    """
    Calculates one display range from the original crop.

    The same range is applied to both original and Gaussian output.
    This makes their visual comparison fair.
    """
    original_float = original_crop.astype(np.float32)

    valid_pixels = original_float[
        np.isfinite(original_float) & (original_float > 0)
    ]

    if valid_pixels.size == 0:
        valid_pixels = original_float[np.isfinite(original_float)]

    if valid_pixels.size == 0:
        return 0.0, 1.0

    low, high = np.percentile(valid_pixels, [1, 99])

    if high <= low:
        high = low + 1.0

    return float(low), float(high)


def stretch_for_display(
    image: np.ndarray,
    low: float,
    high: float,
) -> np.ndarray:
    """
    Converts a raw TIFF crop into an 8-bit preview image.

    This changes only the PNG visualization. It does not modify
    the original TIFF or the filtered TIFF.
    """
    image_float = image.astype(np.float32)

    image_float = np.nan_to_num(
        image_float,
        nan=low,
        posinf=high,
        neginf=low,
    )

    normalized = (image_float - low) / (high - low)
    normalized = np.clip(normalized, 0.0, 1.0)

    return (normalized * 255.0).astype(np.uint8)


def save_png(path: Path, image: np.ndarray) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    iio.imwrite(path, image)
    print(f"Saved: {path}")


def save_comparison(
    path: Path,
    original: np.ndarray,
    gaussian: np.ndarray,
) -> None:
    """
    Creates one side-by-side image for documentation or presentation.
    """
    path.parent.mkdir(parents=True, exist_ok=True)

    figure, axes = plt.subplots(1, 2, figsize=(12, 6))

    axes[0].imshow(original, cmap="gray", vmin=0, vmax=255)
    axes[0].set_title("Original Sentinel-1 TIFF")
    axes[0].axis("off")

    axes[1].imshow(gaussian, cmap="gray", vmin=0, vmax=255)
    axes[1].set_title("After Gaussian Filter")
    axes[1].axis("off")

    figure.tight_layout()
    figure.savefig(path, dpi=300, bbox_inches="tight")
    plt.close(figure)

    print(f"Saved: {path}")


def print_statistics(
    name: str,
    image: np.ndarray,
) -> None:
    finite_pixels = image[np.isfinite(image)]

    print(f"\n{name}:")
    print(f"  Shape: {image.shape}")
    print(f"  Type:  {image.dtype}")

    if finite_pixels.size == 0:
        print("  No finite pixel values found.")
        return

    print(f"  Min:   {finite_pixels.min()}")
    print(f"  Max:   {finite_pixels.max()}")
    print(f"  Mean:  {finite_pixels.mean():.3f}")
    print(
        "  Percentiles:",
        np.percentile(
            finite_pixels,
            [0, 50, 90, 95, 99, 99.5, 100],
        ),
    )


def main() -> None:
    data_dir = Path("data")
    output_dir = Path("output")
    results_dir = Path("results")

    original_tiff = find_original_tiff(data_dir)
    gaussian_tiff = output_dir / "gaussian_seq.tif"

    crop_size = 2048
    step = 1024

    original = read_image(original_tiff)
    gaussian = read_image(gaussian_tiff)

    if original.shape != gaussian.shape:
        raise ValueError(
            "Original and filtered images have different dimensions:\n"
            f"  Original: {original.shape}\n"
            f"  Gaussian: {gaussian.shape}"
        )

    print_statistics("Original full image", original)
    print_statistics("Gaussian full image", gaussian)

    crop_x, crop_y, crop_width, crop_height = (
        find_interesting_crop_position(
            original,
            crop_size=crop_size,
            step=step,
        )
    )

    original_crop_raw = crop_image(
        original,
        crop_x,
        crop_y,
        crop_width,
        crop_height,
    )

    gaussian_crop_raw = crop_image(
        gaussian,
        crop_x,
        crop_y,
        crop_width,
        crop_height,
    )

    print_statistics("Original raw crop", original_crop_raw)
    print_statistics("Gaussian raw crop", gaussian_crop_raw)

    # Use the same brightness range for both previews.
    low, high = calculate_display_range(original_crop_raw)

    print("\nVisualization range:")
    print(f"  Low percentile value:  {low:.3f}")
    print(f"  High percentile value: {high:.3f}")

    original_crop_display = stretch_for_display(
        original_crop_raw,
        low,
        high,
    )

    gaussian_crop_display = stretch_for_display(
        gaussian_crop_raw,
        low,
        high,
    )

    save_png(
        results_dir / "original_crop.png",
        original_crop_display,
    )

    save_png(
        results_dir / "gaussian_crop.png",
        gaussian_crop_display,
    )

    save_comparison(
        results_dir / "before_after_gaussian.png",
        original_crop_display,
        gaussian_crop_display,
    )

    print("\nVisualization completed successfully.")


if __name__ == "__main__":
    main()