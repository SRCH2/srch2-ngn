#! /usr/bin/python

# Test case to test multi-core functionality
# The configuration file for this test case specifies 4 different cores.  The first 3 have a
# different data source.  Three search terms are tested, each expected to be returned by one
# and only one of the cores.  The usual syntax of the queriesAndResults.txt file has been
# extended to the following format:
#     <search-term>||<core1 ID result set>@<core2 ID result set>@<core3 ID result set>
# where each ID result set is a space separated list of record IDs expected from the server.

import sys, urllib2, json, time, subprocess, os, commands, signal, re

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
            if int(responseJson[i]['record']['id']) !=  int(resultValue[i]):
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
    


def testMultipleCores(queriesAndResultsPath, queriesAndResultsPath2, binary_path):
    #Start the engine server
    args = [ binary_path, '--config-file=./multicore/conf-multicore.xml' ]

    if test_lib.confirmPortAvailable(port) == False:
        print 'Port ' + str(port) + ' already in use - aborting'
        return -1

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

            # TODO - Replace srch2 bad JSON (spurious comma).  Ticket SRCN-335 already filed.
            #response = re.sub('[,][}]', '}', response)
            #print query + ' Got ==> ' + response

            response_json = json.loads(response)

            #check the result
            failCount += checkResult(query, response_json['results'], resultValue)

            coreNum += 1

    #######################################################################################
    # Core 1 and Core 4 have different configurations, but on the same data.              #
    # We now test for the differences in those settings.                                  #
    # In queriesAndResults2.txt, here is an explanation of each test:                     #
    # 1) Aviatro||@156001 693000                                                          #
    #    Fuzzy match is off in core1 and on in core4, so only core4 should                #
    #    return any result records                                                        #
    # 2) Aviat||@156001 693000                                                            #
    #    Core1 has prefix matching off and core4 allows prefixes to match,                #
    #    so only core4 should return any results                                          #
    # 3) monkeys||135001@                                                                 #
    #    Core4 uses stop-words2.txt which has "monkeys", so core4 should not              #
    #    return results.  Core1 has the usual stop words file and should                  #
    #    find "monkeys".                                                                  #
    # 4) Rings||908 927 492002 492003 634004 634005 634006@908 927 492002 492003 634004   #
    #    Test different <rows> setting.  Core1 can return up to 10 records, but Core4 is  #
    #    limited to just 5.                                                               #
    # 5) martn~||156001 525017 693000@                                                    #
    #    Core1 will fuzzy match martn against "Martin" in 3 records, because              #
    #    it's similarity threshold is 0.75.  Core4 has a higher threshold                 #
    #    of 0.85, and should not return any matching records.                             #
    #######################################################################################

    print "\nTest suite #2: Comparing different engine configurations on the same data source"
    f_in = open(queriesAndResultsPath2, 'r')
    for line in f_in:
        #get the query keyword and results
        value=line.split('||')
        queryValue=value[0].split()
        allResults=value[1].split('@')

        coreNum= [1,4]  # coreNum are the literal core numbers to use in path this time
        index = 0 # and index iterates coreNum
        for coreResult in allResults:
            resultValue=coreResult.split()
            #construct the query
            query='http://localhost:' + port + '/core' + str(coreNum[index]) + '/search?'
            query = query + prepareQuery(queryValue, False)

            #do the query
            response = urllib2.urlopen(query).read()

            # TODO - Replace srch2 bad JSON (spurious comma).  Ticket SRCN-335 already filed.
            #response = re.sub('[,][}]', '}', response)
            # print query + ' Got ==> ' + response

            response_json = json.loads(response)

            #check the result
            failCount += checkResult(query, response_json['results'], resultValue)

            index += 1

        # Test search_all functionality
        query='http://localhost:' + port + '/_all/search?' + prepareQuery(queryValue, False)
        response = urllib2.urlopen(query).read()
        response_json = json.loads(response)
        # Check the search_all result 
        index = 0
        for coreResult in allResults:
            resultValue = coreResult.split()
            coreName = 'core' + str(coreNum[index])
            failCount += checkResult(query, response_json[coreName]['results'], resultValue)
            index +=1

        
    #test_lib.killServer(serverHandle)

    print '=============================='
    return failCount

if __name__ == '__main__':      
    #Path of the query file
    #each line like "trust||01c90b4effb2353742080000" ---- query||record_ids(results)
    binary_path = sys.argv[1]
    queriesAndResultsPath = sys.argv[2]
    queriesAndResultsPath2 = sys.argv[3]
    exitCode = testMultipleCores(queriesAndResultsPath, queriesAndResultsPath2, binary_path)
    os._exit(exitCode)
