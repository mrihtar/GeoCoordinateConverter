## GeoCoordinateConverter
For English version of this file see [README.md].

**gk-slo** je konverter med geografskimi kartezičnimi koordinatami
(Gauss-Krueger/D48, Transverzalni Merkator/D96) in geodetskimi koordinatami
(širina/dolžina na ETRS89/WGS84) za Slovenijo. Lahko se ga uporabi kot
nadomestilo za uradni program za konverzijo [SiTra] &#40;s Helmertovimi
parametri za vso Slovenijo, brez regionalnih parametrov&#41;. Pri računanju
višin s pomočjo modela geoida sta na razpolago dva absolutna modela geoida
za Slovenijo: Slo2000 in [EGM2008].

Napisan je v jeziku C in se ga da prevesti in uporabljati na vseh pomembnejših
operacijskih sistemih. Podprogrami za konverzijo koordinat (v modulu "geo.c")
se lahko enostavno priredijo za uporabo tudi na drugih lokacijah, ne samo
v Sloveniji (preko definicij elipsoida ter projekcijskih in Helmertovih
parametrov).

Prevedeno verzijo **gk-slo za Windows** (32-bitna, prevedena z MinGW)
lahko dobite na  
[gk-slo-6.03.zip].


### Oznake
Kartezične koordinate                 | Geodetske koordinate
:------------------------------------ |:--------------------
x = easting                           | fi = phi (&phi;), širina (S/J)
y = northing                          | la = lambda (&lambda;), dolžina (V/Z)
H = ortometrična/nadmorska višina     | h = elipsoidna višina
Ng = geoidna višina


### Datoteke
- **[common.h]**  
  Datoteka s skupnimi definicijami za okolji Windows in Unix
- **[util.c]**  
  Zbirka skupnih funkcij za uporabo v ostalih delih programa
- **[geoid_slo.h]**  
  Absolutni model geoida za Slovenijo iz GURS (Slo2000)
- **[geoid_egm.h]**  
  Absolutni model geoida za Slovenijo iz [EGM2008]
- **[geo.h]**  
  Datoteka z definicijami za podprograme za konverzijo koordinat
- **[geo.c]**  
  Zbirka podprogramov za konverzijo koordinat
- **[gk-slo.c]**  
  Glavni program za konverzijo koordinat za uporabo iz ukazne vrstice


### Kako prevesti program
#### Unix
```$ cc -O2 -Wall gk-slo.c util.c geo.c -o gk-slo -lm -lrt``` ali  
```$ make -f Makefile.unix```
#### MinGW na Windows
```$ gcc -O2 -Wall gk-slo.c util.c geo.c -o gk-slo.exe``` ali  
```$ make -f Makefile.mingw```
#### Microsoft C
```$ cl /O2 /Wall gk-slo.c util.c geo.c``` ali  
```$ make -f Makefile.msc```


### Uporaba
<pre>
$ gk-slo [&lt;opcije&gt;] [&lt;vhodime&gt; ...]
  -d                omogoči debug izpis
  -x                izpiši referenčni test in končaj
  -gd &lt;n&gt;           zgeneriraj podatke (znotraj Slovenije) in končaj
                    1: zgeneriraj xy   (d96tm)  podatke
                    2: zgeneriraj fila (etrs89) podatke
                    3: zgeneriraj xy   (d48gk)  podatke
  -ht               izračunaj izhodno višino s 7-param. Helmertovo trans.
  -hc               kopiraj vhodno višino nespremenjeno na izhod
  -hg               izračunaj izhodno višino s pomočjo modela geoida (privzeto)
  -g slo|egm        izberi model geoida (Slo2000 ali EGM2008)
                    privzeto: Slo2000
  -dms              prikaži fila v SMS formatu za višino
  -t &lt;n&gt;            izberi transformacijo:
                    1: xy   (d96tm)  --&gt; fila (etrs89), hg?, privzeto
                    2: fila (etrs89) --&gt; xy   (d96tm),  hg
                    3: xy   (d48gk)  --&gt; fila (etrs89), ht
                    4: fila (etrs89) --&gt; xy   (d48gk),  hg
                    5: xy   (d48gk)  --&gt; xy   (d96tm),  hg(hc)
                    6: xy   (d96tm)  --&gt; xy   (d48gk),  ht(hc)
  -r                obrni vrstni red branja xy/fila
                    (opozorilo se izpiše če je y &lt; 200000)
  &lt;vhodime&gt;         preberi in konvertiraj vhodne podatke iz datoteke &lt;vhodime&gt;
                    &lt;vhodime&gt; "-" pomeni stdin, prej uporabi "--"
  -o -|=|&lt;izhodime&gt; zapiši izhodne podatke na:
                    -: stdout (privzeto)
                    =: prilepi ".out" k imenu vsake datoteke &lt;vhodime&gt; in
                       zapiši izhodne podatke na te ločene datoteke
                    &lt;izhodime&gt;: zapiši vse izhodne podatke na 1 datoteko &lt;izhodime&gt;

Tipični format vhodnih podatkov (SiTra):
[&lt;oznaka&gt;]  &lt;fi|x&gt;  &lt;la|y&gt;  &lt;h|H&gt;
</pre>


#### Primer 1 (D48)
Vhodna datoteka VTG2225.XYZ (DMV, v formatu [SiTraNet], Gauss-Krueger/D48):
<pre>
0000001 509000.000 76000.000 343.30
0000002 509005.000 76000.000 342.80
0000003 509010.000 76000.000 342.30
</pre>
Konvertiraj v nov koordinatni sistem (Transverzalni Merkator/D96); višine se
kopirajo, ne preračunavajo:
<pre>
$ gk-slo -t 5 -hc VTG2225.XYZ
VTG2225.XYZ: possibly reversed xy
0000001 509487.490 575640.546 343.300
0000002 509492.490 575640.546 342.800
0000003 509497.490 575640.546 342.300
</pre>
Če dobiš opozorilo "**possibly reversed xy**", uporabi opcijo "**-r**"
da dobiš pravilno konverzijo:
<pre>
$ gk-slo -t 5 -hc -r VTG2225.XYZ
0000001 76484.893 508628.990 343.300
0000002 76484.893 508633.991 342.800
0000003 76484.893 508638.991 342.300
</pre>
Konvertiraj isto datoteko v ETRS89/WGS84 koordinate. Tokrat se bo višina
preračunala z uporabo Helmertove transformacije (privzeto):
<pre>
$ gk-slo -t 3 -r VTG2225.XYZ
0000001 45.8281655853 15.1110624092 389.063
0000002 45.8281655218 15.1111267639 388.563
0000003 45.8281654582 15.1111911187 388.063
</pre>
Za bolj razumljiv izpis lahko dodaš opcijo "**-dms**":
<pre>
$ gk-slo -t 3 -r -dms VTG2225.XYZ
0000001 45.8281655853 15.1110624092 389.063 45 49 41.39611 15  6 39.82467
0000002 45.8281655218 15.1111267639 388.563 45 49 41.39588 15  6 40.05635
0000003 45.8281654582 15.1111911187 388.063 45 49 41.39565 15  6 40.28803
</pre>
Shrani rezultate v datoteko:
<pre>
$ gk-slo -t 3 -r VTG2225.XYZ -o VTG2225.flh
<i>(ustvari datoteko VTG2225.flh)</i>
</pre>


#### Primer 2 (D96)
Vhodna datoteka VTC0512.XYZ (DMV, v formatu [SiTraNet], Transverzalni Merkator/D96):
<pre>
0000001 412250 97000 606.2
0000002 412250 96995 606.9
0000003 412250 96990 607.9
</pre>
Konvertiraj v ETRS89/WGS84 koordinate, višine se bodo preračunale s pomočjo
modela geoida Slo2000 (privzeto):
<pre>
$ gk-slo -t 1 -r VTC0512.XYZ
0000001 46.0071929697 13.8669428837 652.772
0000002 46.0071479903 13.8669438021 653.472
0000003 46.0071030110 13.8669447206 654.472
</pre>
Če želiš uporabiti model geoida EGM2008, uporabi opcijo "**-g egm**":
<pre>
$ gk-slo -t 1 -r -g egm VTC0512.XYZ
0000001 46.0071929697 13.8669428837 652.660
0000002 46.0071479903 13.8669438021 653.359
0000003 46.0071030110 13.8669447206 654.359
</pre>


#### Primer 3 (ETRS89/WGS84)
Konvertiraj ETRS89/WGS84 koordinate v Transverzalne Merkatorjeve/D96 preko
tipkovnice (ignorirajoč višino, uporabi "**--**" da preprečiš nadaljnjo
analizo opcij):
<pre>
$ gk-slo -t 2 -- -
46.0071929697 13.8669428837 0 <i>&lt;Enter&gt;</i>
97000.000 412250.000 -46.572
</pre>
Konvertiraj datoteko VTG2225.flh (z ETRS89/WGS84 koordinatami, glej Primer 1)
v Gauss-Krueger/D48:
<pre>
$ gk-slo -t 4 VTG2225.flh
0000001 76000.000 509000.000 342.896
0000002 76000.000 509005.000 342.396
0000003 76000.000 509010.000 341.896
</pre>
Če primerjaš rezultat z originalno datoteko VTG2225.XYZ, lahko opaziš majhno
razliko v višinah. To se je zgodilo zato, ker so bile višine v VTG2225.flh
izračunane z uporabo Helmertove transformacije, privzeti način izračuna višin
pri "-t 4" pa je izračun s pomočjo modela geoida.

Kateri način izračuna višin se uporabi za konverzijo je odvisno tudi od
vhodnih podatkov. Za vsak tip konverzij je v programu privzet priporočen
način izračuna višin (glej Uporabo).


#### Primer 4 (obdelava veliko datotek)
Če je bil program gk-slo preveden z MinGW na Windows ali je uporabljan na
Unixu, lahko obdelamo veliko datotek hkrati z enim ukazom.

Vhodne datoteke &lowast;.XYZ (DMV, Transverzalni Merkator/D96):
<pre>
VTH0720.XYZ
VTH0721.XYZ
VTH0722.XYZ
...
</pre>

Konvertiraj vse datoteke v ETRS89/WGS84 koordinate (z nekaj debug izpisa, da
vidimo, kaj se dogaja):
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
Rezultati konverzije za vsako datoteko se zapišejo na novo datoteko s
podaljškom ".out":
<pre>
VTH0720.XYZ.out
VTH0721.XYZ.out
VTH0722.XYZ.out
...
</pre>

[README.md]: https://github.com/mrihtar/GeoCoordinateConverter/blob/master/README.md
[SiTra]: http://sitra.sitranet.si
[SiTraNet]: http://sitranet.si
[EGM2008]: http://earth-info.nga.mil/GandG/wgs84/gravitymod/egm2008/egm08_wgs84.html
[gk-slo-6.03.zip]: https://app.box.com/s/vyj1mlghsuevcy921zhs
[common.h]: https://github.com/mrihtar/GeoCoordinateConverter/blob/master/common.h
[util.c]: https://github.com/mrihtar/GeoCoordinateConverter/blob/master/util.c
[geoid_slo.h]: https://github.com/mrihtar/GeoCoordinateConverter/blob/master/geoid_slo.h
[geoid_egm.h]: https://github.com/mrihtar/GeoCoordinateConverter/blob/master/geoid_egm.h
[geo.h]: https://github.com/mrihtar/GeoCoordinateConverter/blob/master/geo.h
[geo.c]: https://github.com/mrihtar/GeoCoordinateConverter/blob/master/geo.c
[gk-slo.c]: https://github.com/mrihtar/GeoCoordinateConverter/blob/master/gk-slo.c
