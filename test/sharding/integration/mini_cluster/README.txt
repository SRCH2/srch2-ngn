# 1. File explaination

## close window with title T
bash close-windows.sh T

## The template we use for generating the configuration file.
bash conf-template.xml

## This script defines constants needed in installation and run scripts. It must be included in the beginning of scripts.
bash env-constants.sh

## This file contains some bash script utility functions.
bash env-util.sh

## This python script generates the config the I_th config file of a group based on the the TEMPLATE config file and it uses SRCH2_HOME, DATA_FILE_PATH and LOG_FILE_PATH for the values of corresponding tags.
python generateConfigFile.py TEMPLATE I SRCH2_HOME DATA_FILE_PATH LOG_FILE_PATH

## This python script generates batch files of insert and delete requests from the json input data. TODO : under construction
python insertReqGenerator.py

## This script compiles the code in all workspaces
bash make-workspaces.sh

## This script makes the directory structure : TODO : construction
bash mkdir-workspaces.shi

## This script can be used to open per workspace windows in a node
bash open-workspaces.sh

## This script pulls git changes into all workspaces
bash pull-workspaces.sh

## This script starts all engines in gdb in separate terminals and arranges the terminals.
bash start-debuggers.sh

## This script removes all workspaces
bash remove-workspaces.sh

## This script opens and arrange all necessary windows for debugging for each workspace.
bash run-workspaces.sh

## This script removes all the folder structure created in installation process.
bash uninstall-env.sh

## cbash, for 'cluster bash'. This command has two modes. 
## 1 == Bash command mode
./cbash "mkdir -p example"
## 2 == Node side script mode
## ./path/to/command.sh must be added to the array in the beginning of init-cluster.sh 
## so that initializer copies the file on the group srch2Home bin directory. The root 
## from which this script is executed is srch2Home.
./cbash -s "bash ./path/to/command.sh"


# 2. Set up the environment:

## Branch : sharding-merge-master

## Prerequisite software:
   
   Synaptic : The following app can be found in Synaptic.
   CompizConfig Settings Manager -> General -> General Options -> Desktop Size 
      Set Horizontal Virtual Size = 5, Vertical Virtual Size = 3
      Use Ctrl + Alt + Arrow keys to switch between desktops

   apache2 
   apache2-mod-php5
   php5
   curl
   php-curl
   synergy
   git
   vim
   eclipse
   ssh
   chrome

   To check whether the apache is up:
      open chrome -> go to localhost -> You should see "It works!" 

## Workspace : 

   Admin operation workspace :  
      srch2-ngn/test/sharding/integration/mini_cluster/     
   Client operation workspace (The folder will be created after installing the cluster): 
      ~/mini_cluster/ 

## Connect all machines in a local network
   Network Connections -> Add -> Ethernet :
      Connection Name : Cluster
      Ethernet :
         Device MAC address : choose one
      IPv4 Settings : 
         Method : Manual
         Address -> Add
            Address : 192.168.1.203 
            Netmask : 255.255.255.0
            Gateway : 0.0.0.0
      
   Ping all machines to check if connected (Don't forget to ping yourself)

   Modify/Check the configuration file "cluster_config.sh" for the admin machine 
      This config file will be used by all admin bash scripts (The scripts under mini_cluster exclude those under "bin" folder)

      shell> vim srch2-ngn/test/sharding/integration/mini_cluster/cluster_config.sh

      Add each machine's information to the config file.

      To run the cluster on a single local machine:

      shell> mv srch2-ngn/test/sharding/integration/mini_cluster/cluster_config.sh srch2-ngn/test/sharding/integration/mini_cluster/cluster_config.sh.cluster
      shell> srch2-ngn/test/sharding/integration/mini_cluster/cluster_config.sh.local srch2-ngn/test/sharding/integration/mini_cluster/cluster_config.sh
      shell> vim srch2-ngn/test/sharding/integration/mini_cluster/cluster_config.sh
      
      Change the __IP_ADDRESSES, __LOGIN_USERS, __SRCH2_HOMES, __SRC_DIRS, __BINARY_FILES to your machine setting.

   Modify/Check the configuration file "bin/env-constants.sh" for the node machine
      This config file will be used by all scripts under "bin" folder

      shell> vim srch2-ngn/test/sharding/integration/mini_cluster/bin/env-constants.sh

   Modify/Check the configuration file "bin/frontend/cluster_config.txt" for the node machine frontend
      This config file will be used by the frontend GUI

      shell> vim srch2-ngn/test/sharding/integration/mini_cluster/bin/frontend/cluster_config.txt
 
   Add your public key to the local network machines:
      shell> cd work-evnironment
      shell> bash add_self_to_authorized_keys.sh 

   Connect input device using synergy
      shell> bash connect_input.sh (You may need to run this command twice)

## Install the mini_cluster on all nodes

   shell> bash install -f 
   You can also use this command to update the scripts on all nodes.

## Set up the frontend GUI

   shell> cd /var/www/html
   shell> sudo ln -s ~/mini_cluster/bin/frontend . (The folder will be created after installing the cluster)

   Now you can see the GUI on chrome browser with address "http://localhost/frontend/getNodeInfo.php"

## Run the commands on the cluster
   shell> bash cbash [-n] [-s] -c "your command"
   [-n] : Node that will run the command. 
          For example : 
             "-n 1" means the command will only run on the node 1.
             "-n 1,2,5" means the command will only run on the node 1,2 and 5.
             If "-n" is not defined, the command will run on all nodes.
   [-s] : The command will run the script with passing the predefined variables
             These variables are available to this script :  (values are example)
                __IP_ADDRESS=192.168.1.204
                __GROUP_NAME=group-204
                __GROUP=3
                __LOGIN_USER=jamshid
                __SRCH2_HOME=/home/jamshid/mini_cluster
                __GROUP_PROCESS_COUNT=3
                __DATA_FILE_REL_PATH=./data
                __LOG_DIR_REL_PATH=./logs
                __CORE_NAME=stackoverflow 
                __SRC_DIR=up to srch2-ngn folder
                __DBUILD64BIT=0 or 1 depending on 32 or 64 bit
                __BIN_DIR="bin"

             For the completed list of variables, check "cluster_config.sh"

          Note that the path is relative to __SRCH2_HOME defined in "cluster_config.sh"

   [-c] : The command or script to be run on the cluster
          For example : 
             shell> bash cbash -c "mkdir temp"
             This command will make a folder "temp" under "~/"   

   Here are some useful commands by using cbash

      To start nodes using "bin/run-group.sh" script:
	 shell> bash cbash [-n] -s -c "bash bin/run-group.sh"

      To stop nodes:
	 shell> bash cbash [-n] -c "killall -9 srch2-search-server"

      To clean log files using "bin/clean_group" script:
	 shell> bash cbash [-n] -s -c "bash bin/clean-group.sh -l"

      To clean indexes files using "bin/clean_group" script:
	 shell> bash cbash [-n] -s -c "bash bin/clean-group.sh -cr"

      To backup the current log files under a backup folder and clean the log files files using "bin/clean_group" script:
	 shell> bash cbash [-n] -s -c "bash bin/clean-group.sh -b"

# 3. Start the cluster:
   a. Make sure the setting in the file "cluster_config.sh" is correct. 
   a. Ping all nodes in the cluster
   b. Connect the input device
         shell> cd work-environment
         shell> bash connect_inputs.sh
   c. Update the scripts on all nodes
         shell> bash install
   d. Start the engine
         shell> bash cbash -s -c "bash bin/run-group.sh &"
   e. Start the GUI on each machine to monitor the group
         Open chrome browser on eache machine 
   f. Insert records
         shell> bash cbash -s -c "bash bin/record-op.sh &"
   g. Stop the engine
         shell> bash cbash -c "killall -9 srch2-search-server"
   h. Clean the log and indexes
         shell> bash cbash -s -c "bash bin/clean-group.sh -l &" 
         shell> bash cbash -s -c "bash bin/clean-group.sh -cr &" 
