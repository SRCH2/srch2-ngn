#!/bin/bash
. ./env-constants.sh
. ./env-util.sh

cd $ROOT

rm -r $(_ENV_LOG_ROOT)
rm -r $(_ENV_DATA_ROOT)
rm -r $(_ENV_BIN)
rm -r $(_SRCH2_HOME)
rm $ROOT/$LICENSE_FILE
rm $ROOT/$STOP_WORDS
rm $ROOT/$PROTECTED_WORDS 
