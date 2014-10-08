#!/bin/bash
. ./env-constants.sh
. ./env-util.sh

for i in `seq $NODE_START $NODE_END`
do
   command='cd '$BINARY_DIR';echo "run" | gdb --args '$BINARY_NAME' --config='$( _CONF_FILE $i )';exec bash'
   #echo $command
   gnome-terminal --title Node_Run_$i -x sh -c "$command" &
done

sleep 1
for i in `seq $NODE_START $NODE_END`
do
   _ARRANGE_WIN Node_Run_ $i
done
