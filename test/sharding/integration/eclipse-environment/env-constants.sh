#!/bin/bash
if [ -z "$CONSTANTS_DEFINED"  ]; then
##############################################################################
echo "Exporting constants ..."
export NODE_START=1
export NODE_END=7

export LICENSE_FILE="srch2_license_key.txt"
export STOP_WORDS="stop-words.txt"
export PROTECTED_WORDS="srch2_protected_words.txt"
export LOG_FILE_PREFIX="srch2-log"
export DATA_SOURCE="/home/jamshid/srch2/sharding-demo/data/"
export DATA_CORE_NAME="statemedia"
export DATA_FILE_PREFIX="statemedia-1000"
export SRCH2_HOME="srch2-home"


# config file constants
export CONF_FILE_PREFIX="conf-"
export CLUSTER_NAME="SRCH2_Cluster"
export NODE_NAME_PREFIX="Node-"
export TM_PORT_BASE=4000
export TM_IP_ADDRESS="127.0.0.1"
export WELL_KNOWN_HOSTS="127.0.0.1:54000"
export EXT_HOSTNAME="127.0.0.1"
export EXT_PORT_BASE=7000
export NUM_PARTITIONS=8
export NUM_REPLICAS=1


# environment installation variables

#export ROOT="/home/jamshid/srch2/sharding-demo/"
export ROOT="hello"
if [! -d "$ROOT"]; then
   echo "Root ($ROOT) does not exist."
   exit
else
   cd $ROOT
   ROOT=$( pwd )
fi
export ENV_LOG_ROOT="env-logs"
export ENV_DATA_ROOT="env-data"
export ENV_PULL_LOG="pull-outputs.txt"
export ENV_MAKE_LOG="make-outputs.txt"
export ENV_BIN="bin"
export ENV_CLEAR_EXE="clear.sh"
############################################################################
export CONSTANTS_DEFINED="SET"
fi
