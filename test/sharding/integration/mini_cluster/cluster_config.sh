#!/bin/bash
__DATA_FILE="file.json"
__CORE_NAME="stackoverflow"
__LICENSE_FILE="srch2_license_key.txt"
__STOP_WORDS="stop-words.txt"
__PROTECTED_WORDS="srch2_protected_words.txt"
__DISCOVERY_PORT=54000
__BIN_DIR_NAME="bin"
__CONF_TEMP_BASE_NAME="conf-template-base.xml"
__CONF_TEMP_NAME="conf-template.xml"

__IP_ADDRESSES=(192.168.1.200 \
              192.168.1.201 \
              192.168.1.202 \
              192.168.1.203 \
              192.168.1.204 \
              192.168.1.205 \
              192.168.1.206)
__GROUP_NAMES=(group-200 \
              group-201 \
              group-202 \
              group-203 \
              group-204 \
              group-205 \
              group-206)
__GROUPS=(0 1 2 3 4 5 6)
__LOGIN_USERS=(jamshid \
              jamshid \
              jamshid \
              jamshid \
              jamshid \
              jamshid \
              jamshid)
__SRCH2_HOMES=(/home/jamshid/mini_cluster \
              /home/jamshid/mini_cluster \
              /home/jamshid/mini_cluster \
              /home/jamshid/mini_cluster \
              /home/jamshid/mini_cluster \
              /home/jamshid/mini_cluster \
              /home/jamshid/mini_cluster)
### node information of each machine
__GROUP_PROCESS_COUNTS=(4 4  \
                   1 \
                   4 \
                   4 \
                   4 \
                   4)
__DATA_FILE_REL_PATHS=( data \
                     data \
                     data \
                     data \
                     data \
                     data \
                     data )
__LOG_DIR_REL_PATHS=( ./logs/ \
                    ./logs/ \
                    ./logs/ \
                    ./logs/ \
                    ./logs/ \
                    ./logs/ \
                    ./logs/ )
__SRC_DIRS=( /home/jamshid/src/srch2-ngn \
             /home/jamshid/src/srch2-ngn \
             /home/jamshid/src/srch2-ngn \
             /home/jamshid/workspace-srch2-v4/srch2-ngn \
             /home/jamshid/src/srch2-ngn \
             /home/jamshid/src/srch2-ngn \
             /home/jamshid/src/srch2-ngn )
__BINARY_FILES=( /home/jamshid/src/srch2-ngn/build/src/server/srch2-search-server \
             /home/jamshid/src/srch2-ngn/build/src/server/srch2-search-server \
             /home/jamshid/src/srch2-ngn/build/src/server/srch2-search-server \
             /home/jamshid/workspace-srch2-v4/srch2-ngn/build/src/server/srch2-search-server \
             /home/jamshid/src/srch2-ngn/build/src/server/srch2-search-server \
             /home/jamshid/src/srch2-ngn/build/src/server/srch2-search-server \
             /home/jamshid/src/srch2-ngn/build/src/server/srch2-search-server )
__DBUILD64BITiS=(0 0 1 1 0 1 0)






############################# Work Environment #############################
############# NOTE : This section must not be shipped with the product #####


__WORK_ENVIRONMENT_MASTER=4
__SYNERGY_CONF_PATH=/etc/synergy.conf
