# Parallele Bildverarbeitung für Sentinel-1 SAR-Bilder mit OpenMP

Dieses Repository enthält den Code, die Dokumentation und die Benchmark-Ergebnisse für die Belegaufgabe **„Parallele Bildverarbeitung für Sentinel-1 SAR-Bilder mit OpenMP“** im Kurs **Parallel Systems** an der HTW Berlin im Sommersemester 2026.

Ziel des Projekts ist die Entwicklung und Analyse einer parallelen Bildverarbeitungspipeline für große Sentinel-1 SAR-Bilder. Der Schwerpunkt liegt auf der Umsetzung und Bewertung verschiedener OpenMP-Parallelisierungsstrategien für Filteroperationen auf Bilddaten.

Sentinel-1 SAR-Bilder enthalten typischerweise Speckle-Rauschen und können sehr groß sein. Dadurch eignen sie sich gut als realistischer Anwendungsfall für datenparallele Verarbeitung mit Shared-Memory-Parallelisierung.


Aufgaben des Projekts sind:

- a. Direktes Laden einer Sentinel-1-TIFF-Datei mit GDAL
- b. Implementierung eines sequenziellen Gaussian-Filters als Baseline
- c. Implementierung einer parallelen OpenMP-Version
- d. Vergleich verschiedener OpenMP-Scheduling-Strategien
- e. Benchmarking mit unterschiedlichen Bildgrößen, Kernelgrößen und Thread-Anzahlen
- f. Analyse von Laufzeit, Speedup, Effizienz, Speicherverbrauch und maximal verarbeitbarer Problemgröße
- g. Optionale Erweiterung: Implementierung eines SAR-spezifischen Lee-Filters zur Speckle-Reduktion
- h. Optionale Erweiterung: Vergleich der Laufzeit auf unterschiedlichen Hardware-Systemen

Die Hauptbenchmarks sollen auf einem festen System durchgeführt werden, damit die Ergebnisse vergleichbar bleiben. Ein zusätzlicher Vergleich auf mehreren Laptops kann optional durchgeführt werden, um den Einfluss unterschiedlicher Hardware zu zeigen.

| Laptop | Prozessor | RAM  |
| ------ | --------- | --- |
| 1 | Intel(R) Core(TM) i7-10870H CPU @ 2.20GHz, 8 Kerne, 16 logische Prozessoren | 32.0 GB |
| 2 | Apple M5, 10 Kerne | 16.0 GB |

Es werden folgende Vergleichsvarianten betrachtet:

| Variante | Name                 | Aufgabe                                   |
| -------- | ----------------------- | ------------------------------------------- |
| 1        | Sequenzielle Baseline      |  Gaussian-Bildfilter ohne OpenMP; ein Thread verarbeitet das gesamte Bild  |
| 2        | OpenMP parallel for |  Parallelisierung der äußeren Bildschleife, z. B. zeilenweise Verarbeitung |
| 3        | OpenMP Scheduling-Vergleich |  Vergleich von `static`, `dynamic` und `guided` für die parallele Bildschleife             |
| 4        | Optional: Tiling / Blocking | Blockbasierte Verarbeitung zur Untersuchung von Cache-Verhalten und Speicherzugriffen |

## Ergebnisse

<!---
Die folgenden Abbildungen werden später ergänzt:
- Vergleich zwischen Originalbild und gefiltertem Bild
- Laufzeitdiagramm in Abhängigkeit von der Thread-Anzahl
- Speedup-Diagramm
- Effizienz-Diagramm
- Laufzeitvergleich der Scheduling-Strategien
-->

### Sequenzielle Baseline

| Laptop | Filter | Bildgröße | Kernel | Laufzeit (min) | Laufzeit (mean ± stddev) |
| ------ | ------ | --------: | -----: | --------------: | ------------------------: |
| Laptop 1 | Gaussian | 26531 × 16670 | 3×3 | 3.785 s | 4.182 s ± 0.301 s |
| Laptop 1 | Lee | 26531 × 16670 | 7×7 | 22.080 s | 23.009 s ± 0.815 s |
| Laptop 2 | Gaussian | 26531 × 16670 | 3×3 | 1.226 s | 1.435 s ± 0.398 s |
| Laptop 2 | Lee | 26531 × 16670 | 7×7 | 7.237 s | 7.740 s ± 0.552 s |

*Alle Werte basieren auf 5 Wiederholungen; für Speedup/Effizienz wird jeweils die schnellste Laufzeit (min) verwendet, da Rauschen die Laufzeit nur verlängern, nie verkürzen kann.*

### Vergleich zwischen Originalbild und gefiltertem Bild

Da das vollständige verarbeitete Bild sehr groß ist, wird für die visuelle Darstellung ein automatisch ausgewählter Ausschnitt mit sichtbarer Bildstruktur verwendet. Die gezeigten Bilder stammen direkt aus der ursprünglichen Sentinel-1-VV-TIFF-Datei und aus dem vom C/OpenMP-Programm erzeugten gefilterten TIFF. Der Ausschnitt hat eine Größe von `2048 x 2048` Pixeln. Die Benchmarks selbst werden weiterhin auf dem vollständigen Bild (`26562 x 16681`) durchgeführt.

<table width="100%">
  <tr>
    <td width="33%"><img src="results/original_crop.png" width="100%"></td>
    <td width="33%"><img src="results/gaussian_crop.png" width="100%"></td>
    <td width="33%"><img src="results/lee_crop.png" width="100%"></td>
  </tr>
  <tr>
    <td>Original</td>
    <td>Gaussian-Filter</td>
    <td>Lee-Filter</td>
  </tr>
</table>

### OpenMP-Thread-Skalierung des Gaussian-Filters

Für die Thread-Skalierung wird ein fester Gaussian-Kernel von `3×3` verwendet. Die OpenMP-Version wird mit `static` Scheduling und unterschiedlichen Thread-Anzahlen ausgeführt.

<table width="100%">
  <tr>
    <td width="33%"><img src="results/Laptop1_runtime_vs_threads.png" width="100%"></td>
    <td width="33%"><img src="results/Laptop1_speedup_vs_threads.png" width="100%"></td>
    <td width="33%"><img src="results/Laptop1_efficiency_vs_threads.png" width="100%"></td>
  </tr>
  <tr>
    <td>Runtime vs Threads (Laptop 1)</td>
    <td>Speedup vs Threads (Laptop 1)</td>
    <td>Efficiency vs Threads (Laptop 1)</td>
  </tr>
  <tr>
    <td width="33%"><img src="results/Laptop2_runtime_vs_threads.png" width="100%"></td>
    <td width="33%"><img src="results/Laptop2_speedup_vs_threads.png" width="100%"></td>
    <td width="33%"><img src="results/Laptop2_efficiency_vs_threads.png" width="100%"></td>
  </tr>
  <tr>
    <td>Runtime vs Threads (Laptop 2)</td>
    <td>Speedup vs Threads (Laptop 2)</td>
    <td>Efficiency vs Threads (Laptop 2)</td>
  </tr>
</table>

| Laptop | Threads | Scheduling | Laufzeit (min) | Laufzeit (mean ± stddev) | Speedup | Effizienz |
| ------ | ------: | ---------- | --------------: | ------------------------: | ------: | --------: |
| Laptop 1 | 1 | static | 4.003 s | 4.368 s ± 0.384 s | 0.946 | 0.946 |
| Laptop 1 | 2 | static | 2.082 s | 2.180 s ± 0.084 s | 1.818 | 0.909 |
| Laptop 1 | 4 | static | 1.186 s | 1.223 s ± 0.020 s | 3.191 | 0.798 |
| Laptop 1 | 8 | static | 0.722 s | 0.784 s ± 0.032 s | 5.242 | 0.655 |
| Laptop 1 | 16 | static | 0.698 s | 0.730 s ± 0.024 s | 5.423 | 0.339 |
| Laptop 2 | 1 | static | 1.254 s | 1.734 s ± 0.956 s | 0.978 | 0.978 |
| Laptop 2 | 2 | static | 0.657 s | 0.661 s ± 0.007 s | 1.866 | 0.933 |
| Laptop 2 | 4 | static | 0.346 s | 0.353 s ± 0.010 s | 3.538 | 0.885 |
| Laptop 2 | 8 | static | 0.240 s | 0.244 s ± 0.003 s | 5.117 | 0.640 |
| Laptop 2 | 16 | static | 0.229 s | 0.235 s ± 0.005 s | 5.344 | 0.334 |

### OpenMP-Thread-Skalierung des Lee-Filters

Der Lee-Filter wird mit einem `7×7`-Fenster und `static` Scheduling ausgeführt.

| Laptop | Threads | Scheduling | Laufzeit (min) | Laufzeit (mean ± stddev) | Speedup | Effizienz |
| ------ | ------: | ---------- | --------------: | ------------------------: | ------: | --------: |
| Laptop 1 | 1 | static | 23.463 s | 25.529 s ± 1.050 s | 0.941 | 0.941 |
| Laptop 1 | 2 | static | 12.518 s | 12.622 s ± 0.104 s | 1.764 | 0.882 |
| Laptop 1 | 4 | static | 6.414 s | 6.569 s ± 0.178 s | 3.442 | 0.861 |
| Laptop 1 | 8 | static | 3.549 s | 3.609 s ± 0.038 s | 6.221 | 0.778 |
| Laptop 1 | 16 | static | 2.662 s | 2.702 s ± 0.027 s | 8.295 | 0.518 |
| Laptop 2 | 1 | static | 7.215 s | 7.291 s ± 0.112 s | 1.003 | 1.003 |
| Laptop 2 | 2 | static | 3.725 s | 3.774 s ± 0.044 s | 1.943 | 0.971 |
| Laptop 2 | 4 | static | 1.879 s | 1.901 s ± 0.013 s | 3.851 | 0.963 |
| Laptop 2 | 8 | static | 1.308 s | 1.321 s ± 0.014 s | 5.533 | 0.692 |
| Laptop 2 | 16 | static | 1.167 s | 1.194 s ± 0.025 s | 6.202 | 0.388 |

### Bildgrößenvergleich

Für den Bildgrößenvergleich werden ein `3×3`-Gaussian-Kernel, acht Threads und `static` Scheduling verwendet.

| Laptop | Bildgröße | Sequenziell (min) | OpenMP, 8 Threads (min) | Speedup | Effizienz | Speicher |
| ------ | --------: | -----------------: | ----------------------: | ------: | --------: | -------: |
| Laptop 1 | 2048 × 2048 | 0.041 s | 0.008 s | 5.125 | 0.641 | 48.0 MB |
| Laptop 1 | 4096 × 4096 | 0.191 s | 0.032 s | 5.969 | 0.746 | 192.0 MB |
| Laptop 1 | 8192 × 8192 | 0.734 s | 0.134 s | 5.478 | 0.685 | 768.0 MB |
| Laptop 1 | 26531 × 16670 | 3.843 s | 0.744 s | 5.165 | 0.646 | 5061.4 MB |
| Laptop 2 | 2048 × 2048 | 0.012 s | 0.003 s | 4.734 | 0.592 | 48.0 MB |
| Laptop 2 | 4096 × 4096 | 0.050 s | 0.010 s | 4.907 | 0.613 | 192.0 MB |
| Laptop 2 | 8192 × 8192 | 0.219 s | 0.049 s | 4.452 | 0.556 | 768.0 MB |
| Laptop 2 | 26531 × 16670 | 1.263 s | 0.253 s | 4.998 | 0.625 | 5061.4 MB |

<p align="center">
  Laptop 1
  <img src="results/Laptop1_runtime_vs_image_size.png" width="75%">
  Laptop 2
  <img src="results/Laptop2_runtime_vs_image_size.png" width="75%">
</p>

### Scheduling-Vergleich

Für den Scheduling-Vergleich wird eine feste Bildgröße, ein fester Kernel und eine feste Thread-Anzahl (8) verwendet. Verglichen werden die OpenMP-Scheduling-Strategien `static`, `dynamic` und `guided`.

| Laptop | Scheduling | Laufzeit (min) | Laufzeit (mean ± stddev) | Speedup | Effizienz |
| ------ | ---------- | --------------: | ------------------------: | ------: | --------: |
| Laptop 1 | static | 0.816 s | 0.924 s ± 0.057 s | 4.638 | 0.580 |
| Laptop 1 | dynamic | 0.764 s | 0.850 s ± 0.077 s | 4.954 | 0.619 |
| Laptop 1 | guided | 0.705 s | 0.765 s ± 0.044 s | 5.369 | 0.671 |
| Laptop 2 | static | 0.287 s | 0.373 s ± 0.071 s | 4.274 | 0.534 |
| Laptop 2 | dynamic | 0.251 s | 0.519 s ± 0.507 s | 4.881 | 0.610 |
| Laptop 2 | guided | 0.240 s | 0.243 s ± 0.005 s | 5.118 | 0.640 |

### Kernel-Größenvergleich

Für den Kernel-Größenvergleich werden das vollständige Bild, acht Threads und `static` Scheduling verwendet.

Für jede Kernelgröße wird eine eigene sequenzielle Baseline gemessen. Dadurch wird die OpenMP-Version immer mit einer sequenziellen Version verglichen, die denselben Rechenaufwand besitzt.

| Laptop | Kernel | Sequenziell (min) | OpenMP, 8 Threads (min) | Speedup | Effizienz |
| ------ | -----: | -----------------: | ----------------------: | ------: | --------: |
| Laptop 1 | 3×3 | 3.721 s | 0.845 s | 4.404 | 0.550 |
| Laptop 1 | 5×5 | 9.461 s | 1.520 s | 6.224 | 0.778 |
| Laptop 1 | 7×7 | 18.871 s | 2.913 s | 6.478 | 0.810 |
| Laptop 2 | 3×3 | 1.258 s | 0.236 s | 5.326 | 0.666 |
| Laptop 2 | 5×5 | 3.560 s | 0.553 s | 6.437 | 0.805 |
| Laptop 2 | 7×7 | 6.794 s | 1.184 s | 5.738 | 0.717 |

<p align="center">
  Laptop 1
  <img src="results/Laptop1_runtime_vs_kernel_size.png" width="70%">
  Laptop 2
  <img src="results/Laptop2_runtime_vs_kernel_size.png" width="70%">
</p>

## Interpretation
Die Interpretation wird nach Durchführung der Benchmarks ergänzt.

## Getting Started

### Datensatz

Die originalen Sentinel-1-Daten werden nicht im Repository gespeichert, da die Dateien sehr groß sind.

Für die Benchmarks wird ein Sentinel-1 SAR-Bild aus dem [Copernicus Browser](https://dataspace.copernicus.eu/data-collections/copernicus-sentinel-missions/sentinel-1) verwendet. 

Die Schritte zum Herunterladen und Vorbereiten der Sentinel-1-Daten sind in der Datei [`data/README.md`](data/README.md) beschrieben.
Für die aktuelle Benchmark-Version wird die VV-Polarisation eines Sentinel-1-Level-1-GRD-Produkts verwendet. Die TIFF-Messdatei wird direkt mit GDAL im C-Programm eingelesen.

Die ursprünglichen Pixelwerte werden von GDAL in einen Float32-Arbeitspuffer übertragen und anschließend vom sequenziellen beziehungsweise parallelen Filter verarbeitet. Eine vorherige Konvertierung in das PGM-Format ist nicht mehr notwendig.

### Dependencies

Für die Python-Hilfsskripte:
- Python 3.10+
- NumPy
- Matplotlib
- ImageIO
- Pandas

Für die C/OpenMP-Anwendung:
- GCC mit OpenMP-Unterstützung
- GDAL
- Unter Windows: MSYS2/UCRT64

### Installing

Repository klonen:

```bash
git clone <repository-url>
cd <repository-name>
```

### C/OpenMP-Programm kompilieren & ausführen

```bash
gcc -O2 -Wall -Wextra -fopenmp src/main.c src/image_io.c src/filters.c src/benchmark.c -o main.exe -lgdal -lm
./main.exe "data/sentinel_datei.tiff"
```

Das Programm erzeugt unter anderem:

```text
output/gaussian_seq.tif
output/gaussian_omp.tif
output/lee_seq.tif
output/lee_omp.tif
results/benchmark_results.csv
```

### Plots erzeugen

```bash
python scripts/plot_results.py
```

### Vergleichsbilder erzeugen

```bash
python scripts/outputImage.py
```

## Authors 
Houman Safiri HTW Berlin (M.Sc. Applied Computer Science) <br />
Nechirvan Haso Sodo Domili HTW Berlin (M.Sc. Applied Computer Science) <br />
Sarra Malek HTW Berlin (M.Sc. Applied Computer Science) 


## License  

This project is licensed under the MIT License.

## Acknowledgments

- OpenMP Scheduling overview: https://610yilingliu.github.io/2020/07/15/ScheduleinOpenMP/
- S1 Products:  https://sentiwiki.copernicus.eu/web/s1-products
- https://gdal.org/en/stable/download.html
