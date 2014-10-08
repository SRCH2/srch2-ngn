#!/bin/bash
# Constants
. ./env-constants.sh

_ENV_LOG_ROOT()
{
   echo $ROOT/$ENV_LOG_ROOT
}

_ENV_PULL_LOG()
{
   echo $( _ENV_LOG_ROOT )/$ENV_PULL_LOG
}

_ENV_MAKE_LOG()
{
   echo $( _ENV_LOG_ROOT )/$ENV_MAKE_LOG
}

_ENV_DATA_ROOT()
{
   echo $ROOT/$ENV_DATA_ROOT
}

_ENV_BIN()
{
   echo $ROOT/$ENV_BIN
}

_ENV_CLEAR()
{
   echo $(_ENV_BIN)/$ENV_CLEAR_EXE
}

_SRCH2_HOME()
{
   echo $ROOT/$SRCH2_HOME
}

_DATA_DIR()
{
   echo $( _SRCH2_HOME )/$DATA_CORE_NAME
}

_DATA_DIR_REL()
{
   echo ./$DATA_CORE_NAME
}

_DATA_FILE()
{
   echo $( _DATA_DIR )/$DATA_FILE_PREFIX"-"$1".json"
}

_DATA_FILE_REL()
{
   echo $( _DATA_DIR_REL )/$DATA_FILE_PREFIX"-"$1".json"
}

_DATA_SOURCE_FILE()
{
   echo $DATA_SOURCE/$DATA_FILE_PREFIX"-"$1".json"
}

_LOG_FILE()
{
   echo $( SRCH2_HOME )/$LOG_FILE_PREFIX"-"$1".txt"
}

_LOG_FILE_REL()
{
   echo ./$LOG_FILE_PREFIX"-"$1".txt"
}

_CONF_FILE()
{
   echo $(_SRCH2_HOME)/$CONF_FILE_PREFIX""$1".xml"
}

INSTALL_DIR=$( pwd )

