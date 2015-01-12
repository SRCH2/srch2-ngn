#this test is used for exact A1
#using: python exact_A1.py queriesAndResults.txt

import sys, urllib2, json, time, subprocess, os, commands, signal

sys.path.insert(0, 'srch2lib')
import test_lib

def testEmptyIndex(binary_path):
    #Start the engine server
    args = [ binary_path, './empty_index/conf.xml', './empty_index/conf-A.xml', './empty_index/conf-B.xml' ]

    serverHandle = test_lib.startServer(args)
    if serverHandle == None:
        return -1
    #add an record
    record = '{"id":"1234", "name":"Toy Story", "category":"shop"}'
    jsonResponse = test_lib.insertRequest(record)
    if jsonResponse['items'][0]['status'] != True:
        print "/docs operation failed: " + str(jsonResponse)
        return -1

    time.sleep(4)

    #do query
    exitCode = 0
    query = 'q=toy'
    response_json = test_lib.searchRequest(query)
    if len(response_json['results']) != 1 or response_json['results'][0]['record']['id'] != "1234":
       print 'test failed'
       return 1

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

