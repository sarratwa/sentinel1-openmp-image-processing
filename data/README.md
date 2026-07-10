#### Download der Sentinel-1-Daten
1. Im Copernicus Browser ein Benutzerkonto erstellen oder mit einem bestehenden Konto anmelden.

2. Nach Sentinel-1-Daten suchen und ein passendes Sentinel-1-Produkt auswählen.  
   Für dieses Projekt wird ein Level-1-GRD-Produkt verwendet, zum Beispiel mit einem Namen ähnlich zu:

```text
S1A_IW_GRDH_1SDV_...
```

3. Das Produkt herunterladen und entpacken.

4. Im entpackten Sentinel-1-Produkt befindet sich ein Ordner `measurement`.  
   In diesem Ordner liegen die eigentlichen Bilddaten als TIFF-Dateien.

5. Für dieses Projekt wird eine TIFF-Datei aus dem `measurement`-Ordner verwendet, zum Beispiel die VV-Polarisation:

```text
s1a-iw-grd-vv-...tiff
```

6. Diese TIFF-Datei wird in den Ordner `data/` kopiert und einheitlich umbenannt zu:

```text
data/sentinel_input.tiff
```

Die weitere Vorbereitung erfolgt anschließend mit dem Python-Skript `scripts/prepare_tiff.py`. Dabei wird die TIFF-Datei in ein einfaches 8-bit-PGM-Graustufenbild konvertiert, das vom C/OpenMP-Programm gelesen werden kann.