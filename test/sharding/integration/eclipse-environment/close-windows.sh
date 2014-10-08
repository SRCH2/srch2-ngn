#!/bin/bash
. ./env-constants.sh
. ./env-util.sh

for i in `seq $NODE_START $NODE_END`
do
   wmctrl -c $1$i
done

