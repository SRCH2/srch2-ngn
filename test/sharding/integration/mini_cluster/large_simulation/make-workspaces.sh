#!/bin/bash
root=~
for i in `seq 1 7`
do
   cd $root/workspace-srch2-$i/srch2-ngn/build/
   cmake -DBUILD_RELEASE=0 -DBUILD64BIT=1 ..
   make
done
