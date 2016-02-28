#This test is testing the bug SRCN-524 https://srch2inc.atlassian.net/browse/SRCN-524
#It simulates the following scenario: 
#1: start with an empty data set, and commit the indexes. 
#2: prepare 100 records. For each record, the script inserts it to the engine, and immediately delete it. 
#3: The engine should crash on master branch after pull request https://bitbucket.org/srch2inc/srch2-ngn/pull-request/504/improving-the-chinese-tokenizer/diff

import sys, urllib2, json, os, shutil

sys.path.insert(0, 'srch2lib')
import test_lib

port = '7049'

#add an record
def addRecord(record):
    addQuery='http://localhost:' + str(port) + '/docs'
    try:
        opener = urllib2.build_opener(urllib2.HTTPHandler)
        request = urllib2.Request(addQuery, record)
        request.get_method = lambda: 'PUT'
        return json.loads( opener.open(request).read())
    except urllib2.HTTPError as e:
        return json.loads(e.read())

#delete an record
def deleteRecord(id):
    deleteQuery='http://localhost:' + str(port) + '/docs?id='+id
    try:
        opener = urllib2.build_opener(urllib2.HTTPHandler)
        request = urllib2.Request(deleteQuery, '')
        request.get_method = lambda: 'DELETE'
        return json.loads(opener.open(request).read())
    except urllib2.HTTPError as e:
        return json.loads(e.read())

def testReassignId(binary_path,jsonRecordsPath):
    #Start the engine server
    args = [ binary_path, '--config-file=./reassignid-during-delete/srch2-config.xml' ]

    if test_lib.confirmPortAvailable(port) == False:
        print 'Port ' + str(port) + ' already in use - aborting'
        return -1

    print 'starting engine: ' + args[0] + ' ' + args[1]
    serverHandle = test_lib.startServer(args)

    test_lib.pingServer(port)
    #load record
    f_test = open(jsonRecordsPath,'r')
    jsonRecords = json.loads(f_test.read())
    for record in jsonRecords:
        recordId = record['id']
        record = json.dumps(record)
        #Insert one record and delete it immediately
        addRecord(record)
        deleteRecord(recordId)

    test_lib.killServer(serverHandle)
    print 'test pass'
    print '=============================='
    return 0

if __name__ == '__main__':      
    if(os.path.exists("./reassignid-during-delete/reassignid-during-delete")):
        shutil.rmtree("./reassignid-during-delete/reassignid-during-delete")
    binary_path = sys.argv[1]
    jsonRecordsPath = sys.argv[2]
    exitCode = testReassignId(binary_path,jsonRecordsPath)
    if(os.path.exists("./reassignid-during-delete/reassignid-during-delete")):
        shutil.rmtree("./reassignid-during-delete/reassignid-during-delete")
    os._exit(exitCode)
