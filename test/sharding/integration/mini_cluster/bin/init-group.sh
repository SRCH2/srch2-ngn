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
ROOT=$( pwd )

############## Declare constants #####################
. ./$__BIN_DIR/env-constants.sh
. ./$__BIN_DIR/env-util.sh
############### Make the folder structure #############
#Making directory structure and initializing env files
echo "Group $__GROUP_NAME | Preparing the directory structure ..."
#mkdir ./$__DATA_FILE_REL_PATH
#mkdir ./$__DATA_FILE_REL_PATH/$__CORE_NAME
#mkdir ./$__LOG_DIR_REL_PATH
mkdir -p $(_ENV_LOG_ROOT)
touch $(_ENV_PULL_LOG)
touch $(_ENV_MAKE_LOG)
mkdir -p $(_ENV_DATA_ROOT)
mkdir -p $(_DATA_DIR)


# Preparing the source code
#cd $__SRC_DIR
#git clone git@bitbucket.org:srch2inc/srch2-ngn.git
#cd build
#make -j 8


cd $__SRCH2_HOME
# make srch2 home directories
echo "Group $__GROUP_NAME | Generating config files for $NODE_END nodes."
for i in `seq $NODE_START $NODE_END`
do
#   ln -s $( _DATA_SOURCE_FILE $i ) $( _DATA_FILE $i )
   python $INSTALL_DIR/$__BIN_DIR/generateConfigFile.py $__SRCH2_HOME/conf-template.xml $i $(_SRCH2_HOME) $( _DATA_FILE_REL $i ) $( _LOG_FILE_REL $i ) > $(_CONF_FILE $i)
done


### prepare the frontend config file
python $INSTALL_DIR/$__BIN_DIR/generateFrontendConfig.py $__IP_ADDRESS $EXT_PORT_BASE $__GROUP_PROCESS_COUNT > $INSTALL_DIR/$__BIN_DIR/$FRONTEND_CONF_FILE

#echo "bash $INSTALL_DIR/$__BIN_DIR/monitor.sh" >> ~/.bashrc
echo "Group $__GROUP_NAME | Initialization done."
cd $currentPath
