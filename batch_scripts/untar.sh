#!/bin/bash

i=0
for f in *
do
 tar xf $f &
 i=$((i+1))
 if (( $i % 10 == 0 )); then wait; fi
done
wait
echo "finished!" 
