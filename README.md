# Parallele Bildverarbeitung für Sentinel-1 SAR-Bilder mit OpenMP

Dieses Repository enthält den Code, die Dokumentation und die Benchmark-Ergebnisse für die Belegaufgabe **„Parallele Bildverarbeitung für Sentinel-1 SAR-Bilder mit OpenMP“** im Kurs **Parallel Systems** an der HTW Berlin im Sommersemester 2026.

Ziel des Projekts ist die Entwicklung und Analyse einer parallelen Bildverarbeitungspipeline für große Sentinel-1 SAR-Bilder. Der Schwerpunkt liegt auf der Umsetzung und Bewertung verschiedener OpenMP-Parallelisierungsstrategien für Filteroperationen auf Bilddaten.

Sentinel-1 SAR-Bilder enthalten typischerweise Speckle-Rauschen und können sehr groß sein. Dadurch eignen sie sich gut als realistischer Anwendungsfall für datenparallele Verarbeitung mit Shared-Memory-Parallelisierung.


Aufgaben des Projekts sind:

- a. Laden eines vorbereiteten Sentinel-1 SAR-Bildes in ein geeignetes Arbeitsformat
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

| Filter | Bildgröße | Kernel | Scheduling | Laufzeit |
| ------ | --------: | -----: | ---------- | -------: |
| Gaussian | 26562 x 16681 | 3x3 | none | 2.551 s |

### Vergleich zwischen Originalbild und gefiltertem Bild

Da das vollständige Sentinel-1-Bild sehr groß ist, wird für die visuelle Darstellung ein automatisch ausgewählter Ausschnitt mit sichtbarer Bildstruktur verwendet. Der dargestellte Ausschnitt hat eine Größe von `2048 x 2048` Pixeln und stammt aus dem Bereich `x = 0–2048`, `y = 2048–4096` des vollständigen Bildes (`26562 x 16681`). Die Benchmarks selbst werden weiterhin auf dem vollständigen Bild durchgeführt.

<table width="100%">
  <tr>
    <td width="33%"><img src="results/original_crop.png" width="100%"></td>
    <td width="33%"><img src="results/gaussian_crop.png" width="100%"></td>
    <td width="33%"><img src="results/lee_crop.png" width="100%"></td>
  </tr>
  <tr>
    <td>Original</td>
    <td>Gaussian-Filte</td>
    <td>Lee-Filter</td>
  </tr>
</table>

### OpenMP-Thread-Skalierung

<table width="100%">
  <tr>
    <td width="33%"><img src="results/runtime_vs_threads.png" width="100%"></td>
    <td width="33%"><img src="results/speedup_vs_threads.png" width="100%"></td>
    <td width="33%"><img src="results/efficiency_vs_threads.png" width="100%"></td>
  </tr>
  <tr>
    <td>Runtime vs Threads</td>
    <td>Speedup vs Threads</td>
    <td>Efficiency vs Threads</td>
  </tr>
</table>

| Filter | Bildgröße | Kernel | Threads | Scheduling | Laufzeit | Speedup | Effizienz | Speicher |
| ------ | --------: | -----: | ------: | ---------- | -------: | ------: | --------: | -------: |
| Gaussian | 26562 x 16681 | 3x3 | 1 | static | 3.154 s | 0.809 | 0.809 | 1267.66 MB |
| Gaussian | 26562 x 16681 | 3x3 | 2 | static | 1.600 s | 1.594 | 0.797 | 1267.66 MB |
| Gaussian | 26562 x 16681 | 3x3 | 4 | static | 0.931 s | 2.740 | 0.685 | 1267.66 MB |
| Gaussian | 26562 x 16681 | 3x3 | 8 | static | 0.625 s | 4.082 | 0.510 | 1267.66 MB |
| Gaussian | 26562 x 16681 | 3x3 | 16 | static | 0.488 s | 5.227 | 0.327 | 1267.66 MB |

### Bildgrößenvergleich

| Filter | Bildgröße | Threads | Scheduling | Laufzeit | Speicher |
| ------ | --------: | ------: | ---------- | -------: | -------: |
| Gaussian | 2048 x 2048 | 8 | static | offen | offen |
| Gaussian | 4096 x 4096 | 8 | static | offen | offen |
| Gaussian | 8192 x 8192 | 8 | static | offen | offen |
| Gaussian | 26562 x 16681 | 8 | static | 0.625 s | 1267.66 MB |

### Scheduling-Vergleich

Für den Scheduling-Vergleich wird eine feste Bildgröße, ein fester Kernel und eine feste Thread-Anzahl verwendet. Verglichen werden die OpenMP-Scheduling-Strategien `static`, `dynamic` und `guided`.

| Filter | Bildgröße | Kernel | Threads | Scheduling | Laufzeit |
| ------ | --------: | -----: | ------: | ---------- | -------: |
| Gaussian | 26562 x 16681 | 3x3 | 8 | static | 0.625 s |
| Gaussian | 26562 x 16681 | 3x3 | 8 | dynamic | offen |
| Gaussian | 26562 x 16681 | 3x3 | 8 | guided | offen |


## Interpretation
Die Interpretation wird nach Durchführung der Benchmarks ergänzt.

## Getting Started

### Datensatz

Die originalen Sentinel-1-Daten werden nicht im Repository gespeichert, da die Dateien sehr groß sind.

Für die Benchmarks wird ein Sentinel-1 SAR-Bild aus dem [Copernicus Browser](https://dataspace.copernicus.eu/data-collections/copernicus-sentinel-missions/sentinel-1) verwendet. 

Die Schritte zum Herunterladen und Vorbereiten der Sentinel-1-Daten sind in der Datei [`data/README.md`](data/README.md) beschrieben.

Für die aktuelle Benchmark-Version wird die VV-Polarisation eines Sentinel-1-Level-1-GRD-Produkts verwendet. Die TIFF-Messdatei wird vor der Verarbeitung in ein normalisiertes 8-bit-PGM-Graustufenbild konvertiert.

Das PGM-Bild dient nur als internes Arbeitsformat für die C/OpenMP-Implementierung. Die Pixelwerte stammen weiterhin aus der Sentinel-1-TIFF-Messdatei; lediglich das Dateiformat wird vereinfacht, damit das C-Programm ohne zusätzliche GeoTIFF-Bibliotheken arbeiten kann.

<!---
Durch die Konvertierung in ein 8-bit-Graustufenbild wird der ursprüngliche Wertebereich der Sentinel-1-Messdaten normalisiert. Die Bilddimensionen und die Pixelstruktur bleiben erhalten, jedoch nicht die vollständige radiometrische Genauigkeit der ursprünglichen TIFF-Daten. Der Schwerpunkt dieser Arbeit liegt auf der OpenMP-Parallelisierung und dem Laufzeitverhalten der Filteroperationen, nicht auf einer radiometrisch exakten SAR-Auswertung.
-->

### Dependencies

Für die Python-Hilfsskripte:
- Python 3.10+
- NumPy
- Matplotlib
- ImageIO
- Pandas

Für die C/OpenMP-Anwendung:
- GCC mit OpenMP-Unterstützung
(Unter Windows wurde GCC über MSYS2/UCRT64 verwendet)

### Installing

Repository klonen:

```bash
git clone url
cd 
```

### Daten vorbereiten

Da TIFF-Dateien in C nur mit zusätzlichen Bibliotheken wie GDAL oder libtiff direkt gelesen werden können, wird die Sentinel-1-TIFF-Datei zunächst in ein einfaches PGM-Graustufenbild konvertiert. Dieses Format kann im C/OpenMP-Programm ohne externe Bildbibliotheken eingelesen werden:


```bash
python scripts/prepare_tiff.py
```

### C/OpenMP-Programm kompilieren & ausführen

```bash
gcc -O2 -Wall -Wextra -fopenmp src/main.c src/image_io.c src/filters.c src/benchmark.c -o main.exe
./main.exe
```

Das Programm führt aktuell den sequenziellen Gaussian-Filter und die OpenMP-Version aus. Dabei werden die Ausgabebilder und Benchmark-Ergebnisse erzeugt:

```text
output/gaussian_seq.pgm
output/gaussian_omp.pgm
results/benchmark_results.csv
```

### Plots erzeugen

```bash
python scripts/plot_results.py
```

Die erzeugten Plots werden im Ordner `results/` gespeichert:

```text
results/runtime_vs_threads.png
results/speedup_vs_threads.png
results/efficiency_vs_threads.png
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
