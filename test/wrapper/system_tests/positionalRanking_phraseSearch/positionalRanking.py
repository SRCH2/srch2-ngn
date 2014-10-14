# Author: Prateek Verma
# Date: September 30th 2014
# 
# This test is used for testing positional ranking in phrase search queries
# Data set used : positionalRanking-test.json
# Query file used : queries.txt
# The three phrases used for querying the engine are: 
# sue alligator, foo bar, quick fox
# They occur multiple times in the file positionalRanking-test.json with different number of words in between them.
# The engine ranks the record higher which has larger sloppy frequency.
# The system test case validates if the ordering of the records are correct.
# It can be run from the directory: srch2-ngn/test/wrapper/system_tests using the following command
# python positionalRanking_phraseSearch/positionalRanking.py ../../../build/src/server/srch2-search-server positionalRanking_phraseSearch/queries.txt


import sys, urllib2, urllib, json, time, subprocess, os, commands, signal

sys.path.insert(0, 'srch2lib')
import test_lib

port = '8087'

#Function of checking the results
def checkResult(query, responseJson,resultValue, scores):
#    for key, value in responseJson:
#        print key, value
    isPass=1
    if  len(responseJson) == len(resultValue):
        for i in range(0, len(resultValue)):
            #score =  round(float(responseJson[i]['score']),2) 
            if (resultValue[i] != responseJson[i]['record']['id']):
                isPass=0
                print query+' test failed'
                print 'query results||given results'
                print 'number of results:'+str(len(responseJson))+'||'+str(len(resultValue))
                for i in range(0, len(responseJson)):
                    print str(responseJson[i]['record']['id'])+'||'+str(resultValue[i])
                    print "score: " + str(responseJson[i]['score']) + '||' + str(scores[i])
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
    args = [ binary_path, '--config-file=./positionalRanking_phraseSearch/conf-positionalRanking.xml' ]

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
        scores = (value[2]).split()
        query='http://localhost:' + port + '/search?q='+ urllib.quote(phrase)
        print query
        response = urllib2.urlopen(query).read()
        response_json = json.loads(response)
        #print response_json['results']
        #check the result
        failTotal += checkResult(query, response_json['results'], expectedRecordIds, scores)

    test_lib.killServer(serverHandle)
    print '=============================='
    return failTotal

if __name__ == '__main__':      
    #Path of the query file
    binary_path = sys.argv[1]
    queriesAndResultsPath = sys.argv[2]
    exitCode = testPhraseSearch(queriesAndResultsPath, binary_path)
    os._exit(exitCode)
