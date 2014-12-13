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
   gnome-terminal --title "Node_"$i -x sh -c 'cd workspace-srch2-'$i';\
git clone git@bitbucket.org:srch2inc/srch2-ngn.git;\
cd srch2-ngn;\
sh ./runme-to-init-env.sh;\
sh eclipse-bootstrap.sh;\
git fetch;\
git checkout sharding-merge-master;\
cd build;\
cmake -DBUILD_RELEASE=0 -DBUILD64BIT=1 ..;\
make;\
exec bash' &
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
   wmctrl -r "Node_"$i -e 0,$newX,$newY,-1,-1
   #sleep 1
done
