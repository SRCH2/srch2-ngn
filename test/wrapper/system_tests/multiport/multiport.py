#! /usr/bin/python

# Test case to test multi-port functionality
# The configuration file for this test case specifies 2 different cores, each with a different
# data source.  Three search terms are tested, each expected to be returned by one and only one
# of the cores.  The usual syntax of the queriesAndResults.txt file has been extended to the
# following format:
#    <search-term>||<core1 ID result set>@<core2 ID result set>@<core3 ID result set>
# where each ID result set is a space separated list of record IDs expected from the server.

# Specifically:
#
# Global ports:
#         /info -> 8088
#         /[other entrypoints] -> 8087
#
# Core 1: Movies, using global ports
#         /info -> 8088
#         /[other entrypoints] -> 8087
#
# Core 2: StackOverflow data
#         /save -> 9087
#         /export -> 9087
#         /resetLogger -> 9087
#         /docs -> 9087
#         /update -> 9087
# 
# In the test case, we send HTTP requests to those core-ports. Based on the configuration, some of 
# the requests should succeed, and some should fail.
#

import sys, urllib2, json, time, subprocess, os, commands, signal, re

sys.path.insert(0, 'srch2lib')
import test_lib

port = '8087' # core1
infoPort = '8088' # core1 - /info
core2ControlPort = '9087' # core2 - all the control messages

#Function of checking the results
def checkResult(query, responseJson,resultValue):
#    for key, value in responseJson:
#        print key, value
    isPass=1
    if  len(responseJson) == len(resultValue):
        for i in range(0, len(resultValue)):
            #print response_json['results'][i]['record']['id']
            if int(responseJson[i]['record']['id']) != int(resultValue[i]):
                isPass=0
                print query+' test failed'
                print 'query results||given results'
                print 'number of results:'+str(len(responseJson))+'||'+str(len(resultValue))
                for i in range(0, len(responseJson)):
                    print str(responseJson[i]['record']['id']) + '||' + resultValue[i]
                break
    else:
        isPass=0
        print query+' test failed - differing response lengths'
        print 'query results||given results'
        print 'number of results:'+str(len(responseJson))+'||'+str(len(resultValue))
        maxLen = max(len(responseJson),len(resultValue))
        for i in range(0, maxLen):
            if i >= len(resultValue):
                print str(responseJson[i]['record']['id'])+'||'
            elif i >= len(responseJson):
                print '  '+'||'+resultValue[i]
            else:
                print responseJson[i]['record']['id']+'||'+resultValue[i]

    if isPass == 1:
        print  query+' test pass'
        return 0
    return 1


#prepare the query based on the valid syntax
def prepareQuery(queryKeywords, fuzzy):
    query = ''
    #################  prepare main query part
    query = query + 'q='
    # local parameters
#    query = query + '%7BdefaultPrefixComplete=COMPLETE%7D'
    # keywords section
    for i in range(0, len(queryKeywords)):
        if fuzzy:
            keyword = queryKeywords[i] + '~'
        else:
            keyword = queryKeywords[i]

        if i == (len(queryKeywords)-1):
            query=query+keyword # last keyword prefix
        else:
            query=query+keyword+'%20AND%20'
    
#    print 'Query : ' + query
    ##################################
    return query
    


def testMultipleCores(queriesAndResultsPath, binary_path):
    #Start the engine server
    args = [ binary_path, '--config-file=./multiport/conf-multiport.xml' ]
    print 'starting engine: ' + args[0] + ' ' + args[1]
    serverHandle = test_lib.startServer(args)

    test_lib.pingServer(port)
    failCount = 0

    #######################################
    # Basic multi-core functional testing #
    #######################################

    print "Test suite #1 - basic multi-core functionality"
    f_in = open(queriesAndResultsPath, 'r')
    for line in f_in:
        #get the query keyword and results
        value=line.split('||')
        queryValue=value[0].split()
        allResults=value[1].split('@')

        coreNum=0
        for coreResult in allResults:
            resultValue=coreResult.split()
            #construct the query
            if coreNum == 0:
                # test default core (unnamed core) on 0th iteration
                query='http://localhost:' + port + '/search?'
            else:
                query='http://localhost:' + port + '/core' + str(coreNum) + '/search?'
            query = query + prepareQuery(queryValue, False)

            #do the query
            response = urllib2.urlopen(query).read()
            #print query + ' Got ==> ' + response

            response_json = json.loads(response)

            #check the result
            failCount += checkResult(query, response_json['results'], resultValue)

            coreNum += 1

    print "\nTest suite #2: Port security"
    query='http://localhost:' + infoPort + '/info'
    #do the query
    #print query
    response = urllib2.urlopen(query).read()
    #print response
    response_json = json.loads(response)
    if len(response_json) > 0:
        if int(response_json['engine_status']['docs_in_index']) != 244:
            failCount += 1
            print "Info request did not return expected document count: Got " + str(response_json['engine_status']['docs_in_index']) + " but expected 244."
        else:
            print query + ' test pass'
    else:
        failCount += 1
        print "Null response to info request"

        
    print "\nTest suite #3: Control Port security"
    # /save test
    query='http://localhost:' + core2ControlPort + '/core2/save'
    opener = urllib2.build_opener(urllib2.HTTPHandler)
    request = urllib2.Request(query, '')
    #request.add_header('Content-Type', 'your/contenttype')
    request.get_method = lambda: 'PUT'
    #do the query
    print query
    response = opener.open(request).read()
    # response = urllib2.urlopen(request).read()
    #print response
    response_json = json.loads(response)
    if len(response_json) > 0:
        if response_json['log'][0]['save'] != 'success':
            failCount += 1
            print "/save request did not return success"
        else:
            print query + ' test pass'
    else:
        failCount += 1
        print "Null response to info request"

    # /export
    query='http://localhost:' + core2ControlPort + '/core2/export?exported_data_file=core2-exported.json'
    opener = urllib2.build_opener(urllib2.HTTPHandler)
    request = urllib2.Request(query, '')
    #request.add_header('Content-Type', 'your/contenttype')
    request.get_method = lambda: 'PUT'
    #do the query
    print query
    response = opener.open(request).read()
    # response = urllib2.urlopen(request).read()
    #print response
    response_json = json.loads(response)
    if len(response_json) > 0:
        if response_json['log'][0]['export'] != 'success':
            failCount += 1
            print "/export request did not return success"
        else:
            print query + ' test pass'
    else:
        failCount += 1
        print "Null response to save request"

    # /resetLogger test
    query='http://localhost:' + core2ControlPort + '/core2/resetLogger'
    opener = urllib2.build_opener(urllib2.HTTPHandler)
    request = urllib2.Request(query, '')
    #request.add_header('Content-Type', 'your/contenttype')
    request.get_method = lambda: 'PUT'
    #do the query
    print query
    response = opener.open(request).read()
    # response = urllib2.urlopen(request).read()
    #print response
    response_json = json.loads(response)
    if len(response_json) > 0:
        if response_json['log']:
            print query + ' test pass'
        else:
            failCount += 1
            print "/resetLogger request did not return success"
    else:
        failCount += 1
        print "Null response to resetLogger request"


        
    test_lib.killServer(serverHandle)

    print '=============================='
    return failCount

if __name__ == '__main__':      
    #Path of the query file
    #each line like "trust||01c90b4effb2353742080000" ---- query||record_ids(results)
    binary_path = sys.argv[1]
    queriesAndResultsPath = sys.argv[2]
    exitCode = testMultipleCores(queriesAndResultsPath, binary_path)
    os._exit(exitCode)
