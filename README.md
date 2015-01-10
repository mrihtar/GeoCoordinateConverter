# GeoCoordinateConverter
Converter between Gauss-Krueger/Transverse Mercator and latitude/longitude (WGS84) coordinates for Slovenia

## Files
- **common.h**  
  Include file with common definitions for Windows and Unix environment
- **util.c**  
  Collection of utility functions for use in other parts of program
- **geoid_wgs.h**  
  Absolute geoid model for Slovenia from GURS (2000)
- **geoid_egm.h**  
  Absolute geoid model for Slovenia from [EGM2008]
- **geo.h**  
  Include file for using coordinate conversion routines
- **geo.c**  
  Collection of coordinate conversion routines
- **gk-slo.c**  
  Main cmd-line program for converting coordinates

## How to compile
### Unix
```cc -O2 -Wall gk-slo.c util.c geo.c -o gk-slo -lm -lrt```
### MinGW on Windows
```gcc -O2 -Wall gk-slo.c util.c geo.c -o gk-slo.exe```
### Microsoft C
```cl /O2 /Wall gk-slo.c util.c geo.c```

[EGM2008]: http://earth-info.nga.mil/GandG/wgs84/gravitymod/egm2008/egm08_wgs84.html
