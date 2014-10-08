#!/bin/bash
root=~
for i in `seq 1 7`
do
   cd $root/workspace-srch2-$i/srch2-ngn/build/
   branch_name=`git branch | grep '^\*' | cut -d ' ' -f 2`
   #echo $branch_name
   git pull origin $branch_name
done

