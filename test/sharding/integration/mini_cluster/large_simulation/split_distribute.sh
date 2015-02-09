#!/bin/bash
. ./cluster_config.sh
# Note : target must be an array of meaningful 'paths' to directories which will contains the chucks
#        these paths can also be ones on a remote computer like : b@bob.com:~/data_directory/
#
#############

############# read the arguments #################

__abort()
{
    echo "Options: ";
    echo "--data[-d] inputDataFile "; 
    echo "--prefix[-p] outputNamePrefix"; 
    echo "--suffix[-s] outputNameSuffix";
    echo "--delete";
    exit
}
outputNamePrefix='data'
outputNameSuffix='.dataset'
inputFileName=""
deleteFiles=0
while [ $1 ] ; do
    case $1 in
    '--prefix');&
    '-p')
	outputNamePrefix=$2
        shift
        continue
    ;;
    '--suffix');&
    '-s')
        outputNameSuffix=$2
        shift
        continue
    ;;
    '--data');&
    '-d')
        inputFileName=$2
        shift
        continue
    ;;
    '--delete')
        deleteFiles=1
        shift
        continue
    ;;
    esac
    shift
done
#echo "Input arguments are : outputNamePrefix = $outputNamePrefix and outputNameSuffix = $outputNameSuffix"
############# split the inputDataFile into N equal chunks and scp them into targets #####################
totalNumberOfNodes=0
export targetPaths=""
export remoteAddresses=""
for i in "${__GROUPS[@]}" ; do
   GROUP_IDX=${__GROUPS[$i]}
   GROUP_DATA_DIR="${__SRCH2_HOMES[$GROUP_IDX]}/${__DATA_FILE_REL_PATHS[$GROUP_IDX]}"
   echo GROUP_IDX $GROUP_IDX AND GROUP_DATA_DIR $GROUP_DATA_DIR
   for processIdx in `seq 1 ${__GROUP_PROCESS_COUNTS[$GROUP_IDX]}` ;
   do
      remoteAddresses="$remoteAddresses ${__LOGIN_USERS[$GROUP_IDX]}@${__IP_ADDRESSES[$GROUP_IDX]}"
      targetPaths="$targetPaths $GROUP_DATA_DIR/$outputNamePrefix-${__GROUP_NAMES[$GROUP_IDX]}-$processIdx$outputNameSuffix"
      if [ $deleteFiles == 1 ] ; then
          ssh ${__LOGIN_USERS[$GROUP_IDX]}@${__IP_ADDRESSES[$GROUP_IDX]} -C "rm -f $GROUP_DATA_DIR/$outputNamePrefix-${__GROUP_NAMES[$GROUP_IDX]}-$processIdx$outputNameSuffix"
      fi
      ((totalNumberOfNodes++))
   done
done
if [ $deleteFiles == 1 ] ; then
   echo "$totalNumberOfNodes files are deleted successfully."
   exit
fi
echo totalNumberOfNodes $totalNumberOfNodes

if [ $deleteFiles == 0 ] ; then
   split -d -n l/$totalNumberOfNodes $inputFileName "" --filter="IFS=' ' read -a remoteArray <<< \"\$remoteAddresses\"; IFS=' ' read -a targetArray <<< \"\$targetPaths\"; export targetPath=\${targetArray[\$((10#\$FILE))]}; cat \$file | ssh \${remoteArray[\$((10#\$FILE))]} -C \"cat > \$targetPath\"; echo \"File \$targetPath is copied to \${remoteArray[\$((10#\$FILE))]}.\""
fi
#split -d -n l/$totalNumberOfNodes $inputFileName "" --filter="cat \$file"
