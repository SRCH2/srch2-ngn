#!/bin/bash 
SRCH2_ENGINE=$1
PORT=8087


pingServer(){
    timeout=15
    if [ $# -ge 1 ]; then
	timeout=$1
    fi
    info=$( curl -s http://localhost:$PORT/search?q=p | grep -o results)
    count=0
    while [ $timeout -ge 0 ] && [ -z "$info"  ]; do
        if [ "$count" -eq 0 ]; then
            echo "waiting the server to be up"
        fi
        count=$[ $count + 1 ]
        sleep 1 
	timeout=$[ $timeout - 1 ]
        info=$( curl -s http://localhost:$PORT/search?q=p | grep -o results)
    done
    if [ $timeout -lt 0 ]; then
	echo "WARNING: Timed out waiting for the server to start!"
    fi
}

startServer(){
    $SRCH2_ENGINE --config-file=./$1/conf.xml &
    PID=$!
    pingServer

    return $PID
}

endServer(){
    kill $1
    count=0
    alive=$(ps -ef|grep $1| grep -v "grep" | wc -l)
    while [ $alive == 1 ]; do
        if [ $count -gt 10 ]; then
            kill -9 $1
        fi
        sleep 1
        alive=$(ps -ef|grep $1| grep -v "grep" | wc -l)
        count=$[ $count + 1 ]
    done

    #rm ./data/tests_used_for_statemedia/*.idx
    #rm ./data/tests_used_for_statemedia/log.txt
}

writeCorrectnessTest(){
    #cd writes_correctness_test
    WORKING_DIR="tests_used_for_statemedia/writes_correctness_test"
    startServer $WORKING_DIR 
    PID=$!
    ${NODECMD:-node} $WORKING_DIR/writes_correctness_test.js $WORKING_DIR/data/index.json_500_head $WORKING_DIR/data/index.json_500_tail
    exitCode=`expr ${exitCode} + $?`
    echo "ExitCode after writes_correctness_test: ${exitCode}"
    endServer $PID
}

searchPrecisionTest(){
    #cd search_precision_test
    WORKING_DIR="tests_used_for_statemedia/search_precision_test"
    startServer $WORKING_DIR
    PID=$!
    ${NODECMD:-node} $WORKING_DIR/search_precision_test.js
    exitCode=`expr ${exitCode} + $?`
    echo "ExitCode after search_precision_test: ${exitCode}"
    endServer $PID
}

largeInsertionTest(){
    #cd large_insertion_test
    WORKING_DIR="tests_used_for_statemedia/large_insertion_test"
    startServer $WORKING_DIR
    PID=$!
    ruby $WORKING_DIR/large_insertion_test.rb $WORKING_DIR
    exitCode=`expr ${exitCode} + $?`
    echo "ExitCode after large_insertion_test: ${exitCode}"
    endServer $PID
}

updateEndpointTest(){
    #cd update_endpoint_test
    WORKING_DIR="tests_used_for_statemedia/update_endpoint_test"
    startServer $WORKING_DIR
    PID=$!
    python $WORKING_DIR/update_endpoint_test.py
    exitCode=`expr ${exitCode} + $?`
    echo "ExitCode after update_endpoint_test: ${exitCode}"
    endServer $PID
}

exitCode=0

echo -----------------------------------------------------
echo ------------------------ 1/4 ------------------------
echo -----------------------------------------------------
writeCorrectnessTest
echo -----------------------------------------------------
echo ------------------------ 2/4 ------------------------
echo -----------------------------------------------------
searchPrecisionTest
echo -----------------------------------------------------
echo ------------------------ 3/4 ------------------------
echo -----------------------------------------------------
largeInsertionTest
echo -----------------------------------------------------
echo ------------------------ 4/4 ------------------------
echo -----------------------------------------------------
updateEndpointTest

echo "ExitCode:  ${exitCode}"
exit "${exitCode}"
