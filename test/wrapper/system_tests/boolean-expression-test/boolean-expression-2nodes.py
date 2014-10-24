#this test is used for exact A1
#using: python exact_A1.py queriesAndResults.txt

import sys, urllib2, urllib, json, time, subprocess, os, commands, signal


sys.path.insert(0, 'srch2lib')
import test_lib

portA = '8087'
portB= '8089'

#Function of checking the results
def checkResult(query, responseJson,resultValue):
#    for key, value in responseJson:
#        print key, value
    isPass=1
    if  len(responseJson) == len(resultValue):
        for i in range(0, len(resultValue)):
            #print response_json['results'][i]['record']['id']
            if (resultValue.count(responseJson[i]['record']['id']) != 1):
                isPass=0
                print query+' test failed'
                print 'query results||given results'
                print 'number of results:'+str(len(responseJson))+'||'+str(len(resultValue))
                for i in range(0, len(responseJson)):
                    print str(responseJson[i]['record']['id'])+'||'+str(resultValue[i])
                break
    else:
        isPass=0
        print query+' test failed'
        print 'query results||given results'
        print 'number of results:'+str(len(responseJson))+'||'+str(len(resultValue))
        maxLen = max(len(responseJson),len(resultValue))
        for i in range(0, maxLen):
            if i >= len(resultValue):
                print str(responseJson[i]['record']['id'])+'||'
            elif i >= len(responseJson):
                print '  '+'||'+resultValue[i]
            else:
                print str(responseJson[i]['record']['id'])+'||'+str(resultValue[i])

    if isPass == 1:
        print  query +' test pass'
        return 0
    return 1



def testBooleanExpression(queriesAndResultsPath, binary_path):
    #Start the engine server (nodeA)
    args = [ binary_path, '--config-file=./boolean-expression-test/config.xml' ]

    if test_lib.confirmPortAvailable(portA) == False:
        print 'Port ' + str(portA) + ' already in use - aborting'
        return -1

    print 'starting engine: ' + args[0] + ' ' + args[1]
    serverHandle1 = test_lib.startServer(args)
    test_lib.pingServer(portA)

    #Start the engine server (nodeB)
    args = [ binary_path, '--config-file=./boolean-expression-test/config-B.xml' ]

    if test_lib.confirmPortAvailable(portB) == False:
        print 'Port ' + str(portB) + ' already in use - aborting'
        return -1

    print 'starting engine: ' + args[0] + ' ' + args[1]
    serverHandle2 = test_lib.startServer(args)
    test_lib.pingServer(portB)
    time.sleep(5)
    #construct the query
    #format : phrase,proximity||rid1 rid2 rid3 ...ridn
    failCount = 0
    f_in = open(queriesAndResultsPath, 'r')
    for line in f_in:
        value=line.split('||')
        phrase=value[0]
        expectedRecordIds=(value[1]).split()
        query='http://localhost:' + portB + '/search?q='+ urllib.quote(phrase)
        print query
        try:
            response = urllib2.urlopen(query).read()
            response_json = json.loads(response)
        except urllib2.HTTPError as err:
            failCount += 1
        #print response_json['results']
        #check the result
        failCount += checkResult(query, response_json['results'], expectedRecordIds)

    test_lib.killServer(serverHandle1)
    test_lib.killServer(serverHandle2)
    print '=============================='
    return failCount

if __name__ == '__main__':      
    #Path of the query file
    binary_path = sys.argv[1]
    queriesAndResultsPath = sys.argv[2]
    exitCode = testBooleanExpression(queriesAndResultsPath, binary_path)
    os._exit(exitCode)
