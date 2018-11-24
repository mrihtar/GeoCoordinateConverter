#!/bin/bash
#
# Extract GK points from GURS data
#
awk -f extgk.awk virtualne_vezne_tocke_v3.0.txt > vvt-gk.node
[ $? -ne 0 ] && exit 1
./triangle vvt-gk.node
[ $? -ne 0 ] && exit 1
#
# Extract TM points from GURS data
#
awk -f exttm.awk virtualne_vezne_tocke_v3.0.txt > vvt-tm.node
[ $? -ne 0 ] && exit 1
./triangle vvt-tm.node
[ $? -ne 0 ] && exit 1
#
# Generate include files for gk-slo
#
./ctt -d vvt-gk.node vvt-tm.node vvt-gk.1.ele
[ $? -ne 0 ] && exit 1
#
exit 0
