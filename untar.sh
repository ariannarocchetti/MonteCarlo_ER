#!/bin/bash
main_dir=$PWD
echo $main_dir
for dir in `ls`
do
 if test -d $dir
    then
      echo "$dir is a subdirectory..."
      
      cd $dir
      echo $PWD
      i=0
      for f in *
      do
       tar xf $f &
       i=$((i+1))
       if (( $i % 15 == 0 )); then wait; fi
       done
       wait
       echo "finished!"
      cd $main_dir
      
 fi
done 
