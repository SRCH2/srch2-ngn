#!/bin/bash

PWD_DIR=$(pwd)

if [ $# -lt 2 ]; then
    echo "Usage: $0 [-f] <system-test-directory> <server-executable>"
    exit 1
fi

machine=`uname -m`
os=`uname -s`
macName='Darwin'
LOG_DIRECTORY='./test-data/core1/logs'
# process options
force=0
output='/dev/tty'
html=0
upload=''
while [ "$#" -gt 2 ]
do
    # -f or --force option: continue executing tests even after one fails
    if [ "$1" = '-f' ] || [ "$1" = '--force' ]; then
        force=1
        shift
        continue
    fi

    # --html option: Format output in HTML (for dashboard)
    if [ "$1" = '--html' ]; then
        html=1
        shift
        continue
    fi

    # --outfile option: Where to send output
    # required if also specifying --upload
    if [ "$1" = '-o' ] || [ "$1" = '--output' ] || [ "$1" = '--output-file' ]; then
        output="$2"
        shift 2
        continue
    fi

    # --upload option: Upload output file to dashboard server
    # This curl command uses Content-Type: multipart/form-data
    if [ "$1" = '--upload' ]; then
        if [ "$output" = '/dev/tty' ]; then
            output="report-`date +%F-%R`-${machine}-${os}.html"
        fi
        upload="curl -F \"uploadedfile=@${output}\" \"http://dilbert.calit2.uci.edu/CDash-2-0-2/system_tests_submit.php\""
        shift
        continue
    fi

    echo "$0: Unrecognized option: $1"
    break;
done

# $1 is <srch2-main-dir>/test/wrapper/system_tests
SYSTEM_TEST_DIR=$1
if [ ! -d "$SYSTEM_TEST_DIR" ]; then
    echo "$0: \"$SYSTEM_TEST_DIR\" not an existing directory."
    exit 1
fi
cd $SYSTEM_TEST_DIR

# $2 is <srch2-main-dir>/build/src/server/srch2-search-server
SRCH2_ENGINE=$2
if [ ! -x "$SRCH2_ENGINE" ]; then
    echo "$0: Search engine \"$SRCH2_ENGINE\" not valid."
    exit 1
fi

# Turn off Python stdout buffering so we don't lose output messages
PYTHONUNBUFFERED=True
export PYTHONUNBUFFERED

function printTestBanner {
    testName="$1"
    totalLength=79 # width to make banner
    if [ "$html" -gt 0 ]; then
        html_pre='<h4>'
        html_post='</h4>'
        html_results_link_pre="<a href=\"#${testName}\">"
        html_results_link_post='</a>'
        html_results_anchor="<a name=\"${testName}\"></a>"
        totalLength=$((${totalLength} + ${#html_pre} + ${#html_post} + ${#html_results_link_pre} + ${#html_results_link_post}))
    else
        html_pre=''
        html_post=''
        html_results_link_pre=''
        html_results_link_post=''
        html_results_anchor_pre=''
        html_results_anchor_post=''
    fi
    banner="${html_pre}---------------------do ${html_results_link_pre}${test_id}${html_results_link_post} test"
    while [ "${#banner}" -lt "$totalLength" ]
    do
        banner="${banner}-"
    done
    banner="${banner}${html_post}"
    echo "$banner" >> ${output}
    echo "${html_results_anchor}${banner}" >> ./test-data/core1/logs/system_test.log
}

# Clear output file if file was specified
if [ -f "$output" ]; then
    rm -f "$output"
fi

# setup some common HTML highlighting
if [ "$html" -gt 0 ]; then
    html_fail_pre='<font color="#FF0000">'
    html_fail_post='</font>'

    rm -f ./test-data/core1/logs/system_test.log
    html_title="System Tests - `date` - ${machine} ${os}"
    echo "<html><head><title>${html_title}</title></head><body><h1>${html_title}</h1><h2>Test Summary</h2>" >> ${output}
    html_escape_command="sed -e 's/</\&lt;/g' | sed -e 's/>/\&gt;/g'"
else
    html_fail_pre=''
    html_fail_post=''
    html_escape_command='cat'
fi

echo ''
echo "NOTE: $0 will start numerous instances of the srch2 server.  Pre-existing server processes will interfere with this testing."
echo ''

mkdir -p $LOG_DIRECTORY

modeOfEngine="debug"
transaction1_insert="./nodeIntensive-noValidation/nodeIntensiveTransaction-1-insert.txt"
transaction1_query="./nodeIntensive-noValidation/nodeIntensiveTransaction-1-query.txt"
transaction1_delete="./nodeIntensive-noValidation/nodeIntensiveTransaction-1-delete.txt"
transaction2_insert="./nodeIntensive-noValidation/nodeIntensiveTransaction-2-insert.txt"
transaction2_query="./nodeIntensive-noValidation/nodeIntensiveTransaction-2-query.txt"
transaction2_delete="./nodeIntensive-noValidation/nodeIntensiveTransaction-2-delete.txt"
transaction3_insert="./nodeIntensive-noValidation/nodeIntensiveTransaction-3-insert.txt"
transaction3_query="./nodeIntensive-noValidation/nodeIntensiveTransaction-3-query.txt"
transaction3_delete="./nodeIntensive-noValidation/nodeIntensiveTransaction-3-delete.txt"
transaction4_insert_delete="./nodeIntensive-noValidation/nodeIntensiveTransaction-4-insert-delete.txt"
transaction4_query="./nodeIntensive-noValidation/nodeIntensiveTransaction-4-query.txt"
transaction_insert_delete="./nodeIntensive-noValidation/nodeIntensiveTransaction-insert-delete.txt"
transaction_insert="./nodeIntensive-noValidation/nodeIntensiveTransaction-insert.txt"
transaction_query="./nodeIntensive-noValidation/nodeIntensiveTransaction-query.txt"
transaction_run_extraNode="./nodeIntensive-noValidation/transactionKeepNodeRunning.txt"
transaction_kill_extraNode="./nodeIntensive-noValidation/transactionkillRunningNode.txt"
nodeAddress="./config.json"
pythonCode="./nodeIntensive-noValidation/distributedTestFramework.py"

echo "start Transaction-1"
printTestBanner "Transaction-1" 
python $pythonCode $nodeAddress ./nodeIntensive-noValidation/transaction-1.txt &
python $pythonCode $nodeAddress $transaction1_insert &
python $pythonCode $nodeAddress $transaction1_query &
python $pythonCode $nodeAddress $transaction1_delete &
sleep 120
python killPythonProcess.py 
python killNodes.py 
python killNodesOnDilbert.py 

echo"end Transaction-1"


printTestBanner "Transaction-2"
python $pythonCode $nodeAddress ./nodeIntensive-noValidation/transaction-2.txt &
python $pythonCode $nodeAddress $transaction2_insert &
python $pythonCode $nodeAddress $transaction2_query &
python $pythonCode $nodeAddress $transaction2_delete &
sleep 120
python killPythonProcess.py
python killNodes.py 
python killNodesOnDilbert.py 


printTestBanner "Transaction-3"
python $pythonCode $nodeAddress ./nodeIntensive-noValidation/transaction-3.txt &
python $pythonCode $nodeAddress $transaction3_insert &
python $pythonCode $nodeAddress $transaction3_query &
python $pythonCode $nodeAddress $transaction3_delete &
sleep 120
python killPythonProcess.py
python killNodes.py 
python killNodesOnDilbert.py 


printTestBanner "Transaction-4"
python $pythonCode $nodeAddress ./nodeIntensive-noValidation/transaction-4.txt &
python $pythonCode $nodeAddress $transaction4_insert_delete &
python $pythonCode $nodeAddress $transaction4_query &
sleep 120
python killPythonProcess.py
python killNodes.py 
python killNodesOnDilbert.py 


printTestBanner "Transaction-5"
python $pythonCode $nodeAddress ./nodeIntensive-noValidation/transaction-5.txt &
python $pythonCode $nodeAddress $transaction_insert &
python $pythonCode $nodeAddress $transaction_query &
sleep 120
python killPythonProcess.py
python killNodes.py 
python killNodesOnDilbert.py 


printTestBanner "Transaction-6"
python $pythonCode $nodeAddress ./nodeIntensive-noValidation/transaction-6.txt &
python $pythonCode $nodeAddress $transaction_insert &
python $pythonCode $nodeAddress $transaction_query &
sleep 120
python killPythonProcess.py
python killNodes.py 
python killNodesOnDilbert.py 


printTestBanner "Transaction-7"
python $pythonCode $nodeAddress ./nodeIntensive-noValidation/transaction-7.txt &
python $pythonCode $nodeAddress $transaction_insert &
python $pythonCode $nodeAddress $transaction_query &
sleep 120
python killPythonProcess.py
python killNodes.py 
python killNodesOnDilbert.py 


printTestBanner "Transaction-8"
python $pythonCode $nodeAddress ./nodeIntensive-noValidation/transaction-8.txt &
python $pythonCode $nodeAddress $transaction_insert &
python $pythonCode $nodeAddress $transaction_query &
sleep 120
python killPythonProcess.py
python killNodes.py 
python killNodesOnDilbert.py 


printTestBanner "Transaction-9"
python $pythonCode $nodeAddress ./nodeIntensive-noValidation/transaction-9.txt &
python $pythonCode $nodeAddress $transaction_insert &
python $pythonCode $nodeAddress $transaction_query &
sleep 120
python killPythonProcess.py
python killNodes.py 
python killNodesOnDilbert.py 


printTestBanner "Transaction-10"
python $pythonCode $nodeAddress ./nodeIntensive-noValidation/transaction-10.txt &
python $pythonCode $nodeAddress $transaction_insert &
python $pythonCode $nodeAddress $transaction_query &
sleep 120
python killPythonProcess.py
python killNodes.py 
python killNodesOnDilbert.py 


printTestBanner "sleepyTransaction-1"
python $pythonCode $nodeAddress ./nodeIntensive-noValidation/sleepyTransaction-1.txt &
python $pythonCode $nodeAddress $transaction1_insert &
python $pythonCode $nodeAddress $transaction1_query &
python $pythonCode $nodeAddress $transaction1_delete &
sleep 400
python killPythonProcess.py
python killNodes.py 
python killNodesOnDilbert.py 


printTestBanner "sleepyTransaction-2"
python $pythonCode $nodeAddress ./nodeIntensive-noValidation/sleepyTransaction-2.txt &
python $pythonCode $nodeAddress $transaction2_insert &
python $pythonCode $nodeAddress $transaction2_query &
python $pythonCode $nodeAddress $transaction2_delete &
sleep 400
python killPythonProcess.py
python killNodes.py 
python killNodesOnDilbert.py 


printTestBanner "sleepyTransaction-3"
python $pythonCode $nodeAddress ./nodeIntensive-noValidation/sleepyTransaction-3.txt &
python $pythonCode $nodeAddress $transaction3_insert &
python $pythonCode $nodeAddress $transaction3_query &
python $pythonCode $nodeAddress $transaction3_delete &
sleep 400
python killPythonProcess.py
python killNodes.py 
python killNodesOnDilbert.py 


printTestBanner "sleepyTransaction-4"
python $pythonCode $nodeAddress ./nodeIntensive-noValidation/sleepyTransaction-4.txt &
python $pythonCode $nodeAddress $transaction4_insert_delete &
python $pythonCode $nodeAddress $transaction4_query &
sleep 400
python killPythonProcess.py
python killNodes.py 
python killNodesOnDilbert.py 


printTestBanner "sleepyTransaction-5"
python $pythonCode $nodeAddress ./nodeIntensive-noValidation/sleepyTransaction-5.txt &
python $pythonCode $nodeAddress $transaction_insert &
python $pythonCode $nodeAddress $transaction_query &
sleep 400
python killPythonProcess.py
python killNodes.py 
python killNodesOnDilbert.py 


printTestBanner "sleepyTransaction-6"
python $pythonCode $nodeAddress ./nodeIntensive-noValidation/sleepyTransaction-6.txt &
python $pythonCode $nodeAddress $transaction_insert &
python $pythonCode $nodeAddress $transaction_query &
sleep 400
python killPythonProcess.py
python killNodes.py 
python killNodesOnDilbert.py 


printTestBanner "sleepyTransaction-7"
python $pythonCode $nodeAddress ./nodeIntensive-noValidation/sleepyTransaction-7.txt &
python $pythonCode $nodeAddress $transaction_insert &
python $pythonCode $nodeAddress $transaction_query &
sleep 400
python killPythonProcess.py
python killNodes.py 
python killNodesOnDilbert.py 


printTestBanner "sleepyTransaction-8"
python $pythonCode $nodeAddress ./nodeIntensive-noValidation/sleepyTransaction-8.txt &
python $pythonCode $nodeAddress $transaction_insert &
python $pythonCode $nodeAddress $transaction_query &
sleep 400
python killPythonProcess.py
python killNodes.py 
python killNodesOnDilbert.py 


printTestBanner "sleepyTransaction-9"
python $pythonCode $nodeAddress ./nodeIntensive-noValidation/sleepyTransaction-9.txt &
python $pythonCode $nodeAddress $transaction_insert &
python $pythonCode $nodeAddress $transaction_query &
sleep 400
python killPythonProcess.py
python killNodes.py 
python killNodesOnDilbert.py 


printTestBanner "sleepyTransaction-10"
python $pythonCode $nodeAddress ./nodeIntensive-noValidation/sleepyTransaction-10.txt &
python $pythonCode $nodeAddress $transaction_insert &
python $pythonCode $nodeAddress $transaction_query &
sleep 400
python killPythonProcess.py
python killNodes.py 
python killNodesOnDilbert.py 

