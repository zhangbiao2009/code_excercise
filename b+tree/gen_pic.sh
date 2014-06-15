#! /bin/sh

for f in $*
do
    pref=`basename $f .dot`
    dot -Tpng $f -o $pref.png
done
