#!/bin/bash
. ./cluster_config.sh

###################
# Files that must be copied to each machine in install process
__INSTALL_FILES_SCRIPTS=( run_cluster_command \
                  init-group.sh \
                  env-constants.sh \
                  env-util.sh \
                  generateConfigFile.py \
                  init-env.sh \
                  git-operation.sh \
                  run-group.sh \
                  generateFrontendConfig.py \
                  frontend \
                  monitor.sh)
__INSTALL_FILES_DATA=( $__LICENSE_FILE \
                  $__STOP_WORDS \
                  $__PROTECTED_WORDS \
                  conf-template.xml)
###################

if [[ $@ -gt 1 ]] ; then
   refreshFlag=$1
else
   refreshFlag=0
fi

echo "Copying needed scripts on all clients ..."
for i in "${__GROUPS[@]}"
do
   if [[ $refreshFlag ]] ; then
      ssh ${__LOGIN_USERS[$i]}@${__IP_ADDRESSES[$i]} rm -r ${__SRCH2_HOMES[$i]}
   fi
   #echo "${__LOGIN_USERS[$i]}@${__IP_ADDRESSES[$i]} mkdir ${__SRCH2_HOMES[$i]}"
   ssh ${__LOGIN_USERS[$i]}@${__IP_ADDRESSES[$i]} mkdir ${__SRCH2_HOMES[$i]}
   ssh ${__LOGIN_USERS[$i]}@${__IP_ADDRESSES[$i]} mkdir ${__SRCH2_HOMES[$i]}/$__BIN_DIR_NAME
   for f in "${__INSTALL_FILES_SCRIPTS[@]}"
   do
      if [[ -d ./$__BIN_DIR_NAME/$f ]]; then
          scp -r ./$__BIN_DIR_NAME/$f ${__LOGIN_USERS[$i]}@${__IP_ADDRESSES[$i]}:${__SRCH2_HOMES[$i]}/$__BIN_DIR_NAME
      else
          scp ./$__BIN_DIR_NAME/$f ${__LOGIN_USERS[$i]}@${__IP_ADDRESSES[$i]}:${__SRCH2_HOMES[$i]}/$__BIN_DIR_NAME
      fi
      #ssh ${__LOGIN_USERS[$i]}@${__IP_ADDRESSES[$i]} $(echo "'chmod 777 ${__SRCH2_HOMES[$i]}/run_cluster_command'")
   done
   for f in "${__INSTALL_FILES_DATA[@]}"
   do
      scp ./$f ${__LOGIN_USERS[$i]}@${__IP_ADDRESSES[$i]}:${__SRCH2_HOMES[$i]}
      #ssh ${__LOGIN_USERS[$i]}@${__IP_ADDRESSES[$i]} $(echo "'chmod 777 ${__SRCH2_HOMES[$i]}/run_cluster_command'")
   done

done
echo ${__IP_ADDRESSES[*]}
