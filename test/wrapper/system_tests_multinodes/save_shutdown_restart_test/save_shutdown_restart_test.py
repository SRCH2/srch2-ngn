#this test is used for exact A1
#using: python exact_A1.py queriesAndResults.txt

import sys, urllib2, json, time, subprocess, os, commands, signal, subprocess

sys.path.insert(0, 'srch2lib')
import test_lib

def testSaveShutdownRestart(binary_path):
    #Start the engine server
    binary= [ binary_path, './save_shutdown_restart_test/conf.xml', './save_shutdown_restart_test/conf-A.xml', './save_shutdown_restart_test/conf-B.xml' ]

    serverHandle = test_lib.startServer(binary)
    if serverHandle == None:
        return -1

    #save the index
    
    jsonResponse = test_lib.saveRequest()
    if jsonResponse['status'] != True:
        print "Save operation failed: " + response
        exit(-1)
    #shutdown use system kill 
    test_lib.kill9Server(serverHandle)

    #search a query for checking if the server is shutdown
    try:
        response = test_lib.searchRequest('q=good')
        print response
    except:
        print 'server has been shutdown'
    else:
        print 'server is not shutdown'
        exit(-1)
    #restart
    serverHandle = test_lib.startServer(binary)
    #search a query for checking if the server is shutdown
    response = test_lib.searchRequest('q=good')
    if response == 0:
        print 'server does not start'
        exit(-1)
    else:
        print 'server start'

    #export data to json
    #exportQuery='export?exported_data_file=exportData.json'
    #jsonResponse = test_lib.sendPutRequest(exportQuery, '')
    #if jsonResponse['log'][0]['export'] != "success":
    #    print "Export operation failed: " + response
    #    exit(-1)

    #get pid of srch2-search-server and kill the process

    #shutdown use restful API
    response = test_lib.shutdownRequest()
    print response
    import time
    time.sleep(2)

    #search a query for checking if the server is shutdown
    try:
        response = test_lib.searchRequest('q=good')
        print response
    except:
        print 'server has been shutdown'
    else:
        print 'server is not shutdown'
        exit(-1)
 
    #proc.send_signal(signal.SIGUSR1)
    print 'test pass'
    print '=============================='
    return 0

if __name__ == '__main__':      
    #Path of the query file
    #each line like "trust||01c90b4effb2353742080000" ---- query||record_ids(results)
    binary_path = sys.argv[1]
    exitCode = testSaveShutdownRestart(binary_path)
    os._exit(exitCode)
