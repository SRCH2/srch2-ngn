#!/bin/bash
. ./env-constants.sh
. ./env-util.sh

cd $(_ROOT)

rm -r $(_ENV_LOG_ROOT)
rm -r $(_ENV_DATA_ROOT)
rm -r $(_ENV_BIN)
rm $(_ROOT)/$LICENSE_FILE
rm $(_ROOT)/$STOP_WORDS
rm $(_ROOT)/$PROTECTED_WORDS
rm $(_ROOT)/conf-*.xml
rm -r $(_DATA_DIR)
