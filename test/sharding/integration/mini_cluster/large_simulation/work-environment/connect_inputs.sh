#!/bin/bash
. ../cluster_config.sh

if [[ $# -gt 0 ]]; then
	i=$1
        IP=${__IP_ADDRESSES[$i]}
        MASTER_IP=${__IP_ADDRESSES[$__WORK_ENVIRONMENT_MASTER]}
        LOGIN_USER=${__LOGIN_USERS[$i]}
	if [[ $__WORK_ENVIRONMENT_MASTER == $i ]] ; then
		ssh $LOGIN_USER@$IP -C killall -9 synergys
		ssh $LOGIN_USER@$IP -C "synergys --name $IP -c $__SYNERGY_CONF_PATH &"
		echo "IP $IP is the master - synergy server started successfully."
	else
		ssh $LOGIN_USER@$IP -C killall -9 synergyc
		ssh $LOGIN_USER@$IP -C "synergyc --name $IP $MASTER_IP &"
		echo "IP $IP synergy client started successfully."
	fi
	exit
fi

for i in "${__GROUPS[@]}" ; do
	IP=${__IP_ADDRESSES[$i]}
        MASTER_IP=${__IP_ADDRESSES[$__WORK_ENVIRONMENT_MASTER]}
        LOGIN_USER=${__LOGIN_USERS[$i]}
        if [[ $__WORK_ENVIRONMENT_MASTER == $i ]] ; then
		ssh $LOGIN_USER@$IP -C killall -9 synergys
		ssh $LOGIN_USER@$IP -C "synergys --name $IP -c $__SYNERGY_CONF_PATH &"
		echo "IP $IP is the master - synergy server started successfully."
	else
		ssh $LOGIN_USER@$IP -C killall -9 synergyc 
		ssh $LOGIN_USER@$IP -C "synergyc --name $IP $MASTER_IP &"
		echo "IP $IP synergy client started successfully."
	fi
done

