# Datensatz

Die für die Benchmarks verwendeten Sentinel-1-Daten werden nicht im Repository gespeichert, da die originalen `.SAFE`-Produkte mehrere Gigabyte groß sein können.

## Datenquelle

Die Daten stammen aus dem Copernicus Browser:

```text
https://dataspace.copernicus.eu/data-collections/copernicus-sentinel-missions/sentinel-1
```

Verwendet wird ein Sentinel-1 Level-1 GRD-Produkt mit folgendem Produkttyp:

```text
S1*_IW_GRDH_1SDV_...
```

Dabei bezeichnet `IW_GRDH` ein Sentinel-1-Produkt im Interferometric-Wide-Swath-Modus als Ground-Range-Detected-High-Resolution-Datensatz. `1SDV` steht für ein Level-1-Produkt mit dualer Polarisation, typischerweise VV und VH.

## Verwendete Eingabedatei

Nach dem Download liegt das Produkt als `.SAFE`-Verzeichnis vor. Für die Bildverarbeitung wird die VV-Polarisation aus dem `measurement`-Verzeichnis verwendet:

```text
data/S1*_IW_GRDH_1SDV_*.SAFE/measurement/*vv*.tiff
```

Diese TIFF-Datei wird als zweidimensionales Intensitätsbild geladen und dient als Eingabe für die sequenzielle und parallele Filterpipeline.

## Lokale Datenstruktur

Erwartete lokale Struktur:

```text
data/
└── S1*_IW_GRDH_1SDV_*.SAFE/
    ├── annotation/
    ├── measurement/
    │   ├── *vv*.tiff
    │   └── *vh*.tiff
    ├── preview/
    ├── support/
    └── manifest.safe
```
