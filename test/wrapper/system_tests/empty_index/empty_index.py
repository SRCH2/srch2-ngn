#this test is used for exact A1
#using: python exact_A1.py queriesAndResults.txt

import sys, urllib2, json, time, subprocess, os, commands, signal

sys.path.insert(0, 'srch2lib')
import test_lib

port = '8087'

def testEmptyIndex(binary_path):
    #Start the engine server
    args = [ binary_path, '--config-file=./empty_index/conf.xml' ]
    print 'starting engine: ' + args[0] + ' ' + args[1]
    serverHandle = test_lib.startServer(args)

    test_lib.pingServer(port)
    #add an record
    addCommand='curl "http://localhost:' + str(port) + '/docs" -i -X PUT -d \'{"id":"1234", "name":"Toy Story", "category":"shop"}\''
    os.system(addCommand)

    time.sleep(11)

    #do query
    exitCode = 0
    query='http://localhost:' + str(port) + '/search?q=toy'
    response = urllib2.urlopen(query).read()
    response_json = json.loads(response)
    if len(response_json['results']) != 1 or response_json['results'][0]['record']['id'] != "1234":
       print 'test failed'
       exitCode = 1

    test_lib.killServer(serverHandle)
    print 'test pass'
    print '=============================='
    return exitCode

if __name__ == '__main__':      
    #Path of the query file
    #each line like "trust||01c90b4effb2353742080000" ---- query||record_ids(results)
    binary_path = sys.argv[1]
    exitCode = testEmptyIndex(binary_path)
    os._exit(exitCode)

