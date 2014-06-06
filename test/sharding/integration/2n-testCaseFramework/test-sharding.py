#This tests insertion, querying and deletion from multiple nodes

import sys, urllib2, json, time, subprocess, os, commands, signal

sys.path.insert(0, 'srch2lib')
import test_lib

nodes = dict()
serverHandles = []

#Function of checking the results
def checkResult(query, responseJson,resultValue):
#    for key, value in responseJson:
#        print key, value
    isPass=1
    print "responseJson is " + responseJson
    print "resultValue is " + responseValue

    if  len(responseJson) == len(resultValue):
        for i in range(0, len(resultValue)):
            #print response_json['results'][i]['record']['id']
            if responseJson[i]['record']['id'] !=  resultValue[i]:
                isPass=0
                print query+' test failed'
                print 'query results||given results'
                print 'number of results:'+str(len(responseJson))+'||'+str(len(resultValue))
                for i in range(0, len(responseJson)):
                    print responseJson[i]['record']['id']+'||'+resultValue[i]
                break
                raise 
    else:
        isPass=0
        print query+' test failed'
        print 'query results||given results'
        print 'number of results:'+str(len(responseJson))+'||'+str(len(resultValue))
        maxLen = max(len(responseJson),len(resultValue))
        for i in range(0, maxLen):
            if i >= len(resultValue):
             print responseJson[i]['record']['id']+'||'
            elif i >= len(responseJson):
             print '  '+'||'+resultValue[i]
            else:
             print responseJson[i]['record']['id']+'||'+resultValue[i]
        raise
    if isPass == 1:
        print  query+' test pass'
        return 0
    return 1

#prepare the query based on the valid syntax
def prepareQuery(queryKeywords):
    query = ''
    #################  prepare main query part
    query = query + 'q='
    # local parameters
    query = query + '%7BdefaultPrefixComplete=COMPLETE%7D'
    # keywords section
    for i in range(0, len(queryKeywords)):
        if i == (len(queryKeywords)-1):
            query=query+queryKeywords[i]+'*' # last keyword prefix
        else:
            query=query+queryKeywords[i]+'%20AND%20'

    ################# fuzzy parameter
    query = query + '&fuzzy=false'

    ##################################
    return query

class node:
    def __init__(self, nodeId, portNum, conf):
        self.portNo = portNum
        self.Id = nodeId
        self.conf = conf

def parseNodes(nodesPath):
    f_in = open(nodesPath, 'r')
    for line in f_in:
        value=line.split('||')
        nodeId=value[0].split()
        portNum=value[1].split()
        conf=value[2].split()
        nodes[nodeId[0]] = node(nodeId[0], portNum[0], conf[0])

def startEngines():
    #for node in nodes:
    #Start the engine server
    for key in nodes:
        args = [ binary_path, '--config-file='+(nodes[key].conf) ]
#        print args
        if test_lib.confirmPortAvailable(nodes[key].portNo) == False:
            print 'Port ' + str(nodes.portNo) + ' already in use - aborting'
            return -1

        print 'starting engine: ' + args[0] + ' ' + (nodes[key].conf)
        serverHandles.append(test_lib.startServer(args))
    for key in nodes:	
        test_lib.pingServer(nodes[key].portNo)

def testInsertAndQuery(queriesAndResultsPath, binary_path):    
    #construct the query
    failCount = 0
    f_in = open(queriesAndResultsPath, 'r')
    for line in f_in:
        #time.sleep(3)
        if ((line.strip() == '')or(line[0] == '#')):
            continue 
        #get the query keyword and results
        value=line.split('||')
        nodeId=value[0].split()
        operation=value[1].split()
        if(operation[0] == 'query'):
            queryValue=value[2].split()
            resultValue=(value[3]).split()
            numberOfResultsFound=(value[4]).split() 
            #construct the query
            query='http://localhost:' + nodes[nodeId[0]].portNo + '/search?'
            query = query + prepareQuery(queryValue) 
            qq = 'curl "'+ query +'"'
            status, output = commands.getstatusoutput(qq)
            flag1 = str(output).find(resultValue[0]);
            flag2 = str(output).find(numberOfResultsFound[0]);
            assert flag1 > -1, "querying failed - inconsistent output" 
            assert flag2 > -1, "Wrong number of results returned"

        if(operation[0] == 'insert'):
            insertValue=value[2]
            expectedValue=value[3].split()
            #test , insert a record
            command = 'curl "http://localhost:' + nodes[nodeId[0]].portNo + '/docs" -i -X PUT -d ' + '\''+insertValue+'\'';
            status, output = commands.getstatusoutput(command)
            flag = str(output).find(expectedValue[0]);
            assert flag > -1, 'Error, rid <no.> is not updated correctly!'
    
        if(operation[0] == 'delete'):
            deleteId=value[2].split()
            expectedValue=value[3].split()
            commandDelete = 'curl "http://localhost:'+nodes[nodeId[0]].portNo + '/docs?id='+deleteId[0]+'" -i -X DELETE';
            status, output = commands.getstatusoutput(commandDelete)
            flag = str(output).find(expectedValue[0]);
            assert flag > -1, 'Error file could not be deleted'
     
        if(operation[0] == 'update'):
            inputValue=value[2]
            expectedValue=value[3]
            command = 'curl "http://localhost:' + nodes[nodeId[0]].portNo + '/update" -i -X PUT -d ' + '\'' + inputValue + '\'';
            status, output = commands.getstatusoutput(command)
            flag = str(output).find(expectedValue[0]);
            assert flag > -1, 'Error, record could not be updated'
 
    return failCount

if __name__ == '__main__':
    #Path of the query file
    nodesPath = sys.argv[3]
    parseNodes(nodesPath)
    binary_path = sys.argv[1]
    queriesAndResultsPath = sys.argv[2]
    os.popen('rm -rf ./test-data/core1/*.idx')
    startEngines()
    try:
        exitCode=testInsertAndQuery(queriesAndResultsPath, binary_path)
        for i in range(len(serverHandles)):
            subprocess.Popen(['kill','-9',str(serverHandles[i].pid)])
        print '==========test-sharding passed=========='
        os._exit(exitCode)
    except:
        print '==========test-sharding failed=========='
        exitCode=1
        for i in range(len(serverHandles)):
            subprocess.Popen(['kill','-9',str(serverHandles[i].pid)])
        os._exit(exitCode)
