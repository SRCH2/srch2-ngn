#!/bin/bash
if [ -z "$CONSTANTS_DEFINED"  ]; then
##############################################################################
echo "Exporting constants ..."
export NODE_START=1
export NODE_END=20

export LICENSE_FILE="srch2_license_key.txt"
export STOP_WORDS="stop-words.txt"
export PROTECTED_WORDS="srch2_protected_words.txt"
export LOG_FILE_PREFIX="srch2-log"
export DATA_SOURCE="/home/jamshid/srch2/sharding-demo/data/"
export DATA_CORE_NAME="statemedia"
export DATA_FILE_PREFIX="statemedia-1000"


# config file constants
export CONF_FILE_PREFIX="conf-"
export CLUSTER_NAME="SRCH2_Cluster"
export NODE_NAME_PREFIX="Node-group-201-"
export TM_PORT_BASE=4000
export TM_IP_ADDRESS="192.168.1.201"
export WELL_KNOWN_HOSTS="192.168.1.201:54000"
export EXT_HOSTNAME="192.168.1.201"
export EXT_PORT_BASE=7000
export NUM_PARTITIONS=8
export NUM_REPLICAS=4
export BINARY_DIR="/home/jamshid/workspace-srch2-v4/srch2-ngn/build/"
export BINARY_NAME="src/server/srch2-search-server"

# environment installation variables

export SRCH2_HOME="/home/jamshid/srch2/sharding-demo/srch2-home/"
export ROOT=$SRCH2_HOME
#export ROOT="hello"
export ENV_LOG_ROOT="env-logs"
export ENV_DATA_ROOT="env-data"
export ENV_PULL_LOG="pull-outputs.txt"
export ENV_MAKE_LOG="make-outputs.txt"
export ENV_BIN="bin"
export ENV_CLEAR_EXE="clear.sh"


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
