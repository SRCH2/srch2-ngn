# This test is used for highlighter module 
# Data set used : yelp.json
# Schema: Single attribute : review text
# Query files used : queries.txt
# Each line in query try to test following cases:
# 1. Complete exact
# 2. Complete fuzzy
# 3. Complete AND Prefix ( no fuzzy)
# 4. Complete OR Prefix (no fuzzy)
# 5. prefix exact
# 6. prefix fuzzy
# 7. phrase
# 8. proximity phrase
# 9. Phrase
# 10. Phrase and complete term
#
import sys, urllib2, urllib, json, time, subprocess, os, commands, signal
# these imports have test results.
import expectedResultsAnalyzer,expectedResultsCharOffset,expectedResultsTags
sys.path.insert(0, 'srch2lib')
import test_lib
import time
port = '8087'
expectedResults = [expectedResultsAnalyzer.results, expectedResultsCharOffset.results, expectedResultsTags.results]
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
            if len(responseJson[i]["snippet"]["text"]) == 0:
                print "snippet not generated for the record = " + str(responseJson[i]['record_id']) 
                isPass=0
                break;
            else:
                if  responseJson[i]["snippet"]["text"] != expectedResults[testRunId][queryId][i]:
                    print "snippets mismatch !!"
                    print "expected >> " + expectedResults[testRunId][queryId][i]
                    print "generated >> " + responseJson[i]["snippet"]["text"]
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
            failTotal += checkResult(query, response_json['results'], expectedRecordIds, queryId)
            queryId += 1
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
    global testRunId
    testRunId = 0
    exitCode = runTest(queriesAndResultsPath, binary_path, './highlight/highlighter_no_char_offset.xml')
    time.sleep(3);
    print '----------------------------------------------------'
    print 'case 2: snippet generation with offset information'
    print '----------------------------------------------------'
    testRunId = 1
    exitCode |= runTest(queriesAndResultsPath, binary_path, './highlight/highlighter_with_char_offset.xml')
    time.sleep(3);
    print '----------------------------------------------------'
    print 'case 3: snippet generation with offset information but different highlighter tags'
    print '----------------------------------------------------'
    testRunId = 2
    exitCode |= runTest(queriesAndResultsPath, binary_path, './highlight/highlighter_with_pre_post_tags.xml')
    os._exit(exitCode)
