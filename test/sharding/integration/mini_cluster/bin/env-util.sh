#!/bin/bash
# Constants

_ROOT()
{
   echo $SRCH2_HOME
}

_ENV_LOG_ROOT()
{
   echo $( _ROOT)/$ENV_LOG_ROOT
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
   echo $( _ROOT)/$ENV_DATA_ROOT
}

_ENV_BIN()
{
   echo $( _ROOT)/$ENV_BIN
}

_ENV_CLEAR()
{
   echo $(_ENV_BIN)/$ENV_CLEAR_EXE
}

_SRCH2_HOME()
{
   echo $SRCH2_HOME
}

_CLUSTER_NAME()
{
   echo $(_ROOT)/$CLUSTER_NAME
}

_DATA_DIR()
{
   echo $(_ENV_DATA_ROOT)/$DATA_CORE_NAME
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

_ENV_LOG_ROOT()
{
 echo $(_SRCH2_HOME)/$ENV_LOG_ROOT
}

_ENGINE_BINARY()
{
  echo $BINARY_FILE
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

# consule_log group_name node_index
_CONSULE_LOG()
{
   echo $(_ENV_LOG_ROOT)/consule-$1-$2.log
}

_RUN_INFO_LOG()
{
   echo $(_ENV_LOG_ROOT)/run-info-$1-$2.log
}

# $1 : group-name
_KILL_COMMAND()
{
   echo $(_ENV_BIN)/kill-all-nodes.sh
}

# $1 is string to explode, $2 is the delimiter, default value is space

_GET_Ith_TOKEN()
{
str=$1
idx=$2
if [ $3 ]
then
    IFS=$3
else
    IFS=' '
fi
read -a strArray <<< $str
echo ${strArray[$idx]}
}

_WIN_XY()
{
   # $1 is the index of this node
   # desktop assumption : R(4) x C(2)
   # list of ids => desktop offset rxc
   # 0,1,2,3 =>     desktop offset 0,0
   # 4,5,6,7 =>     desktop offset 1,0
   # 8,9,10,11 =>   desktop offset 2,0
   # 12,13,14,15 => desktop offset 3,0
   # 16,..,19    => desktop offset 0,1
   # 20,..,23    => desktop offset 1,1
   # 24,..,27    => desktop offset 2,1
   # 28,..,31    => desktop offset 3,1   => c = (id / WIN_PER_DESKTOP) / DESKTOP_ROW_SIZE
   #                                     => r = (id / WIN_PER_DESKTOP) % DESKTOP_ROW_SIZE 
   # desktop_x = r * DESKTOP_W
   # desktop_y = c * DESKTOP_H   => desktop_x and desktop_y are coordinates of top-left point of desktop
   # win_x = desktop_x + ((id % WIN_PER_DESKTOP) % 2)*WIN_W
   # win_y = desktop_y + ((id % WIN_PER_DESKTOP) / 2)*WIN_H 
   id=$1
   desktop_col=$(( (id / WIN_PER_DESKTOP) / DESKTOP_ROW_SIZE ))
   desktop_row=$(( (id / WIN_PER_DESKTOP) % DESKTOP_ROW_SIZE ))
   desktop_x=$(( desktop_row * DESKTOP_W ))
   desktop_y=$(( desktop_col * DESKTOP_H ))
   win_x=$(( desktop_x + ( (id % WIN_PER_DESKTOP) % 2 ) * WIN_W ))
   win_y=$(( desktop_y + ( (id % WIN_PER_DESKTOP) / 2 ) * WIN_H ))
   #$desktop_x + ( ($id % $WIN_PER_DESKTOP) % 2 ) * $WIN_W
   echo $win_x,$win_y
}

_ARRANGE_WIN()
{
   # $1 title prefix
   # $2 id
   win_xy=$( _WIN_XY $2 )
   echo "wmctrl -r $1""$2 -e 0,$win_xy,-1,-1"
}

INSTALL_DIR=$( pwd )

