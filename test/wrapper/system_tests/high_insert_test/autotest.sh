#!/bin/bash 
SRCH2_ENGINE_DIR=$1
PORT=8082
binary=$SRCH2_ENGINE_DIR/srch2-search-server

pingServer(){
    info=$( curl -s http://localhost:$PORT/search?q=p | grep -o results)
    count=0
    while [  -z "$info"  ]; do
        if [ "$count" -eq 0 ]; then
            echo "waiting the server to be up"
        fi
        count=$[ $count + 1 ]
        sleep 1 
        info=$( curl -s http://localhost:$PORT/search?q=p | grep -o results)
    done
}

startServer(){
    $binary --config-file=high_insert_test/conf.ini &
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
    rm ./high_insert_test/index -rf
    rm ./high_insert_test/log.txt
}

HighInsertTest(){
    #cd large_insertion_test
    startServer
    PID=$!
    bundle exec ruby ./high_insert_test/boom.rb
    endServer $PID
}

HighInsertTest

