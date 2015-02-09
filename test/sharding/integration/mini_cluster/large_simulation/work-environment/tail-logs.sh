#!/bin/bash
. ../cluster_config.sh

group_positions=( "0,3225,0,614,493" \
                  "0,2572,0,650,493" \
                  "0,1922,0,650,493" \
                  "0,3234,550,605,493" \
                  "0,2571,550,659,493" \
                  "0,1920,550,650,494" \
                  "0,1044,0,650,493" )

if [[ $1 == "-k" ]]; then
	for groupName in "${__GROUP_NAMES[@]}"
	do
		wmctrl -c $groupName
	done
	exit
fi

for i in "${__GROUPS[@]}" ; do
   GROUP_IDX=${__GROUPS[$i]}
   GROUP_DATA_DIR="${__SRCH2_HOMES[$GROUP_IDX]}/${__DATA_FILE_REL_PATHS[$GROUP_IDX]}"
   GROUP_LOG_DIR="${__SRCH2_HOMES[$GROUP_IDX]}/${__LOG_DIR_REL_PATHS[$GROUP_IDX]}"
   echo GROUP_IDX $GROUP_IDX, GROUP_DATA_DIR $GROUP_DATA_DIR, GROUP_LOG_DIR $GROUP_LOG_DIR
   targetPaths=""
   mtailInput=""
   for processIdx in `seq 1 ${__GROUP_PROCESS_COUNTS[$GROUP_IDX]}` ;
   do
      targetPath=$GROUP_LOG_DIR/srch2-log-$processIdx.txt
      targetPaths="$targetPaths $GROUP_LOG_DIR/srch2-log-$processIdx.txt"
      mtailInput="$mtailInput -l \"ssh -n ${__LOGIN_USERS[$GROUP_IDX]}@${__IP_ADDRESSES[$GROUP_IDX]} -C tail -f $targetPath\""
   done
   echo "multitail $mtailInput" > log-tail-${__GROUP_NAMES[$GROUP_IDX]}.sh
   #gnome-terminal --profile=${__GROUP_NAMES[$GROUP_IDX]} -e multitail $mtailInput
   #echo gnome-terminal --profile=${__GROUP_NAMES[$GROUP_IDX]} -e multitail $mtailInput
done

for i in "${__GROUPS[@]}" ; do
   GROUP_IDX=${__GROUPS[$i]}
   gnome-terminal --profile=${__GROUP_NAMES[$GROUP_IDX]} -e "bash log-tail-${__GROUP_NAMES[$GROUP_IDX]}.sh" &
   echo gnome-terminal --profile=${__GROUP_NAMES[$GROUP_IDX]} -e "bash log-tail-${__GROUP_NAMES[$GROUP_IDX]}.sh"
done
sleep 3
for i in "${__GROUPS[@]}"
do
        GROUP_IDX=${__GROUPS[$i]}
	wmctrl -r ${__GROUP_NAMES[$GROUP_IDX]} -e ${group_positions[$GROUP_IDX]}
	#echo wmctrl -r ${__GROUP_NAMES[$i]} -e ${group_positions[$i]}
done
