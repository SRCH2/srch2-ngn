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
__BIN_DIR="bin"
cd $__SRCH2_HOME
currentPath=$(pwd)
ROOT=$(pwd)


EXCEPT_LIST=();

############## Declare constants #####################
. ./$__BIN_DIR/env-constants.sh
. ./$__BIN_DIR/env-util.sh


############## Operation #############################
if [ $1 -a $1 == "-k" ]; then
    bash $( _KILL_COMMAND $__GROUP_NAME)
    exit
fi
cd $(_SRCH2_HOME)
echo "echo \"Killing the engines ...\"" > $( _KILL_COMMAND $__GROUP_NAME)
chmod a+x $( _KILL_COMMAND $__GROUP_NAME)
for i in `seq $NODE_START $NODE_END`
   
do
   date > $(_CONSULE_LOG $__GROUP_NAME $i)
   $(_ENGINE_BINARY) --config=$(_CONF_FILE $i) > $(_CONSULE_LOG $__GROUP_NAME $i) &
   sleep 2
   PID=$(_GET_Ith_TOKEN "$(ps aux | grep \"--config=$(_CONF_FILE $i)\" | head -1)" 1 ' ')
   echo $PID > $(_RUN_INFO_LOG $__GROUP_NAME $i)
   echo "kill -9 $PID" >> $(_KILL_COMMAND $__GROUP_NAME)
done



