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
