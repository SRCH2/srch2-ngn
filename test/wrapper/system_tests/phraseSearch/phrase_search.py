# This test is used for phrase search test
# Data set used : ps-test.json
# Query files used : queries.txt, booleanQueries.txt
# Test cases:
# 1. Test exact phrase --> "term1 term2"   ( find term1 and term2 next to each other)
# 2. Test fuzzy phrase --> "term1 term2"~1 ( find term1 and term2 with slop 1)
# 3. Test exact phrase in particular attribute --> description:"term1 term2"
#                  ( find term1 and term2 next to each other in a description )
# 4. Test boolean expression including phrase -->  term1 and "term2 term3" etc.
# 5. Test top-k query return k rows with phrasesearch ( given there are more than k such matches) 
#

import sys, urllib2, urllib, json, time, subprocess, os, commands, signal

sys.path.insert(0, 'srch2lib')
import test_lib

port = '8087'

#Function of checking the results
def checkResult(query, responseJson,resultValue):
#    for key, value in responseJson:
#        print key, value
    isPass=1
    if  len(responseJson) == len(resultValue):
        for i in range(0, len(resultValue)):
            #print response_json['results'][i]['record']['id']
            if str(responseJson[i]['record']['id']) !=  str(resultValue[i]):
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
        print  query+' test pass'
        return 0
    return 1


def testPhraseSearch(queriesAndResultsPath, binary_path):
    #Start the engine server
    args = [ binary_path, '--config-file=./phraseSearch/ps.xml' ]

    if test_lib.confirmPortAvailable(port) == False:
        print 'Port ' + str(port) + ' already in use - aborting'
        return -1

    print 'starting engine: ' + args[0] + ' ' + args[1]
    serverHandle = test_lib.startServer(args)

    test_lib.pingServer(port)

    #construct the query
    #format : phrase,proximity||rid1 rid2 rid3 ...ridn
    failTotal = 0
    f_in = open(queriesAndResultsPath, 'r')
    for line in f_in:
        value=line.split('||')
        phrase=value[0]
        expectedRecordIds=(value[1]).split()
        query='http://localhost:' + port + '/search?q='+ urllib.quote(phrase)
        print query
        response = urllib2.urlopen(query).read()
        response_json = json.loads(response)
        #print response_json['results']
        #check the result
        failTotal += checkResult(query, response_json['results'], expectedRecordIds)

    test_lib.killServer(serverHandle)
    print '=============================='
    return failTotal

if __name__ == '__main__':      
    #Path of the query file
    binary_path = sys.argv[1]
    queriesAndResultsPath = sys.argv[2]
    exitCode = testPhraseSearch(queriesAndResultsPath, binary_path)
    os._exit(exitCode)
