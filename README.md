# GeoCoordinateConverter
Converter between Gauss-Krueger/Transverse Mercator and latitude/longitude (WGS84) coordinates for Slovenia

## Files
- **geoid_wgs.h**  
  Absolute geoid model for Slovenia from GURS (2000)
- **geoid_egm.h**  
  Absolute geoid model for Slovenia from [EGM2008]
- **gk-slo.c**  
  Main cmd-line program for converting coordinates

## How to compile
### Unix
```cc -O2 -Wall gk-slo.c -o gk-slo -lm -lrt```
### MinGW on Windows
```gcc -O2 -Wall gk-slo.c -o gk-slo.exe```
### Microsoft C
```cl /O2 /Wall gk-slo.c```

[EGM2008]: http://earth-info.nga.mil/GandG/wgs84/gravitymod/egm2008/egm08_wgs84.html
