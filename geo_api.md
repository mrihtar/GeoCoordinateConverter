## API for coordinate conversion routines ([geo.c])

#### Data structures:
- **DMS**  
  Structure holding degree, minutes & seconds.

- **ELLIPSOID**  
  Structure holding parameters and some precalculated constants for series
  for given ellipsoid.

- **HELMERT7**  
  Parameters for Helmert 7-parameters transformation, including precalculated
  rotation matrix.

- **PROJ**  
  Projection parameters for Gauss-Krueger/Transverse Mercator projections.

- **GEOGRA**  
  Structure holding geodetic/geographic coordinates (fi, la, h).

- **GEOUTM**  
  Structure holding geographic cartesian coordinates (x, y, H).

- **GEOCEN**  
  Structure holding geocentric cartesian coordinates (X, Y, Z).

- **AFT**  
  Structure holding affine transformation data (source and destination
  triangle coordinates and pre-calculated parameters for direct affine
  transformation for this triangle).

#### Global variables:
- **gid_wgs**  
  Selected geoid model on WGS84 (Slo2000 or [EGM2008]; via cmd-line or
  default).

- **hsel**  
  Selected type of output height (transformed height, copied height or
  geoid height; via cmd-line or default).

#### Initialization routines:
- **ellipsoid_init**  
  Initializes parameters and precalculate series' constants for 3 ellipsoids:
  - Bessel 1841
  - WGS84
  - GRS_80

- **params_init**  
  Initializes parameters for Helmert 7-parameters transformations, projection
  parameters (luckily both projections GK and TM are the same for Slovenia)
  and some geoid model data.

#### Supporting routines:
- **geoid_height**  
  Calculates geoid height from *fi,la* and given geoid *id* using bilinear
  interpolation. Both included geoid models (Slo2000 and [EGM2008]) cover
  only the area between:
  - N 45°15' &ndash; 47°00' in 1.0' steps (105 cells)
  - E 13°15' &ndash; 16°45' in 1.5' steps (140 cells)

  Missing values in Slo2000 geoid model (on Slovenia borders) are filled with
  the same value as current value.

- **coord_in_triangle**  
  Checks whether specified *x,y* coordinates (GK or TM) lie within or on
  borders of specified triangle (from AFT array). Algorithm first checks if
  the point is contained in a triangle bounding box, then performs the normal
  check and at the end an additional check, if the point lies on triangle
  borders (within ε distance).

- **xy2fila_ellips**  
  Transforms *x,y,H* coordinates (GK or TM) to *fi,la,h* on specified
  ellipsoid *oid*. Coordinates are first converted from relative to real
  coordinates according to projection, then fi and la are calculated. In
  the end geoid height for this point is used in calculating output h.

- **fila_ellips2xy**  
  Transforms *fi,la,h* coordinates to *x,y,H* (GK or TM) on specified
  ellipsoid *oid*. First the geoid height for given point is calculated,
  fi and la are converted to radians and then x and y are calculated. 
  In the end the precalculated geoid height is used in calculating output h.

- **xyz2fila_ellips**  
  Transforms cartesian *X,Y,Z* coordinates to *fi,la,h* on specified
  ellipsoid *oid* using Bowring's algorithm. Output h is calculated via
  algorithm. In the end fi and la are converted from radians to degrees.

- **fila_ellips2xyz**  
  Transforms *fi,la,h* coordinates to cartesian *X,Y,Z* on specified
  ellipsoid *oid*. First fi and la are converted to radians, then X,Y and
  Z are calculated using ISO/DIS 19111:2001 algorithm.

- **xyz2xyz_helmert**  
  Transforms cartesian *X,Y,Z* coordinates to cartesian *X,Y,Z* using
  specified Helmert 7-parameters transformation using the following formula
  ```[Xo;Yo;Zo] = [Xt;Yt;Zt] + (1 + s)*R*[Xi;Yi;Zi]```, where
  ```R: rotation matrix```.

#### Main conversion routines:
- **gkxy2fila_wgs**  
  Transforms Gauss-Krueger *x,y,H* coordinates on Bessel 1841 to *fi,la,h* on
  WGS84 using the following steps:
  - converts GK *x,y,H* on Bessel 1841 to fi,la,h on Bessel 1841 using xy2fila_ellips()
  - converts fi,la,h on Bessel 1841 to X,Y,Z on Bessel 1841 using fila_ellips2xyz()
  - converts X,Y,Z on Bessel 1841 to X,Y,Z on WGS84 using xyz2xyz_helmert()
  - converts X,Y,Z on WGS84 to *fi,la,h* on WGS84 using xyz2fila_ellips()

  In the end the correct height is calculated according to selected type of
  output height (transformed, copied or geoid height).

- **fila_wgs2gkxy**  
  Transforms *fi,la,h* coordinates on WGS84 to Gauss-Krueger *x,y,H* on Bessel
  1841 using the following steps:
  - the geoid height for given point is calculated
  - converts *fi,la,h* on WGS84 to X,Y,Z on WGS84 using fila_ellips2xyz()
  - converts X,Y,Z on WGS84 to X,Y,Z on Bessel 1841 using xyz2xyz_helmert()
  - converts X,Y,Z on Bessel 1841 to fi,la,h on Bessel 1841 using xyz2fila_ellips()
  - converts fi,la,h on Bessel 1841 to GK *x,y,H* on Bessel 1841 using fila_ellips2xy()

  In the end the correct height is calculated according to selected type of
  output height (transformed, copied or geoid height).

- **gkxy2tmxy**  
  Transforms Gauss-Krueger *x,y,H* coordinates on Bessel 1841 to Transverse
  Mercator *n,e,H* on WGS84 using the following steps:
  - converts GK *x,y,H* on Bessel 1841 to fi,la,h on Bessel 1841 using xy2fila_ellips()
  - converts fi,la,h on Bessel 1841 to X,Y,Z on Bessel 1841 using fila_ellips2xyz()
  - converts X,Y,Z on Bessel 1841 to X,Y,Z on WGS84 using xyz2xyz_helmert()
  - converts X,Y,Z on WGS84 to fi,la,h on WGS84 using xyz2fila_ellips()
  - converts fi,la,h on WGS84 to TM *n,e,H* on WGS84 using fila_ellips2xy()

  In the end the correct height is calculated according to selected type of
  output height (transformed, copied or geoid height).

- **tmxy2gkxy**  
  Transforms Transverse Mercator *n,e,H* coordinates on WGS84 to Gauss-Krueger
  *x,y,H* on Bessel 1841 using the following steps:
  - converts TM *n,e,H* on WGS84 to fi,la,h on WGS84 using xy2fila_ellips()
  - converts fi,la,h on WGS84 to X,Y,Z on WGS84 using fila_ellips2xyz()
  - converts X,Y,Z on WGS84 to X,Y,Z on Bessel 1841 using xyz2xyz_helmert()
  - converts X,Y,Z on Bessel 1841 to fi,la,h on Bessel 1841 using xyz2fila_ellips()
  - converts fi,la,h on Bessel 1841 to GK *x,y,H* on Bessel 1841 using fila_ellips2xy()

  In the end the correct height is calculated according to selected type of
  output height (transformed, copied or geoid height).

- **gkxy2tmxy_aft**  
  Transforms Gauss-Krueger *x,y,H* coordinates on Bessel 1841 to Transverse
  Mercator *n,e,H* on WGS84 using affine/triangle-based transformation
  with 899 reference virtual tie points (already built-in).

  Input height is copied to the output height.

- **tmxy2gkxy_aft**  
  Transforms Transverse Mercator *n,e,H* coordinates on WGS84 to Gauss-Krueger
  *x,y,H* on Bessel 1841 using affine/triangle-based transformation
  with 899 reference virtual tie points (already built-in).

  Input height is copied to the output height.

- **tmxy2fila_wgs**  
  Transforms Transverse Mercator *n,e,H* coordinates on WGS84 to *fi,la,h* on
  WGS84 using the following steps:
  - converts TM *n,e,H* on WGS84 to *fi,la,h* on WGS84 using xy2fila_ellips()

  In the end the correct height is calculated according to selected type of
  output height (only copied or geoid height possible).

- **fila_wgs2tmxy**  
  Transforms *fi,la,h* coordinates on WGS84 to Transverse Mercator *n,e,H* on
  WGS84 using the following steps:
  - converts *fi,la,h* on WGS84 to TM *n,e,H* on WGS84 using fila_ellips2xy()

  In the end the correct height is calculated according to selected type of
  output height (only copied or geoid height possible).

- **gkxy2fila_wgs_aft**  
  Transforms Gauss-Krueger *x,y,H* coordinates on Bessel 1841 to *fi,la,h* on
  WGS84 using affine/triangle-based transformation:
  - converts GK *x,y,H* on Bessel 1841 to TM *n,e,H* on WGS84 using gkxy2tmxy_aft()
  - converts TM *n,e,H* on WGS84 to *fi,la,h* on WGS84 using tmxy2fila_wgs()

  Correct height is calculated in tmxy2fila_wgs() according to selected type of
  output height (only copied or geoid height possible).

- **fila_wgs2gkxy_aft**  
  Transforms *fi,la,h* coordinates on WGS84 to Gauss-Krueger *x,y,H* on Bessel
  1841 using affine/triangle-based transformation:
  - converts *fi,la,h* on WGS84 to to TM *n,e,H* on WGS84 using fila_wgs2tmxy()
  - converts TM *n,e,H* on WGS84 to GK *x,y,H* on Bessel 1841 using tmxy2gkxy_aft()

  Correct height is calculated in fila_wgs2tmxy() according to selected type of
  output height (only copied or geoid height possible).


#### Additional routines:
- **xy2fila_ellips_loop**  
  Transforms *x,y,H* coordinates (GK or TM) to *fi,la,h* on specified
  ellipsoid *oid* using loop search over fi,la coordinates. This search
  is currently implemented only for GK zone 5 (Slovenia). Loop using
  fila_ellips2xy() ends when precision is good enough or *fi,la,h* are
  not found. In the end geoid height for this point is used in calculating
  output h.

- **fila_ellips2xy_loop**  
  Transforms *fi,la,h* coordinates to *x,y,H* (GK or TM) on specified
  ellipsoid *oid* using loop search over x,y coordinates. This search
  is currently implemented only for GK zone 5 (Slovenia). First the geoid
  height for given point is calculated, then loop using xy2fila_ellips()
  is executed until precision is good enough or *fi,la,h* are
  not found. In the end the precalculated geoid height is used in calculating
  output h.

These two routines are included for curiosity only.

#### Supporting libraries:
- **shapelib v1.3.0**  
  The [Shapefile C Library] provides routines needed for reading and writing
  ESRI Shapefiles (.shp) and the associated attribute files (.dbf):

  - SHPOpen, DBFOpen, SHPGetInfo, SHPCreate, DBFCloneEmpty, SHPComputeExtents,
    SHPWriteObject, DBFWriteTuple, ...

[geo.c]: https://github.com/mrihtar/GeoCoordinateConverter/blob/master/geo.c
[EGM2008]: http://earth-info.nga.mil/GandG/wgs84/gravitymod/egm2008/egm08_wgs84.html
[Shapefile C Library]: http://shapelib.maptools.org

