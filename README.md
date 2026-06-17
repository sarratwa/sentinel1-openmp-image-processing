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

Es werden 3 Hauptvarianten verglichen werden:

| Variante | Name                 | Aufgabe                                   |
| -------- | ----------------------- | ------------------------------------------- |
| 1        | Baseline / CPU      |  ohne Parallelisierung       |
| 2        | OpenMP parallel for |  Parallelisierung der äußeren Bildschleife, z. B. zeilenweise Verarbeitung |
| 3        | OpenMP Scheduling-Vergleich |  Vergleich von `static`, `dynamic` und `guided` für die parallele Bildschleifez             |
| 4        | Optional: Tiling / Blocking | Blockbasierte Verarbeitung zur Untersuchung von Cache-Verhalten |

## Ergebnisse:

Die folgenden Abbildungen werden später ergänzt:

Originalbild
        ↓
Gefiltertes Bild
        ↓
Benchmark-Diagramme

### Laufzeit, Speedup und Effizienz
| Laptop | Bildgröße | Kernel | Threads | Scheduling | Laufzeit | Speedup | Effizienz |
| ------ | --------: | ------------: | -----------------: | ---------------: | ----------------: | ----------------: | ----------------: | 
| Laptop 1 | xxx | xxx | xxx | 1 | Static | xxx s | xxx | xxx |
| Laptop 1 | xxx | xxx | xxx | 2 | Static | xxx s | xxx | xxx |
| Laptop 1 | xxx | xxx | xxx | 4 | Static | xxx s | xxx | xxx |
| Laptop 1 | xxx | xxx | xxx | 8 | Static | xxx s | xxx | xxx |
| Laptop 1 | xxx | xxx | xxx | 16 | Static | xxx s | xxx | xxx |


### Scheduling-Vergleich
| Laptop | Bildgröße | Kernel | Threads | Scheduling | Laufzeit | 
| ------ | --------: | ------------: | -----------------: | ---------------: | ----------------: | 
| Laptop 1 | xxx | xxx | xxx | 8 | Static | xxx s |
| Laptop 1 | xxx | xxx | xxx | 8 | Dynamic | xxx s |
| Laptop 1 | xxx | xxx | xxx | 8 | Guided | xxx s |

## Interpretation


## Getting Started

### Datensatz
...

### Dependencies

Für die Python-Hilfsskripte:
- Python 3.10+
- NumPy
- Matplotlib

Für die C/OpenMP-Anwendung:
- GCC
- Make 

### Installing

Repository klonen:

```bash
git clone url
cd 
```
C/OpenMP-Programm kompilieren:

```bash
make
```
Beispielaufruf:

```bash
in bearbeitung
```
Plots erzeugen:

```bash
python scripts/plot_results.py
```


## Authors 
Houman Safiri HTW Berlin (M.Sc. Applied Computer Science) 
Nechirvan Haso Sodo Domili HTW Berlin (M.Sc. Applied Computer Science) 
Sarra Malek HTW Berlin (M.Sc. Applied Computer Science) 


## License  

This project is licensed under the MIT License.

## Acknowledgments

https://610yilingliu.github.io/2020/07/15/ScheduleinOpenMP/