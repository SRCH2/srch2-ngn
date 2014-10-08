#!/bin/bash
. ./env-constants.sh
. ./env-util.sh

#if [ -d "$ROOT"]; then
#   cd $ROOT
#   ROOT=$( pwd )
#else
#   echo "Root ($ROOT) does not exist."
#   exit 1
#fi
cd $ROOT
ROOT=$( pwd )
#Making directory structure and initializing env files
mkdir $(_ENV_LOG_ROOT)
touch $(_ENV_PULL_LOG)
touch $(_ENV_MAKE_LOG)
mkdir $(_ENV_DATA_ROOT)
mkdir $(_ENV_BIN)
# copy necessary files here
echo "Copying necessary files ..."
cp $INSTALL_DIR/$LICENSE_FILE $ROOT/$LICENSE_FILE
cp $INSTALL_DIR/$STOP_WORDS $ROOT/$STOP_WORDS
cp $INSTALL_DIR/$PROTECTED_WORDS $ROOT/$PROTECTED_WORDS

echo "Preparing directory structure ..."
mkdir $( _SRCH2_HOME )
mkdir $( _DATA_DIR )
# make srch2 home directories
echo "Generating config files and soft links to data source files ..."
for i in `seq $NODE_START $NODE_END`
do
   ln -s $( _DATA_SOURCE_FILE $i ) $( _DATA_FILE $i )
   python $INSTALL_DIR/generateConfigFile.py $INSTALL_DIR/conf-template.xml $i $(_SRCH2_HOME) $( _DATA_FILE_REL $i ) $( _LOG_FILE_REL $i ) > $(_CONF_FILE $i)
done


cp -r $( _SRCH2_HOME ) $(_ENV_DATA_ROOT)/.$SRCH2_HOME-backup
echo "rm -r $( _SRCH2_HOME );cp -r $(_ENV_DATA_ROOT)/.$SRCH2_HOME-backup $( _SRCH2_HOME )" > $(_ENV_CLEAR)

echo "Testing environment is ready to use."
