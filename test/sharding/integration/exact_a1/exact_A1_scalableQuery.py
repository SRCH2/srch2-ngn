#this test is used for exact A1
#using: python exact_A1.py queriesAndResults.txt

import sys, urllib2, json, time, subprocess, os, commands, signal

sys.path.insert(0, 'srch2lib')
import test_lib

port = '8087'
port2 = '8088'
nodes1 = dict()
serverHandles = []
nodes = []
nodesPath="./exact_a1/nodesList.txt" 
nodesAndQueriesResult="./exact_a1/nodes_queryFile.txt"
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
        nodes1[nodeId[0]] = node(nodeId[0], portNum[0], conf[0])
         
    print "list of nodes:"
    for n in nodes:
        print "node Id: ", n.Id, " port number: ", n.portNo, "configuration: ", n.conf[0]
        print "node ID in dictonary: ", nodes1[n.Id[0]].Id

def startEngines():
    #for node in nodes:
    #Start the engine server
    for key in nodes1:
        args = [ binary_path, '--config-file='+(nodes1[key].conf) ]
#        print args
        if test_lib.confirmPortAvailable(nodes1[key].portNo) == False:
            print 'Port ' + str(nodes1.portNo) + ' already in use - aborting'
            return -1

        print 'starting engine: ' + args[0] + ' ' + (nodes1[key].conf)
        serverHandles.append(test_lib.startServer(args))
        test_lib.pingServer(nodes1[key].portNo)
        

def testExactA1Batch(queriesAndResultsPath, binary_path, node):
    #construct the query
    failCount = 0
    f_in = open(queriesAndResultsPath, 'r')
    for line in f_in:
        #get the query keyword and results
        value=line.split('||')
        queryValue=value[0].split()
        resultValue=(value[1]).split()
        #construct the query
        query='http://localhost:' + node.portNo + '/search?'
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

def testExactA1Simultaneous(queriesAndResultsPath, binary_path):
    #construct the query
    failCount = 0
    f_in = open(queriesAndResultsPath, 'r')
    for line in f_in:
        #get the query keyword and results
        value=line.split('||')
        nodeId=value[0]
        queryValue=value[1].split()
        resultValue=(value[2]).split()
        #construct the query
        node=nodes1[nodeId[0]]
        query='http://localhost:' + nodes1[nodeId].portNo + '/search?'
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



def parseQuery(nodesAndQueriesResult):
    f_in = open(nodesAndQueriesResult)
    earr = []
    for line in f_in:
        value=line.split('||')
        nodeId=value[0].split()
        queryResultPath=value[1].split()
        exitcode=testExactA1Batch(queryResultPath[0], binary_path, nodes1[nodeId[0]])
        earr.append(exitcode)

    for i in range(len(serverHandles)):
        test_lib.killServer(serverHandles[i])

    for e in earr:
        os._exit(e)   

if __name__ == '__main__':      
    #Path of the query file
    parseNodes(nodesPath)
    binary_path = sys.argv[1]
    queriesAndResultsPath = sys.argv[2]
    startEngines()

    #For testing the two nodes simultaneously call the function testExactA1Simultaneous()
    exitCode=testExactA1Simultaneous(queriesAndResultsPath, binary_path)
   
    #For batch testing call the function parseQuery()
    #parseQuery(nodesAndQueriesResult)

    for i in range(len(serverHandles)):
        test_lib.killServer(serverHandles[i])

    os._exit(exitCode)
