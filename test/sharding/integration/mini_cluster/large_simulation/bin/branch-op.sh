#!/bin/bash

###########################
# These variables are available to this script :  (values are example)
# __IP_ADDRESS=192.168.1.204
# __GROUP_NAME=group-204
# __GROUP=3
# __LOGIN_USER=jamshid
# __SRCH2_HOME=/home/jamshid/mini_cluster
# __GROUP_PROCESS_COUNT=3
# __DATA_FILE_REL_PATH=./data
# __LOG_DIR_REL_PATH=./logs
# __CORE_NAME=stackoverflow
# __SRC_DIR=up to srch2-ngn folder
# __DBUILD64BIT=0 or 1 depending on 32 or 64 bit
__BIN_DIR="bin"
cd $__SRCH2_HOME
currentPath=$(pwd)
ROOT=$(pwd)


EXCEPT_LIST=();

############## Declare constants #####################
. ./$__BIN_DIR/env-constants.sh
. ./$__BIN_DIR/env-util.sh


############## Operation #############################
operation=$1
cd $__SRC_DIR
branch_name=`git branch | grep '^\*' | cut -d ' ' -f 2`
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
