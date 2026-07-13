# Ausführung unter Linux

Diese Anleitung beschreibt, wie die C/OpenMP-Anwendung aus dem Projekt **„Parallele Bildverarbeitung für Sentinel-1 SAR-Bilder mit OpenMP"** unter Linux kompiliert und ausgeführt wird. Das Projekt lässt sich ohne Code-Änderungen übersetzen; benötigt werden lediglich GCC (mit OpenMP-Unterstützung, bei allen gängigen Distributionen enthalten) und die GDAL-Entwicklungsbibliothek.

Allgemeine Informationen zum Projekt, zu den Benchmarks und zum Datensatz stehen im Haupt-[README](README.md).

## 1. Abhängigkeiten installieren

Ubuntu / Debian:

```bash
sudo apt update
sudo apt install build-essential libgdal-dev gdal-bin
```

Fedora:

```bash
sudo dnf install gcc gdal gdal-devel
```

Für die Python-Hilfsskripte (Plots und Vergleichsbilder):

```bash
sudo apt install python3 python3-pip   # Ubuntu/Debian
pip3 install numpy matplotlib imageio pandas
```

## 2. Datensatz vorbereiten

Die Sentinel-1-TIFF-Datei ist nicht im Repository enthalten. Die Schritte zum Herunterladen und Vorbereiten sind in [`data/README.md`](data/README.md) beschrieben. Die Datei wird anschließend unter `data/` abgelegt.

## 3. Kompilieren

Die Pfade zu den GDAL-Headern und -Bibliotheken liefert `gdal-config` automatisch:

```bash
gcc -O2 -Wall -Wextra -fopenmp \
    src/main.c src/image_io.c src/filters.c src/benchmark.c \
    -o main $(gdal-config --cflags) $(gdal-config --libs) -lm
```

## 4. Ausführen

```bash
./main "data/sentinel_datei.tiff"
```

Die Thread-Anzahlen für die einzelnen Benchmarks (1, 2, 4, 8, 16) werden vom Programm selbst gesetzt und müssen nicht konfiguriert werden.

Das Programm erzeugt unter anderem:

```text
output/gaussian_seq.tif
output/gaussian_omp.tif
output/lee_seq.tif
output/lee_omp.tif
results/benchmark_results.csv
```

Die Ausgabedateien sind identisch zur Windows-Version.

## 5. Plots und Vergleichsbilder erzeugen

```bash
python3 scripts/plot_results.py
python3 scripts/outputImage.py
```
