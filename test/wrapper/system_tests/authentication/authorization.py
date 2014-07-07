#this test is used for exact A1
#using: python exact_A1.py queriesAndResults.txt

import sys, urllib2, json, time, subprocess, os, commands, signal

sys.path.insert(0, 'srch2lib')
import test_lib

authKey='Hey'
port = '8087'

def confirmPortAvailable(port, OAuth) :
    query = 'http://localhost:' + str(port) + '/info?OAuth='+ OAuth
    opener = urllib2.build_opener(urllib2.HTTPHandler)
    request = urllib2.Request(query, '')
    try:
        response = opener.open(request).read()
    except urllib2.URLError as err:
        # err code is 111 on Ubuntu Linux and 61 on Mac (darwin)
        if hasattr(err, 'reason') and hasattr(err.reason, 'errno') and ((sys.platform == 'linux2' and err.reason.errno == 111) or (sys.platform == 'darwin' and err.reason.errno == 61)):
            return True # connection refused - port available
        return False # unexpected error response - nonetheless port must be in use
    return False # no error - port already in use



def pingServer(port, OAuth):
    query = 'q=march'
    timeout = 15
    info = 'curl -s \"http://localhost:' + str(port) + '/search?OAuth=' + OAuth + '&' + query + '\" | grep -q results'
    #print "Pinging with: " + info
    while timeout >= 0 and subprocess.call(info, shell=True) != 0:
        timeout -= 1
        time.sleep(1)
    #print 'server is built!'
    if timeout < 0:
        print "WARNING: Timed out waiting for the server to start!"
        return -1
    return 0


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
    
def testExactA1(queriesAndResultsPath, binary_path, authKey):
    #Start the engine server
    args = [ binary_path, '--config-file=./authentication/conf.xml', '--OAuth=file.txt']

    if confirmPortAvailable(port, authKey) == False:
        print 'Port ' + str(port) + ' already in use - aborting'
        return -1

    print 'starting engine: ' + args[0] + ' ' + args[1]
    serverHandle = test_lib.startServer(args)

    pingServer( port, authKey)

    #construct the query
    failCount = 0
    f_in = open(queriesAndResultsPath, 'r')
    for line in f_in:
        #get the query keyword and results
        value=line.split('||')
        queryValue=value[0].split()
        resultValue=(value[1]).split()
        #construct the query
        query='http://localhost:' + port + '/search?OAuth=' + authKey + '&'
        query = query + prepareQuery(queryValue) 
        #print query
        #do the query
        response = urllib2.urlopen(query).read()
        response_json = json.loads(response)

        #check the result
        failCount += checkResult(query, response_json['results'], resultValue )

    test_lib.killServer(serverHandle)
    print '=============================='
    return failCount

if __name__ == '__main__':      
    #Path of the query file
    #each line like "trust||01c90b4effb2353742080000" ---- query||record_ids(results)
    binary_path = sys.argv[1]
    queriesAndResultsPath = sys.argv[2]
    exitCode = testExactA1(queriesAndResultsPath, binary_path, authKey)
    os._exit(exitCode)
