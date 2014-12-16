#!/bin/bash
. ../cluster_config.sh

for i in "${__GROUPS[@]}"
do
        ssh ${__LOGIN_USERS[$i]}@${__IP_ADDRESSES[$i]} 'mkdir -p ~/.ssh'
	cat ~/.ssh/id_rsa.pub | ssh ${__LOGIN_USERS[$i]}@${__IP_ADDRESSES[$i]} 'cat >> ~/.ssh/authorized_keys'
done
