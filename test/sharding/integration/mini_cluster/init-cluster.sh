#!/bin/bash
. ./cluster_config.sh

##############################################################
# Files that must be copied to each machine in install process
__INSTALL_FILES_SCRIPTS=( run_cluster_command \
                  init-group.sh \
                  env-constants.sh \
                  env-util.sh \
                  generateConfigFile.py \
                  insertReqGenerator.py \
                  init-env.sh \
                  branch-op.sh \
                  run-group.sh \
                  valgrind-group.sh \
                  generateFrontendConfig.py \
                  frontend \
                  monitor.sh \
                  suppressions_srch2.supp \
                  clean-group.sh)
__INSTALL_FILES_DATA=( $__LICENSE_FILE \
                  $__STOP_WORDS \
                  $__PROTECTED_WORDS \
                  conf-template.xml)
###############################################################
formatAll=0
forceUpdate=0
for var in "$@"
do
   case $var in
   -a)
      formatAll=1
      continue 
   ;;
   -na)
      formatAll=0
      continue
   ;;
   -f)
      forceUpdate=1
      continue
   ;;
   -nf)
      forceUpdate=0
      continue
   ;;
   *)
      echo "Option not recognized.\nUsage : bash init-cluster.sh [-a{format all}] [-f{force update all}]"
      exit
   ;;
   esac
done
#echo "Read args are $1 and $2"
#echo "Args are $formatAll and $forceUpdate ."
echo "Going to copy modified(new) scripts on all clients ..."
for i in "${__GROUPS[@]}"
do
   if [[ $formatAll -eq 1 ]] ; then
      ssh ${__LOGIN_USERS[$i]}@${__IP_ADDRESSES[$i]} rm -r ${__SRCH2_HOMES[$i]}
   fi
   ssh ${__LOGIN_USERS[$i]}@${__IP_ADDRESSES[$i]} mkdir -p ${__SRCH2_HOMES[$i]}
   ssh ${__LOGIN_USERS[$i]}@${__IP_ADDRESSES[$i]} mkdir -p ${__SRCH2_HOMES[$i]}/$__BIN_DIR_NAME
   for f in "${__INSTALL_FILES_SCRIPTS[@]}"
   do
      ###### Only upload the modified scripts  ######
      if [ -z `find ./$__BIN_DIR_NAME/$f -newer .lastChangeTime` ] && [ $forceUpdate -eq 0 ] ; then
         continue
      fi
      ###### Transferring the file ######
      #echo "File ./$__BIN_DIR_NAME/$f has modified." 
      if [[ -d ./$__BIN_DIR_NAME/$f ]]; then
          scp -r ./$__BIN_DIR_NAME/$f ${__LOGIN_USERS[$i]}@${__IP_ADDRESSES[$i]}:${__SRCH2_HOMES[$i]}/$__BIN_DIR_NAME
      else
          scp ./$__BIN_DIR_NAME/$f ${__LOGIN_USERS[$i]}@${__IP_ADDRESSES[$i]}:${__SRCH2_HOMES[$i]}/$__BIN_DIR_NAME
      fi
   done
   for f in "${__INSTALL_FILES_DATA[@]}"
   do
      ###### Only upload the modified scripts  ######
      if [ -z `find ./$f -newer .lastChangeTime` ] && [ $forceUpdate -eq 0 ] ; then
         continue
      fi
      ###### Transferring the file ######
      #echo "File ./$f has modified." 
      scp ./$f ${__LOGIN_USERS[$i]}@${__IP_ADDRESSES[$i]}:${__SRCH2_HOMES[$i]}
   done
done
touch -m .lastChangeTime
echo ${__IP_ADDRESSES[*]}
