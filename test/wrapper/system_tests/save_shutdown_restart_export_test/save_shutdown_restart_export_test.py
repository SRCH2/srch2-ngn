#this test is used for exact A1
#using: python exact_A1.py queriesAndResults.txt

import sys, urllib2, json, time, subprocess, os, commands, signal, subprocess

sys.path.insert(0, 'srch2lib')
import test_lib

port = '8087'

def testSaveShutdownRestart(binary_path):
    #Start the engine server
    binary= [ binary_path, '--config-file=./save_shutdown_restart_export_test/conf.xml' ]
    print 'starting engine: ' + binary[0] + ' ' + binary[1]
    proc = subprocess.Popen(binary)

    test_lib.pingServer(port)

    #save the index
    saveQuery='http://localhost:' + port + '/save'
    opener = urllib2.build_opener(urllib2.HTTPHandler)
    request = urllib2.Request(saveQuery, '')
    request.get_method = lambda: 'PUT'
    response = opener.open(request).read()
    jsonResponse = json.loads(response)
    if jsonResponse['log'][0]['save'] != "success":
        print "Save operation failed: " + response
        exit(-1)
    
    #shutdown
    subprocess.call(["kill", "-2", "%d" % proc.pid])
    proc.wait()

    #search a query for checking if the server is shutdown
    try:
        query='http://localhost:' + port + '/search?q=good'
        response = urllib2.urlopen(query).read()
        print response
    except:
        print 'server has been shutdown'
    else:
        print 'server is not shutdown'
        exit(-1)
    #restart
    proc = subprocess.Popen(binary)
    test_lib.pingServer(port)
    #search a query for checking if the server is shutdown
    query='http://localhost:' + port + '/search?q=good'
    response = urllib2.urlopen(query).read()
    if response == 0:
        print 'server does not start'
        exit(-1)
    else:
        print 'server start'

    #export data to json
    exportQuery='http://localhost:' + port + '/export?exported_data_file=exportData.json'
    opener = urllib2.build_opener(urllib2.HTTPHandler)
    request = urllib2.Request(exportQuery, '')
    request.get_method = lambda: 'PUT'
    response = opener.open(request).read()
    jsonResponse = json.loads(response)
    if jsonResponse['log'][0]['export'] != "success":
        print "Export operation failed: " + response
        exit(-1)

    #get pid of srch2-search-server and kill the process
    proc.send_signal(signal.SIGUSR1)
    print 'test pass'
    print '=============================='
    return 0

if __name__ == '__main__':      
    #Path of the query file
    #each line like "trust||01c90b4effb2353742080000" ---- query||record_ids(results)
    binary_path = sys.argv[1]
    exitCode = testSaveShutdownRestart(binary_path)
    os._exit(exitCode)
