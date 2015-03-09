#using python exact_Attribute_based_Search.py queriesAndResults.txt

import sys, urllib2, json, time, subprocess, os, commands,signal

sys.path.insert(0, 'srch2lib')
import test_lib

#the function of checking the results
def checkResult(query, responseJson, resultValue):
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
        # first extract the filters
        queryTermParts = queryKeywords[i].split(':')
        fieldFilter = ''
        if len(queryTermParts) == 2:
            fieldFilter = queryTermParts[1] + '%3A'
        keyword = queryTermParts[0]
        # now add them to the query
        if i == (len(queryKeywords)-1):
            query=query+fieldFilter+keyword+'*' # last keyword prefix
        else:
            query=query+fieldFilter+keyword+'%20AND%20'

    ################# fuzzy parameter
    query = query + '&fuzzy=false'

    #print 'Query : ' + query
    ##################################
    return query


def testExactAttributeBasedSearch(queriesAndResultsPath, args):
    # Start the engine server

    serverHandle = test_lib.startServer(args)
    if serverHandle == None:
        return -1

    #construct the query
    failCount = 0
    f_in = open(queriesAndResultsPath, 'r')
    for line in f_in:
        #get the query keyword and results
        value=line.split('||')
        queryValue=value[0].split()
        resultValue=(value[1]).split()
        #construct the query
        query = prepareQuery(queryValue)
        response_json = test_lib.searchRequest(query)
      
        #check the result
        failCount += checkResult(query, response_json['results'], resultValue )

    f_in.close()
    print '=============================='
    test_lib.killServer(serverHandle)
    return failCount

if __name__ == '__main__':     
    #Path of the query file
    #each line like "Alaska:name||3 89 8 10" ---- query||record_ids(results)
    binary_path = sys.argv[1]
    queriesAndResultsPath = sys.argv[2]

    args = [ binary_path, './exact_attribute_based_search/conf.xml', './exact_attribute_based_search/conf-A.xml', './exact_attribute_based_search/conf-B.xml']
    exitCode = testExactAttributeBasedSearch(queriesAndResultsPath, args)
    time.sleep(5) # give first server time to shutdown
    args = [ binary_path, './exact_attribute_based_search/conf_w_positional_info.xml', './exact_attribute_based_search/conf_w_positional_info-A.xml', './exact_attribute_based_search/conf_w_positional_info-B.xml']
    exitCode += testExactAttributeBasedSearch(queriesAndResultsPath, args)

    os._exit(exitCode)
