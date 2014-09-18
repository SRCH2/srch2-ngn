#!/bin/bash

PWD_DIR=$(pwd)

#if [ $# -lt 2 ]; then
#    echo "Usage: $0 [-f] <system-test-directory> <server-executable>"
#    exit 1
#fi



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

