#!/bin/bash
num_ws=8
root=~
desktop_offset=1
desktop_W=`wmctrl -d | head -n 1 | cut -d ' ' -f 12 | cut -d 'x' -f 1`
desktop_H=`wmctrl -d | head -n 1 | cut -d ' ' -f 12 | cut -d 'x' -f 2`
remove_only=0
if [ "$#" = "1" ]; then
   remove_only=$1
fi


for i in `seq 1 7`
do
   wmctrl -c Node_Run_$i
#   wmctrl -c Node_Run_$i
#   wmctrl -c Node_Run_$i
done

if [ "$remove_only" = "1" ]; then
   echo "Node windows are closed."
   exit
else
   echo "Running nodes ..."
fi

for i in `seq 1 7`
do
   gnome-terminal --title Node_Run_$i -x sh -c 'cd '$root/'workspace-srch2-'$i'/srch2-ngn/build;\
echo "run" | gdb --args ./src/server/srch2-search-server --config='$root/'/workspace-srch2-'$i'/srch2-ngn/test/sharding/integration/conf_'$i'.xml;\
exec bash' &
done


sleep 1
echo "Rearranging Nodes ..."
for i in `seq 1 7`
do
   newX=$(( (i % 4) * desktop_W + desktop_W / 3))
   newY=$(( (i / 4) * desktop_H + 2 * (desktop_H / 3)  ))
   echo $newX;
   echo $newY;
   wmctrl -r Node_Run_$i -e 0,$newX,$newY,-1,-1
   #wmctrl -r Node_Run_$i -b add,maximized_vert,maximized_horz
   #sleep 1
done
