#!/bin/bash
num_ws=8
root=~
desktop_offset=1
desktop_W=`wmctrl -d | head -n 1 | cut -d ' ' -f 12 | cut -d 'x' -f 1`
desktop_H=`wmctrl -d | head -n 1 | cut -d ' ' -f 12 | cut -d 'x' -f 2`

echo "Desktop size detected : ($desktop_W,$desktop_H)"
cd $root
#for i in `seq 0 7`

for i in `seq 4 7`
do
   echo "Preparing Node "$i" ..."
   gnome-terminal --title Node__$i -x sh -c 'cd workspace-srch2-'$i'/srch2-ngn/build;\
echo "Branch name : ";git branch|grep "^*";\
exec bash' &
   eclipse -data /home/jamshid/workspace-srch2-$i/ &
   #gnome-terminal --title "Node_"$i -x sh -c '"'$workspace_init_job'"' &
   sleep 1
done
sleep 1
echo "Rearranging Nodes ..."
for i in `seq 4 7`
do
   newX=$(( (i % 4) * desktop_W + desktop_W / 3))
   newY=$(( (i / 4) * desktop_H + 2 * (desktop_H / 3)  ))
   echo $newX;
   echo $newY;
   wmctrl -r Node__$i -e 0,$newX,$newY,-1,-1
   wmctrl -r Node__$i -b add,maximized_vert,maximized_horz
   #sleep 1
done
