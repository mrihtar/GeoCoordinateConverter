#!/bin/bash
#
# Extract GK points from GURS data
#
awk 'BEGIN {
  n = 0;
}{
  if (FNR == 1) next;
  sub("[[:space:]]*$", "");
  col1[n] = $1;
  y[n] = $2;
  x[n] = $3;
  n++;
} END {
  print n, "2 0 0";
  for (ii = 0; ii < n; ii++) {
    print ii, y[ii], x[ii]
  }
}' virtualne_vezne_tocke_v3.0.txt > vvt-tm.node
[ $? -ne 0 ] && exit 1
if [ ! -x ./triangle ]; then
  make -f Makefile.unix triangle
  [ $? -ne 0 ] && exit 1
fi
./triangle vvt-tm.node
[ $? -ne 0 ] && exit 1
#
# Extract TM points from GURS data
#
awk 'BEGIN {
  n = 0;
}{
  if (FNR == 1) next;
  sub("[[:space:]]*$", "");
  col1[n] = $1;
  y[n] = $4;
  x[n] = $5;
  n++;
} END {
  print n, "2 0 0";
  for (ii = 0; ii < n; ii++) {
    print ii, y[ii], x[ii]
  }
}' virtualne_vezne_tocke_v3.0.txt > vvt-gk.node
[ $? -ne 0 ] && exit 1
if [ ! -x ./triangle ]; then
  make -f Makefile.unix triangle
  [ $? -ne 0 ] && exit 1
fi
./triangle vvt-gk.node
[ $? -ne 0 ] && exit 1
#
# Generate include files for gk-slo
#
if [ ! -x ./ctt ]; then
  make -f Makefile.unix ctt
  [ $? -ne 0 ] && exit 1
fi
./ctt -d vvt-gk.node vvt-tm.node vvt-gk.1.ele
[ $? -ne 0 ] && exit 1
#
exit 0
