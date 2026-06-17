# Parallele Bildverarbeitung für Sentinel-1 SAR-Bilder mit OpenMP

Dieses Repository enthält den Code und die Ergebnisse für die Belegaufgabe **„Parallele Bildverarbeitung für Sentinel-1 SAR-Bilder mit OpenMP“** im Kurs **Parallel Systems** (HTW Berlin,  Semester 2026).

Aufgaben des Projekts sind:

- a. Laden Sie ein RGB-Bild und wandeln Sie es in ein Graustufenbild um.
- b. Passen Sie anschließend Helligkeit und Kontrast des Bildes an.
- c. Berechnen Sie zum Schluss das Histogramm der Grauwerte und stellen Sie es geeignet dar
- (d. Laufzeit und Speedup verlgleichen)

Die Aufgaben werden auf x Laptops (mit unterschiedlichen Specs ausgeführt):

| Laptop | Prozessor | GPU |
| ------ | --------- | --- |
| 1 | Intel(R) Core(TM) Ultra 7 258V, 2200 MHz, 8 Kerne, 8 logische Prozessoren | Intel(R) Arc(TM) 140V GPU, 16 GB |
| 2 | xxx | xxx |


Es werden x Varianten verglichen werden:


| Variante | Name                 | Aufgabe                                   |
| -------- | ----------------------- | ------------------------------------------- |
| 1        | Baseline / CPU      |  ohne Parallelisierung       |
| 2        | OpenCV/ CPU |  optimierte Bibliothek für Bildverarbeitung |
| 3        | PyOpenCL / GPU |  mit Parallelisierung             |
| 2 | xxx | xxx |

## Ergebnisse:

a. + b.

<table width="100%">
  <tr>
    <td width="30%"><img src="images_input/1.nature_small.jpeg" width="100%"></td>
    <td align="center" width="5%">></td>
    <td width="30%"><img src="images_output/8a.png" width="100%"></td>
    <td align="center" width="5%">></td>
    <td width="30%"><img src="images_output/8b.png" width="100%"></td>
  </tr>
  <tr>
    <td align="center">Originalbild</td>
    <td></td>
    <td align="center">8a: Graustufenbild</td>
    <td></td>
    <td align="center">8b: Helligkeit/Kontrast erhöht</td>
  </tr>
</table>

c.

<table width="100%">
  <tr>
    <td width="50%"><img src="images_output/1.nature_small.png" width="100%"></td>
    <td width="50%"><img src="images_output/2.nature_medium.png" width="100%"></td>
  </tr>
  <tr>
    <td align="center">640×415</td>
    <td align="center">1024×663</td>
  </tr>
  <tr>
    <td width="50%"><img src="images_output/3.nature_large.png" width="100%"></td>
    <td width="50%"><img src="images_output/4.nature_mega.png" width="100%"></td>
  </tr>
  <tr>
    <td align="center">1280×829</td>
    <td align="center">2048×1327</td>
  </tr>
</table>

d.

<table width="100%">
  <tr>
    <td width="50%"><img src="images_output/runtime.png" width="100%"></td>
    <td width="50%"><img src="images_output/speedup.png" width="100%"></td>
  </tr>
  <tr>
    <td align="center">Laufzeitvergleich CPU vs. PyOpenCL</td>
    <td align="center">Speedup CPU vs. PyOpenCL</td>
  </tr>
  <tr>
    <td width="50%"><img src="images_output/runtime2.png" width="100%"></td>
    <td width="50%"><img src="images_output/speedup2.png" width="100%"></td>
  </tr>
  <tr>
    <td align="center">Laufzeitvergleich CPU vs. PyOpenCL vs. OpenCV</td>
    <td align="center">Speedup CPU vs. PyOpenCL und OpenCV</td>
  </tr>
</table>



| Laptop | Bildgröße | CPU-Zeit in s | PyOpenCL-Zeit in s | OpenCV-Zeit in s | Speedup PyOpenCL | Speedup OpenCV |
| ------ | --------: | ------------: | -----------------: | ---------------: | ----------------: | -------------: |
| Laptop 1 | 640×415 | 0.001519 | 0.000843 | 0.000266 | 1.803074 | 5.717592 |
| Laptop 1 | 1024×663 | 0.004478 | 0.003081 | 0.000389 | 1.453387 | 11.498905 |
| Laptop 1 | 1280×829 | 0.036199 | 0.006760 | 0.000560 | 5.354889 | 64.600783 |
| Laptop 1 | 2048×1327 | 0.080978 | 0.014617 | 0.001083 | 5.539958 | 74.748139 |
| Laptop 2 | 640×415 | … | … | … | … | … |
| Laptop 2 | 1024×663 | … | … | … | … | … |
| Laptop 2 | 1280×829 | … | … | … | … | … |
| Laptop 2 | 2048×1327 | … | … | … | … | … |

## Interpretation


## Getting Started

### Dependencies

- Python 3.10+
- NumPy
- Matplotlib
- PyOpenCL
- OpenCV

### Installing

Repository klonen:

```bash
git clone url
cd 
```

## Authors 
Sarra Malek HTW Berlin (M.Sc. Applied Computer Science) 
xxxxxxx

## License  

This project is licensed under the MIT License.

## Acknowledgments
