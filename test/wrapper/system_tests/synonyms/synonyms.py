# This test is used for synonym module and highlighting of synonyms 
# Data set used : data.json
# Schema: body, title, tag, id 
#
import sys, urllib2, urllib, json, time, subprocess, os, commands, signal
# these imports have test results.
import expectedResultsTermOffsetON, expectedResultsTermOffsetOFF, expectedResultsTermOffsetON_1
sys.path.insert(0, 'srch2lib')
import test_lib
import time
port = '8087'
expectedResults = [expectedResultsTermOffsetON.results, expectedResultsTermOffsetOFF.results, expectedResultsTermOffsetON_1.results]
#Function of checking the results
def checkResult(query, responseJson,resultValue, queryId):
#    for key, value in responseJson:
#        print key, value
    global testRunId 
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
            if len(responseJson[i]["snippet"]) == 0:
                print "snippet not generated for the record = " + str(responseJson[i]['record_id']) 
                isPass=0
                break;
            else:
                if  responseJson[i]["snippet"] != expectedResults[testRunId][queryId][i]:
                    print "Test run " + str(testRunId) + " query " + str(queryId) + " response record " + str(i) + " snippets mismatch !!"
                    print "expected >> " + str(expectedResults[testRunId][queryId][i])
                    print "generated >> " + str(responseJson[i]["snippet"])
                    isPass = 0
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

    if test_lib.confirmPortAvailable(port) == False:
        print 'Port ' + str(port) + ' already in use - aborting'
        return -1

    print 'starting engine: ' + args[0] + ' ' + args[1]
    serverHandle = test_lib.startServer(args)

    test_lib.pingServer(port)
    try:
        #construct the query
        #format : phrase,proximity||rid1 rid2 rid3 ...ridn
        failTotal = 0
        f_in = open(queriesAndResultsPath, 'r')
        queryId = 1
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
            #print queryId , ":"
            #print "["
            #for obj in response_json['results']:
            #    print obj['record_id']
            #    print obj['snippet']
            #    print ","
            #print "],"
            failTotal += checkResult(query, response_json['results'], expectedRecordIds, queryId)
            queryId += 1
        print '=============================='
        return failTotal
    finally:
        test_lib.killServer(serverHandle)

if __name__ == '__main__':      
    #Path of the query file
    binary_path = sys.argv[1]
    print '----------------------------------------------------'
    print 'case 1: synonym and highlighting with offset information'
    print '----------------------------------------------------'
    global testRunId
    testRunId = 0
    exitCode = runTest( './synonyms/queries.txt', binary_path, './synonyms/conf_term_offset_on.xml')
    print '----------------------------------------------------'
    print 'case 2: synonym and highlighting without offset information'
    print '----------------------------------------------------'
    testRunId = 1
    exitCode |= runTest( './synonyms/queries.txt', binary_path, './synonyms/conf_term_offset_off.xml')
    print '----------------------------------------------------'
    print 'case 3: synonym with phrase search and highlighting with offset information'
    print '----------------------------------------------------'
    testRunId = 2 
    exitCode |= runTest('./synonyms/queries1.txt' , binary_path, './synonyms/conf_term_offset_on_1.xml')
    os._exit(exitCode)
