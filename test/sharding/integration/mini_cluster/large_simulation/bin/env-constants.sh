#!/bin/bash
if [ -z "$CONSTANTS_DEFINED"  ]; then
##############################################################################
echo "Exporting constants ..."
export NODE_START=1
export NODE_END=$__GROUP_PROCESS_COUNT
echo "There are $__GROUP_PROCESS_COUNT nodes in this group."

export LICENSE_FILE="srch2_license_key.txt"
export STOP_WORDS="stop-words.txt"
export PROTECTED_WORDS="srch2_protected_words.txt"
export LOG_FILE_PREFIX="srch2-log"
export DATA_SOURCE="/home/jamshid/srch2/sharding-demo/data/"
export DATA_FILE_PREFIX="statemedia-1000"

# config file constants
export LOG_FILE_PREFIX=$__LOG_DIR_REL_PATH/"srch2-log"
export CONF_FILE_PREFIX="conf-$__GROUP_NAME-"
export CLUSTER_NAME="SRCH2_Cluster"
export NODE_NAME_PREFIX="Node-$__GROUP_NAME-"
export TM_PORT_BASE=4001
export TM_IP_ADDRESS="$__IP_ADDRESS"
export WELL_KNOWN_HOSTS="$__IP_ADDRESS:54000"
export EXT_HOSTNAME="$__IP_ADDRESS"
export EXT_PORT_BASE=7000
export NUM_PARTITIONS=16
export NUM_REPLICAS=3
export WORKSPACE_DIR=$__SRC
export BINARY_FILE=$__BINARY_FILE
export DATA_CORE_NAME=$__CORE_NAME

# environment installation variables

export SRCH2_HOME=$__SRCH2_HOME
export ROOT=$SRCH2_HOME
#export ROOT="hello"
export ENV_LOG_ROOT=$__LOG_DIR_REL_PATH
export ENV_DATA_ROOT="data"
export ENV_PULL_LOG="pull-outputs.txt"
export ENV_MAKE_LOG="make-outputs.txt"
export ENV_BIN="bin"
export ENV_CLEAR_EXE="clear.sh"
### frontend paths are relative
export FRONTEND_CONF_FILE="./frontend/cluster_config.txt"

# desktop test constants
#export DESKTOP_ROW_SIZE=4
#export DESKTOP_COL_SIZE=2
#export DESKTOP_OFFSET=1
#export DESKTOP_W=`wmctrl -d | head -n 1 | cut -d ' ' -f 12 | cut -d 'x' -f 1`
#export DESKTOP_H=`wmctrl -d | head -n 1 | cut -d ' ' -f 12 | cut -d 'x' -f 2`
#export WIN_PER_DESKTOP=4
#export WIN_W=$(( DESKTOP_W / 3))
#export WIN_H=$(( DESKTOP_H / 4))
############################################################################
export CONSTANTS_DEFINED="SET"
fi
