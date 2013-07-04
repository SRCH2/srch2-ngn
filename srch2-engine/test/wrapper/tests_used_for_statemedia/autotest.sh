#!/bin/bash 

if [ "$1" = "" ]; then
    echo "ERROR: need to specify the version number"
    exit 1
fi

VERSION=$1
PORT=8082
binary=./../../../../build/search-server/srch2-search-server-$VERSION

pingServer(){
    info=$( curl -s http://localhost:$PORT/search?q=p | grep -o results)
    count=0
    while [  -z $info  ]; do
        if [ "$count" -eq 0 ]; then
            echo "waiting the server to be up"
        fi
        count=$[ $count + 1 ]
        sleep 1 
        info=$( curl -s http://localhost:$PORT/search?q=p | grep -o results)
    done
}

startServer(){
    $binary --config-file=conf.ini &
    PID=$!
    pingServer

    return $PID
}

endServer(){
    kill $1
    rm *.idx
    rm log.txt
    cd ..
}

writeCorrectnessTest(){
    cd writes_correctness_test
    startServer
    PID=$!
    node writes_correctness_test.js data/index.json_500_head data/index.json_500_tail
    endServer $PID
}

searchPrecisionTest(){
    cd search_precision_test
    startServer
    PID=$!
    node search_precision_test.js
    endServer $PID
}

largeInsertionTest(){
    cd large_insertion_test
    startServer
    PID=$!
    ruby large_insertion_test.rb
    endServer $PID
}

updateEndpointTest(){
    cd update_endpoint_test
    startServer
    PID=$!
    python update_endpoint_test.py
    endServer $PID
}

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
