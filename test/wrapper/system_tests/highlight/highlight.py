# This test is used for highlighter module 
# Data set used : yelp.json
# Schema: Single attribute : review text
# Query files used : queries.txt
# Each line in query try to test following cases:
#
#
import sys, urllib2, urllib, json, time, subprocess, os, commands, signal

sys.path.insert(0, 'srch2lib')
import test_lib
import time
port = '8087'

#Function of checking the results
def checkResult(query, responseJson,resultValue):
#    for key, value in responseJson:
#        print key, value
    isPass=1
    if  len(responseJson) == len(resultValue):
        print 'fetched rec#' + str(len(responseJson)) + '|expected rec#' + str(len(resultValue))
        for i in range(0, len(resultValue)):
            if str(responseJson[i]['record_id']) !=  str(resultValue[i]):
                isPass=0
                print query+' test failed'
                print 'query results||given results'
                print 'number of results:'+str(len(responseJson))+'||'+str(len(resultValue))
                for i in range(0, len(responseJson)):
                    print str(responseJson[i]['record_id'])+'||'+str(resultValue[i])
                break
            if len(responseJson[i]["snippet"]["text"]) == 0:
                print "snippet not generated for the record = " + str(responseJson[i]['record_id']) 
                isPass=0
                break;
    else:
        isPass=0
        print query+' test failed'
        print 'query results||given results'
        print 'number of results:'+str(len(responseJson))+'||'+str(len(resultValue))
        maxLen = max(len(responseJson),len(resultValue))
        for i in range(0, maxLen):
            if i >= len(resultValue):
                print responseJson[i]['record_id']+'||'
            elif i >= len(responseJson):
                print '  '+'||'+resultValue[i]
            else:
                print str(responseJson[i]['record_id'])+'||'+str(resultValue[i])

    if isPass == 1:
        print  query+' test pass'
        return 0
    return 1


def runTest(queriesAndResultsPath, binary_path, configFile):
    #Start the engine server
    args = [ binary_path, '--config-file=' + configFile] 
    print 'starting engine: ' + args[0] + ' ' + args[1]
    serverHandle = test_lib.startServer(args)

    test_lib.pingServer(port)
    try:
        #construct the query
        #format : phrase,proximity||rid1 rid2 rid3 ...ridn
        failTotal = 0
        f_in = open(queriesAndResultsPath, 'r')
        for line in f_in:
            value=line.split('||')
            phrase=value[0]
            phrase=phrase.replace(' ', '%20')
            phrase=phrase.replace('"', '%22')
            expectedRecordIds=(value[1]).split()
            query='http://localhost:' + port + '/search?q='+ phrase
            #query='http://localhost:' + port + '/search?q='+ urllib.quote(phrase)
            #query = query.replace("%26", "&");
            #query = query.replace("%3D", "=");
            print "query : " + query
            response = urllib2.urlopen(query).read()
            response_json = json.loads(response)
            #print response_json['results']
            #check the result
            failTotal += checkResult(query, response_json['results'], expectedRecordIds)
        print '=============================='
        return failTotal
    finally:
        test_lib.killServer(serverHandle)

if __name__ == '__main__':      
    #Path of the query file
    binary_path = sys.argv[1]
    queriesAndResultsPath = sys.argv[2]
    print '----------------------------------------------------'
    print 'case 1: snippet generation without offset information'
    print '----------------------------------------------------'
    exitCode = runTest(queriesAndResultsPath, binary_path, './highlight/highlighter_no_char_offset.xml')
    time.sleep(3);
    print '----------------------------------------------------'
    print 'case 2: snippet generation with offset information'
    print '----------------------------------------------------'
    exitCode |= runTest(queriesAndResultsPath, binary_path, './highlight/highlighter_with_char_offset.xml')
    os._exit(exitCode)
