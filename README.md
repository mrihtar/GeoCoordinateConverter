## GeoCoordinateConverter
Za slovensko verzijo te datoteke glej [PREBERIME.md].

**gk-slo** is a converter between geographic cartesian coordinates
(Gauss-Krueger/D48, Transverse Mercator/D96) and geodetic coordinates
(latitude/longitude on ETRS89/WGS84) for Slovenia. It can be used
as a replacement for the official conversion program [SiTra] &#40;with Helmert
parameters for the whole Slovenia, no regional parameters&#41;. For calculating
heights with the help of geoid model two absolute geoid models for Slovenia are
available: Slo2000 and [EGM2008].

It's written in C language and can be compiled and used on all major
operating systems. Coordinate conversion routines (in module "geo.c")
can be easily adapted to locations other than Slovenia (via definition of
ellipsoid, projection and Helmert parameters).

Detailed description of coordinate conversion routines and their API
from module "geo.c" is in file [geo_api.md].

Precompiled version of **gk-slo for Windows** (32-bit, compiled with MinGW)
can be downloaded from  
[gk-slo-6.03.zip].


### Conventions
Cartesian coordinates                 | Geodetic coordinates
:------------------------------------ |:--------------------
x = easting                           | fi = phi (&phi;), latitude, Breite (N/S)
y = northing                          | la = lambda (&lambda;), longitude, L&auml;nge (E/W)
H = ortometric/above sea level height | h = ellipsoidal height
Ng = geoid height


### Files
- **[common.h]**  
  Include file with common definitions for Windows and Unix environment
- **[util.c]**  
  Collection of utility functions for use in other parts of program
- **[geoid_slo.h]**  
  Absolute geoid model for Slovenia from GURS (Slo2000)
- **[geoid_egm.h]**  
  Absolute geoid model for Slovenia from [EGM2008]
- **[geo.h]**  
  Include file for using coordinate conversion routines
- **[geo.c]**  
  Collection of coordinate conversion routines
- **[gk-slo.c]**  
  Main cmd-line program for converting coordinates


### How to compile
#### Unix
```$ cc -O2 -Wall gk-slo.c util.c geo.c -o gk-slo -lm -lrt``` or  
```$ make -f Makefile.unix```
#### MinGW on Windows
```$ gcc -O2 -Wall gk-slo.c util.c geo.c -o gk-slo.exe``` or  
```$ make -f Makefile.mingw```
#### Microsoft C
```$ cl /O2 /Wall gk-slo.c util.c geo.c``` or  
```$ make -f Makefile.msc```


### Usage
<pre>
$ gk-slo [&lt;options&gt;] [&lt;inpname&gt; ...]
  -d                enable debug output
  -x                print reference test and exit
  -gd &lt;n&gt;           generate data (inside Slovenia) and exit
                    1: generate xy   (d96tm)  data
                    2: generate fila (etrs89) data
                    3: generate xy   (d48gk)  data
  -ht               calculate output height with 7-params Helmert trans.
  -hc               copy input height unchanged to output
  -hg               calculate output height from geoid model (default)
  -g slo|egm        select geoid model (Slo2000 or EGM2008)
                    default: Slo2000
  -dms              display fila in DMS format after height
  -t &lt;n&gt;            select transformation:
                    1: xy   (d96tm)  --&gt; fila (etrs89), hg?, default
                    2: fila (etrs89) --&gt; xy   (d96tm),  hg
                    3: xy   (d48gk)  --&gt; fila (etrs89), ht
                    4: fila (etrs89) --&gt; xy   (d48gk),  hg
                    5: xy   (d48gk)  --&gt; xy   (d96tm),  hg(hc)
                    6: xy   (d96tm)  --&gt; xy   (d48gk),  ht(hc)
  -r                reverse parsing order of xy/fila
                    (warning displayed if y &lt; 200000)
  &lt;inpname&gt;         parse and convert input data from &lt;inpname&gt;
                    &lt;inpname&gt; "-" means stdin, use "--" before
  -o -|=|&lt;outname&gt;  write output data to:
                    -: stdout (default)
                    =: append ".out" to each &lt;inpname&gt; and
                       write output to these separate files
                    &lt;outname&gt;: write all output to 1 file &lt;outname&gt;

Typical input data format (SiTra):
[&lt;label&gt;]  &lt;fi|x&gt;  &lt;la|y&gt;  &lt;h|H&gt;
</pre>


#### Example 1 (D48)
Input file VTG2225.XYZ (DMV, in [SiTraNet] format, Gauss-Krueger/D48):
<pre>
0000001 509000.000 76000.000 343.30
0000002 509005.000 76000.000 342.80
0000003 509010.000 76000.000 342.30
</pre>
Convert to new coordinate system (Transverse Mercator/D96); heights should
be copied, not calculated:
<pre>
$ gk-slo -t 5 -hc VTG2225.XYZ
VTG2225.XYZ: possibly reversed xy
0000001 509487.490 575640.546 343.300
0000002 509492.490 575640.546 342.800
0000003 509497.490 575640.546 342.300
</pre>
If you see the warning "**possibly reversed xy**", use the option "**-r**"
to get the correct conversion:
<pre>
$ gk-slo -t 5 -hc -r VTG2225.XYZ
0000001 76484.893 508628.990 343.300
0000002 76484.893 508633.991 342.800
0000003 76484.893 508638.991 342.300
</pre>
Convert the same file to ETRS89/WGS84 coordinates. This time the height will
be recalculated using Helmert transformation (default):
<pre>
$ gk-slo -t 3 -r VTG2225.XYZ
0000001 45.8281655853 15.1110624092 389.063
0000002 45.8281655218 15.1111267639 388.563
0000003 45.8281654582 15.1111911187 388.063
</pre>
For better readout you can include the option "**-dms**":
<pre>
$ gk-slo -t 3 -r -dms VTG2225.XYZ
0000001 45.8281655853 15.1110624092 389.063 45 49 41.39611 15  6 39.82467
0000002 45.8281655218 15.1111267639 388.563 45 49 41.39588 15  6 40.05635
0000003 45.8281654582 15.1111911187 388.063 45 49 41.39565 15  6 40.28803
</pre>
Store result in a file:
<pre>
$ gk-slo -t 3 -r VTG2225.XYZ -o VTG2225.flh
<i>(creates file VTG2225.flh)</i>
</pre>


#### Example 2 (D96)
Input file VTC0512.XYZ (DMV, in [SiTraNet] format, Transverse Mercator/D96):
<pre>
0000001 412250 97000 606.2
0000002 412250 96995 606.9
0000003 412250 96990 607.9
</pre>
Convert to ETRS89/WGS84 coordinates, height will be recalculated using
geoid model Slo2000 (default):
<pre>
$ gk-slo -t 1 -r VTC0512.XYZ
0000001 46.0071929697 13.8669428837 652.772
0000002 46.0071479903 13.8669438021 653.472
0000003 46.0071030110 13.8669447206 654.472
</pre>
If you want to use the EGM2008 geoid model, use the "**-g egm**" option:
<pre>
$ gk-slo -t 1 -r -g egm VTC0512.XYZ
0000001 46.0071929697 13.8669428837 652.660
0000002 46.0071479903 13.8669438021 653.359
0000003 46.0071030110 13.8669447206 654.359
</pre>


#### Example 3 (ETRS89/WGS84)
Convert ETRS89/WGS84 coordinates to Transverse Mercator/D96 via keyboard
(ignoring height, use "**--**" to stop parsing options):
<pre>
$ gk-slo -t 2 -- -
46.0071929697 13.8669428837 0 <i>&lt;Enter&gt;</i>
97000.000 412250.000 -46.572
</pre>
Convert file VTG2225.flh (with ETRS89/WGS84 coordinates, see Example 1)
to Gauss-Krueger/D48:
<pre>
$ gk-slo -t 4 VTG2225.flh
0000001 76000.000 509000.000 342.896
0000002 76000.000 509005.000 342.396
0000003 76000.000 509010.000 341.896
</pre>
If you compare the output with the original VTG2225.XYZ, you'll notice
a small difference in heights. This is because the heights in VTG2225.flh
were calculated using Helmert transformation but the default height
calculation for "-t 4" is using geoid model.

Which height calculation you use for conversion depends also on input data.
There are some recommended defaults for each type of conversion (see Usage).


#### Example 4 (processing many files)
If gk-slo was compiled with MinGW on Windows or is being used on Unix, you
can process many files with one command.

Input files &lowast;.XYZ (DMV, Transverse Mercator/D96):
<pre>
VTH0720.XYZ
VTH0721.XYZ
VTH0722.XYZ
...
</pre>

Convert all files to ETRS89/WGS84 coordinates (with some debug info to see
what's going on):
<pre>
$ gk-slo -t 1 -r -d &lowast;.XYZ -o =
Processing VTH0720.xyz
Processing time: 4.854913
Processing VTH0721.xyz
Processing time: 4.846438
Processing VTH0722.xyz
Processing time: 4.846361
...
</pre>
Results of conversion for each file are written to a new file with an
extension ".out":
<pre>
VTH0720.XYZ.out
VTH0721.XYZ.out
VTH0722.XYZ.out
...
</pre>

[PREBERIME.md]: https://github.com/mrihtar/GeoCoordinateConverter/blob/master/PREBERIME.md
[SiTra]: http://sitra.sitranet.si
[SiTraNet]: http://sitranet.si
[EGM2008]: http://earth-info.nga.mil/GandG/wgs84/gravitymod/egm2008/egm08_wgs84.html
[geo_api.md]: https://github.com/mrihtar/GeoCoordinateConverter/blob/master/geo_api.md
[gk-slo-6.03.zip]: https://app.box.com/s/vyj1mlghsuevcy921zhs
[common.h]: https://github.com/mrihtar/GeoCoordinateConverter/blob/master/common.h
[util.c]: https://github.com/mrihtar/GeoCoordinateConverter/blob/master/util.c
[geoid_slo.h]: https://github.com/mrihtar/GeoCoordinateConverter/blob/master/geoid_slo.h
[geoid_egm.h]: https://github.com/mrihtar/GeoCoordinateConverter/blob/master/geoid_egm.h
[geo.h]: https://github.com/mrihtar/GeoCoordinateConverter/blob/master/geo.h
[geo.c]: https://github.com/mrihtar/GeoCoordinateConverter/blob/master/geo.c
[gk-slo.c]: https://github.com/mrihtar/GeoCoordinateConverter/blob/master/gk-slo.c
