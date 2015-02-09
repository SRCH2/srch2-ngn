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
case $operation in
'-l');&
'--logs')
        cd $__SRC_DIR
        truncate --size 0k $(_ENV_LOG_ROOT)/srch2-log-*.txt
        truncate --size 0k $(_ENV_LOG_ROOT)/consule-*.log
        truncate --size 0k $(_ENV_LOG_ROOT)/run-info-*.log
        truncate --size 0k $(_ENV_LOG_ROOT)/valgrind-*.log
        echo "Group $__GROUP_NAME | log files truncated."
;;
'-cr');&
'--cluster-root')
        rm -r $CLUSTER_NAME/*
        echo "Group $__GROUP_NAME | all content of $CLUSTER_NAME is removed."
;;
'-b')
        cd $__SRC_DIR
        mkdir -p $(_ENV_LOG_ROOT)/srch2-log-backup/
        mkdir -p $(_ENV_LOG_ROOT)/consule-backup/
        mkdir -p $(_ENV_LOG_ROOT)/run-info-log-backup/
        mkdir -p $(_ENV_LOG_ROOT)/valgrind-backup/
        cp $(_ENV_LOG_ROOT)/srch2-log-*.txt $(_ENV_LOG_ROOT)/srch2-log-backup/
        cp $(_ENV_LOG_ROOT)/consule-*.log $(_ENV_LOG_ROOT)/consule-backup/
        cp $(_ENV_LOG_ROOT)/run-info-*.log $(_ENV_LOG_ROOT)/run-info-log-backup/
        cp $(_ENV_LOG_ROOT)/run-info-*.log $(_ENV_LOG_ROOT)/valgrind-backup/
        truncate --size 0k $(_ENV_LOG_ROOT)/srch2-log-*.txt
        truncate --size 0k $(_ENV_LOG_ROOT)/consule-*.log
        truncate --size 0k $(_ENV_LOG_ROOT)/run-info-*.log
        truncate --size 0k $(_ENV_LOG_ROOT)/valgrind-*.log
        echo "Group $__GROUP_NAME | log files backed up and truncated."
;;
esac

