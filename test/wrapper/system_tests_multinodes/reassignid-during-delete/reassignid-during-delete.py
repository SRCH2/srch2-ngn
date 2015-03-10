#This test is testing the bug SRCN-524 https://srch2inc.atlassian.net/browse/SRCN-524
#It simulates the following scenario: 
#1: start with an empty data set, and commit the indexes. 
#2: prepare 100 records. For each record, the script inserts it to the engine, and immediately delete it. 
#3: The engine should crash on master branch after pull request https://bitbucket.org/srch2inc/srch2-ngn/pull-request/504/improving-the-chinese-tokenizer/diff

import sys, urllib2, json, os, shutil

sys.path.insert(0, 'srch2lib')
import test_lib

#add an record
def addRecord(record):
    test_lib.insertRequest(record)

#delete an record
def deleteRecord(id):
    test_lib.deleteRequest('id='+id)

def testReassignId(binary_path,jsonRecordsPath):
    #Start the engine server
    args = [ binary_path, './reassignid-during-delete/conf-1.xml', './reassignid-during-delete/conf-2.xml', './reassignid-during-delete/conf-3.xml' ]

    serverHandle = test_lib.startServer(args)
    if serverHandle == None:
        return -1

    #Load initial data
    dataFile = './reassignid-during-delete/stackoverflow-100.json.json'
    test_lib.loadIntialData(dataFile)

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
