#This function is doing the following things:
#Start the engine with conf1.xml. The engine will load the data and build the indexes
#Shutdown the engine.
#Start the engine with another config file conf2.xml. In conf2.xml, the schema has been changed (name  => nameChanged)
#The engine should raise a warning to tell the user that 'The schema in the config file has changed, remove all the index files and run it again'

import sys, urllib2, time, subprocess, os, commands, signal,shutil,json

sys.path.insert(0,'srch2lib')
import test_lib

port = '8087'
serverHandle = None
totalFailCount = 0
binary_path = None

#Start the SRCH2 engine with sqlite config file.
def startSrch2Engine(confPath):
    global serverHandle
    #Start the engine server
    args = [binary_path , '--config-file='+confPath]

    if test_lib.confirmPortAvailable(port) == False:
        print 'Port' + str(port) + ' already in use -aborting '
        return -1

    print 'starting engine: ' + args[0] + ' ' + args[1]
    serverHandle = test_lib.startServer(args)
    test_lib.pingServer(port)

#Shut down the srch2 engine
def shutdownSrch2Engine():
    global serverHandle
    #Shutdown the engine server
    test_lib.killServer(serverHandle)

#Function of checking the results
def checkResult(query, responseJson,resultValue,checkField):
#    for key, value in responseJson:
#        print key, value
    isPass=1
    if  len(responseJson) == len(resultValue):
        for i in range(0, len(resultValue)):
            #print response_json['results'][i]['record'][checkField]
            if responseJson[i]['record'][checkField] !=  resultValue[i]:
                isPass=0
                print query+' test failed'
                print 'query results||given results'
                print 'number of results:'+str(len(responseJson))+'||'+str(len(resultValue))
                for i in range(0, len(responseJson)):
                    print responseJson[i]['record'][checkField]+'||'+resultValue[i]
                break
    else:
        isPass=0
        print query+' test failed'
        print 'query results||given results'
        print 'number of results:'+str(len(responseJson))+'||'+str(len(resultValue))
        maxLen = max(len(responseJson),len(resultValue))
        for i in range(0, maxLen):
            if i >= len(resultValue):
             print responseJson[i]['record'][checkField]+'||'
            elif i >= len(responseJson):
             print '  '+'||'+resultValue[i]
            else:
             print responseJson[i]['record'][checkField]+'||'+resultValue[i]

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

def testRefiningAttrType(queriesAndResultsPath):
    #construct the query
    global totalFailCount
    failCount = 0
    f_in = open(queriesAndResultsPath, 'r')
    for line in f_in:
        #get the query keyword and results
        value=line.split('||')
        queryValue=value[0].split()
        fieldValue=value[1].split()
        resultValue=value[2].split()
        #construct the query
        query='http://localhost:' + port + '/search?'
        query = query + prepareQuery(queryValue) 
        #print query
        #do the query
        response = urllib2.urlopen(query).read()
        response_json = json.loads(response)

        #check the result
        failCount += checkResult(query, response_json['results'], resultValue,fieldValue[0])	

        totalFailCount = failCount

if __name__ == '__main__':
    if(os.path.exists("data")):
        shutil.rmtree("data")
    #Start the test cases
    binary_path = sys.argv[1]
    startSrch2Engine('refining_attr_type/conf.xml')
    testRefiningAttrType('refining_attr_type/queriesAndResults.txt')
    shutdownSrch2Engine()
    time.sleep(2) # sleep to wait for the engine to shutdown
    print '=============================='
    
    if(os.path.exists("data")):
        shutil.rmtree("data")
    os._exit(totalFailCount)
