#this test is used for exact A1
#using: python exact_A1.py queriesAndResults.txt

import sys, urllib2, json, time, subprocess, os, commands, signal

sys.path.insert(0, 'srch2lib')
import test_lib

port = '8087'
port2 = '8088'

serverHandles = []
nodes = []
nodesPath="./exact_a1/nodesList.txt" 
#Function of checking the results
def checkResult(query, responseJson,resultValue):
#    for key, value in responseJson:
#        print key, value
    isPass=1
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
    
#    print 'Query : ' + query
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
        nodes.append(node(nodeId, portNum, conf))
                 
    print "list of nodes:"
    for n in nodes:
        print "node Id: ", n.Id, " port number: ", n.portNo, "configuration: ", n.conf[0]

def startEngines():
    #for node in nodes:
    #Start the engine server
    for node in nodes:
        args = [ binary_path, '--config-file='+(node.conf[0]) ]
#        print args
        if test_lib.confirmPortAvailable(node.portNo[0]) == False:
            print 'Port ' + str(node.portNo[0]) + ' already in use - aborting'
            return -1

        print 'starting engine: ' + args[0] + ' ' + (node.conf[0])
        serverHandles.append(test_lib.startServer(args))
        test_lib.pingServer(node.portNo[0])
        
def testExactA1(queriesAndResultsPath, binary_path):
#    Start the engine server
#    args = [ binary_path, '--config-file=./exact_a1/conf.xml' ]
    
    #for n in nodes:
#    args = [ binary_path, '--config-file='+(nodes[0].conf[0])]
#    startEngines()

#    if test_lib.confirmPortAvailable(nodes[0].portNo[0]) == False:
#        print 'Port ' + str(port) + ' already in use - aborting'
#        return -1

#    print 'starting engine: ' + args[0] + ' ' + args[1]
#    print 'starting engine: ' + args[0] + ' ' + nodes[0].conf[0]
    
#    serverHandles.append(test_lib.startServer(args))

#    test_lib.pingServer(nodes[0].portNo[0])

    #construct the query
    failCount = 0
    f_in = open(queriesAndResultsPath, 'r')
    for line in f_in:
        #get the query keyword and results
        value=line.split('||')
        queryValue=value[0].split()
        resultValue=(value[1]).split()
        #construct the query
        query='http://localhost:' + port + '/search?'
        query = query + prepareQuery(queryValue) 
        #print query
        #do the query
        response = urllib2.urlopen(query).read()
        response_json = json.loads(response)

        #check the result
        failCount += checkResult(query, response_json['results'], resultValue )

#    test_lib.killServer(serverHandle)
    print '=============================='
    return failCount

def testExactA2(queriesAndResultsPath, binary_path):
    #Start the engine server
#    args = [ binary_path, '--config-file=./exact_a1/conf2.xml' ]

#    if test_lib.confirmPortAvailable(port2) == False:
#        print 'Port ' + str(port2) + ' already in use - aborting'
#        return -1

#    print 'starting engine: ' + args[0] + ' ' + args[1]
#    serverHandles.append(test_lib.startServer(args))

#    test_lib.pingServer(port2)

    #construct the query
    failCount = 0
    f_in = open(queriesAndResultsPath, 'r')
    for line in f_in:
        #get the query keyword and results
        value=line.split('||')
        queryValue=value[0].split()
        resultValue=(value[1]).split()
        #construct the query
        query='http://localhost:' + port2 + '/search?'
        query = query + prepareQuery(queryValue)
        #print query
        #do the query
        response = urllib2.urlopen(query).read()
        response_json = json.loads(response)

        #check the result
        failCount += checkResult(query, response_json['results'], resultValue )

#    test_lib.killServer(serverHandle)
    print '=============================='
    return failCount


if __name__ == '__main__':      
    #Path of the query file
    #each line like "trust||01c90b4effb2353742080000" ---- query||record_ids(results)
    parseNodes(nodesPath)
    binary_path = sys.argv[1]
    queriesAndResultsPath = sys.argv[2]
    startEngines()
    exitCode = testExactA1(queriesAndResultsPath, binary_path)
    exitCode2 = testExactA2(queriesAndResultsPath, binary_path)

    for i in range(len(serverHandles)):
        test_lib.killServer(serverHandles[i])

    os._exit(exitCode)
    os._exit(exitCode2)   
