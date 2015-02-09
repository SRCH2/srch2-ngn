#!/bin/bash
. ./cluster_config.sh

############# split the inputDataFile into N equal chunks and scp them into targets #####################
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
   gnome-terminal -e "multitail $mtailInput"
   echo gnome-terminal -e "multitail $mtailInput"
done
