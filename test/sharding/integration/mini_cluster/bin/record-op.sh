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
##### This script performs batches of insert/delete/update 
##### operations on the process given in input


__abort()
{
   if [[ $# == 1 ]];
   then
      echo "Failed: $1"
   fi
   echo "Usage: bash ./record-op.sh [options]"
   echo "Options:"
   echo "--operation insert/delete/update | (default value: insert)"
   echo "--batch_size 50 | (Only available in case of insertion, default value: 100)"
   echo "--hostname 192.168.1.200 | (default value: 127.0.0.1)"
   echo "--port 1234 | (default value: 8089)"
   echo "--core_name statemedia | (default value: stackoverflow)"
   exit
}

operation="insert"
hostname="127.0.0.1"
port_num=8089
core_name="stackoverflow"

# only in case of insert this option is valid
batch_size=100

while (($#)) ;
do
   case $1 in
   --operation)
       operation=$2
       shift
       shift
    ;;
   --core)
       core_name=$1
       shift
       shift
    ;;
   --hostname)
       hostname=$1
       shift
       shift
    ;;
   --port)
       port_num=$1
       shift
       shift
    ;;
   --batch_size)
       if [[ $operation != "insert" ]];
       then
          __abort "batch size is only usable for operation insert"
       fi
       batch_size=$1
       if [[ $batch_size > 100 ]];
       then
          batch_size=100
          echo "Warning: Maximum batch size can be 100. Batch size is set to 100."
       fi
       shift
       shift
    ;;
   *)
       shift
    ;;
   esac

done

if [[ $ENV_DEBUG_FLAG ]];
then
   echo "Program Inputs:"
   echo "Operation : $operation"
   if [[ $operation == "insert" ]];
   then
      echo "	Batch size : $batch_size"
      echo "    NODES      : $NODE_START - $NODE_END"
      for i in `seq $NODE_START $NODE_END`

      do 
         echo "    FILE PATH  : $__DATA_FILE_REL_PATH/$__CORE_NAME/data-$__GROUP_NAME-$i.json"
         echo "    IP ADDRESS : $__IP_ADDRESS"
         echo "    CORE       : $__CORE_NAME"
         let "EXT_PORT_BASE += 1"
         echo "    PORT       : $EXT_PORT_BASE" 
         echo "    COMMAND    : python ./$__BIN_DIR/insertReqGenerator.py $__DATA_FILE_REL_PATH/$__CORE_NAME/data-$__GROUP_NAME-$i.json $__IP_ADDRESS $EXT_PORT_BASE $__CORE_NAME insert &"
         python ./$__BIN_DIR/insertReqGenerator.py $__DATA_FILE_REL_PATH/$__CORE_NAME/data-$__GROUP_NAME-$i.json $__IP_ADDRESS $EXT_PORT_BASE $__CORE_NAME insert &
      done
   fi
   echo "Core name : $core_name"
   echo "Hostname : $hostname"
   echo "Port : $port_num"
fi





