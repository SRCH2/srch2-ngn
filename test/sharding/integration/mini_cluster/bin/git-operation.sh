#!/bin/bash
operation=$1
cd $WORKSPACE_DIR
branch_name=`git branch | grep '^\*' | cut -d ' ' -f 2`
#echo $branch_name
case $operation in
-p) git pull origin $branch_name
    ;;
-c) git clone git@bitbucket.org:srch2inc/srch2-ngn.git
    ;;
-cm) cmake -DBUILD_RELEASE=0 -DBUILD64BIT=1 ..
    ;;
-m)  make -j 8 ..
    ;;   
esac
