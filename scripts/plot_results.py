from pathlib import Path

import matplotlib.pyplot as plt
import pandas as pd


CSV_PATH = Path("results/benchmark_results.csv")
RESULTS_DIR = Path("results")


def save_plot(filename: str) -> None:
    """Save the current figure with a consistent layout."""
    output_path = RESULTS_DIR / filename
    plt.tight_layout()
    plt.savefig(output_path, dpi=300, bbox_inches="tight")
    plt.close()
    print(f"Saved: {output_path}")


def load_results() -> pd.DataFrame:
    if not CSV_PATH.exists():
        raise FileNotFoundError(
            f"CSV file not found: {CSV_PATH}\n"
            "Run the C benchmark first."
        )

    df = pd.read_csv(CSV_PATH)

    required_columns = {
        "filter",
        "version",
        "benchmark_type",
        "width",
        "height",
        "kernel",
        "threads",
        "runtime_min",
        "speedup",
        "efficiency",
    }

    missing = required_columns.difference(df.columns)

    if missing:
        raise ValueError(
            "CSV is missing required columns: "
            + ", ".join(sorted(missing))
        )

    return df


def get_gaussian_thread_data(df: pd.DataFrame) -> pd.DataFrame:
    """Return Gaussian OpenMP thread-scaling rows only."""
    data = df[
        (df["benchmark_type"] == "thread_scaling")
        & (df["filter"].str.lower() == "gaussian")
        & (df["version"].str.lower() == "openmp")
    ].copy()

    return data.sort_values("threads")


def plot_runtime_vs_threads(df: pd.DataFrame) -> None:
    data = get_gaussian_thread_data(df)

    if data.empty:
        print("Skipped runtime_vs_threads.png: no matching rows.")
        return

    plt.figure(figsize=(7, 4.5))
    plt.plot(data["threads"], data["runtime_min"], marker="o")
    plt.xlabel("Number of Threads")
    plt.ylabel("Runtime in Seconds")
    plt.title("OpenMP Gaussian Filter: Runtime vs Threads")
    plt.xticks(data["threads"].astype(int))
    plt.grid(True, alpha=0.35)
    save_plot("runtime_vs_threads.png")


def plot_speedup_vs_threads(df: pd.DataFrame) -> None:
    data = get_gaussian_thread_data(df)

    if data.empty:
        print("Skipped speedup_vs_threads.png: no matching rows.")
        return

    plt.figure(figsize=(7, 4.5))
    plt.plot(data["threads"], data["speedup"], marker="o")
    plt.xlabel("Number of Threads")
    plt.ylabel("Speedup")
    plt.title("OpenMP Gaussian Filter: Speedup vs Threads")
    plt.xticks(data["threads"].astype(int))
    plt.grid(True, alpha=0.35)
    save_plot("speedup_vs_threads.png")


def plot_efficiency_vs_threads(df: pd.DataFrame) -> None:
    data = get_gaussian_thread_data(df)

    if data.empty:
        print("Skipped efficiency_vs_threads.png: no matching rows.")
        return

    plt.figure(figsize=(7, 4.5))
    plt.plot(data["threads"], data["efficiency"], marker="o")
    plt.xlabel("Number of Threads")
    plt.ylabel("Efficiency")
    plt.title("OpenMP Gaussian Filter: Efficiency vs Threads")
    plt.xticks(data["threads"].astype(int))
    plt.grid(True, alpha=0.35)
    save_plot("efficiency_vs_threads.png")


def plot_runtime_vs_kernel_size(df: pd.DataFrame) -> None:
    data = df[
        (df["benchmark_type"] == "kernel_scaling")
        & (df["filter"].str.lower() == "gaussian")
    ].copy()

    if data.empty:
        print("Skipped runtime_vs_kernel_size.png: no matching rows.")
        return

    plt.figure(figsize=(7, 4.5))

    for version, label in (
        ("sequential", "Sequential"),
        ("openmp", "OpenMP"),
    ):
        group = data[
            data["version"].str.lower() == version
        ].sort_values("kernel")

        if group.empty:
            continue

        kernel_labels = (
            group["kernel"].astype(int).astype(str)
            + "x"
            + group["kernel"].astype(int).astype(str)
        )

        plt.plot(
            kernel_labels,
            group["runtime_min"],
            marker="o",
            label=label,
        )

    plt.xlabel("Kernel Size")
    plt.ylabel("Runtime in Seconds")
    plt.title("Gaussian Filter: Runtime vs Kernel Size")
    plt.grid(True, alpha=0.35)
    plt.legend()
    save_plot("runtime_vs_kernel_size.png")


def plot_runtime_vs_image_size(df: pd.DataFrame) -> None:
    data = df[
        (df["benchmark_type"] == "image_size_comparison")
        & (df["filter"].str.lower() == "gaussian")
    ].copy()

    if data.empty:
        print("Skipped runtime_vs_image_size.png: no matching rows.")
        return

    data["pixel_count"] = data["width"] * data["height"]

    plt.figure(figsize=(8, 4.8))

    for version, label in (
        ("sequential", "Sequential"),
        ("openmp", "OpenMP"),
    ):
        group = data[
            data["version"].str.lower() == version
        ].sort_values("pixel_count")

        if group.empty:
            continue

        size_labels = (
            group["width"].astype(int).astype(str)
            + "x"
            + group["height"].astype(int).astype(str)
        )

        plt.plot(
            size_labels,
            group["runtime_min"],
            marker="o",
            label=label,
        )

    plt.xlabel("Image Size")
    plt.ylabel("Runtime in Seconds")
    plt.title("Gaussian Filter: Runtime vs Image Size")
    plt.xticks(rotation=20, ha="right")
    plt.grid(True, alpha=0.35)
    plt.legend()
    save_plot("runtime_vs_image_size.png")


def main() -> None:
    RESULTS_DIR.mkdir(parents=True, exist_ok=True)

    try:
        df = load_results()
    except (FileNotFoundError, ValueError) as error:
        print(f"Error: {error}")
        return

    plot_runtime_vs_threads(df)
    plot_speedup_vs_threads(df)
    plot_efficiency_vs_threads(df)
    plot_runtime_vs_kernel_size(df)
    plot_runtime_vs_image_size(df)

    print("\nFinished. Five benchmark plots were generated.")


if __name__ == "__main__":
    main()