## Afina/Trikotniška transformacija
For English version of this file see [README.md].

### Teorija
Afina transformacija deluje samo nad kartezičnimi koordinatami (GK x,y ⇔ TM x,y).
Zanjo potrebujete minimalno 3 kontrolne točke (na primer trikotnik), vsak s
4 koordinatami ```(Xs, Ys, Xt, Yt)```, kjer sta ```Xs,Ys``` koordinati v
izvornem sistemu in ```Xt,Yt``` koordinati v ciljnem sistemu.

Za vsako novo točko ```Xi,Yi``` se transformacija izračuna po naslednji enačbi:
<pre>
Xo = a&#8270;Xi + b&#8270;Yi + c
Yo = d&#8270;Xi + e&#8270;Yi + f
</pre>
kjer imajo parametri ```a..f``` naslednji pomen:
<pre>
a: Skaliranje X
e: Skaliranje Y
d: Rotacija X
b: Rotacija Y
c: Translacija X
f: Translacija Y
</pre>
Za izračun parametrov ```a..f``` za dani trikotnik je potrebno rešiti sistem
linearnih enačb s 6 neznankami, ki ga lahko predstavimo z naslednjimi
matrikami:
<pre>
| Xs1 Ys1 1 0 0 0 | | a |   | Xt1 |
| Xs2 Ys2 1 0 0 0 | | b |   | Xt2 | 
| Xs3 Ys3 1 0 0 0 | | c | = | Xt3 | 
| 0 0 0 Xs1 Ys1 1 | | d |   | Yt1 | 
| 0 0 0 Xs2 Ys2 1 | | e |   | Yt2 | 
| 0 0 0 Xs3 Ys3 1 | | f |   | Yt3 |
</pre>
Ta sistem lahko rešimo z eliminacijsko metodo (operacije nad vrsticami ⇒ 
identična matrika) ali z drugimi metodami (Jacobi, Gauss-Seidel, ...).

### Predizračun
Za Slovenijo imamo 899 referenčnih [virtualih veznih točk] &#40;v3.0&#41; v
obeh koordinatnih systemih (GK in TM), ki formirajo 1776 trikotnikov:

<img src="../images/Slovenia-tie-points.gif" width="400px">
<img src="../images/Slovenia-triangles.gif" height="266px">

Za vsak trikotnik lahko vnaprej izračunamo parametre ```a..f``` in njihove
vrednosti shranimo v tabeli, ki jo lahko direktno uporabimo v programu.

Za izračun trikotnikov iz začetnega spiska referenčnih točk uporabimo
obstoječi program [triangle] od Jonathana Shewchuka (UCB), ki generira
natančne Delaunayeve triangulacije.

#### Natančen opis procesa
- pridobi referenčne [virtualne vezne točke] &#40;že priložene&#41;:  
  ⇒ datoteka ```virtualne_vezne_tocke_v3.0.txt```
- ekstraktaj GK in TM koordinate iz referenčne datoteke
- uporabi program [triangle] za kreiranje trikotnikov
- uporabi priložen program ```ctt``` za kreiranje tabel, ki se bodo vključile v program:  
  ⇒ datoteki ```aft_gktm.h``` and ```aft_tmgk.h```  
  ```ctt``` rešuje zgoraj omenjen sistem linearnih enačb za vsak trikotnik.

Vse to se lahko avtomatizira z uporabo priloženih skript ```cvvt.sh``` (za Unix)
ali ```cvvt.bat``` (za Windows).

### Uporaba v programu
Za vsako kartezično koordinato se je potrebno z zanko sprehoditi po tabeli
1776 vnaprej izračunanih trikotnikov in preveriti, ali koordinata leži znotraj
trikotnika. Ta test mora biti zelo natančen, drugače lahko končamo s točko,
ki leži nekje "med" trikotniki (glej [geo_api.md]).

Ko/če je takšen trikotnik najden, lahko uporabimo preprosto transformacijo
s parametri ```a..f``` (glej **Teorija** zgoraj).


[README.md]: https://github.com/mrihtar/GeoCoordinateConverter/blob/master/aft/README.md
[virtualih veznih točk]: http://www.e-prostor.gov.si/si/zbirke_prostorskih_podatkov/drzavni_koordinatni_sistem/horizontalni_drzavni_koordinatni_sistem_d96tm/d96tm/transformacijski_parametri/
[virtualne vezne točke]: http://www.e-prostor.gov.si/si/zbirke_prostorskih_podatkov/drzavni_koordinatni_sistem/horizontalni_drzavni_koordinatni_sistem_d96tm/d96tm/transformacijski_parametri/
[triangle]: http://www.cs.cmu.edu/~quake/triangle.html
[geo_api.md]: https://github.com/mrihtar/GeoCoordinateConverter/blob/master/geo_api.md
