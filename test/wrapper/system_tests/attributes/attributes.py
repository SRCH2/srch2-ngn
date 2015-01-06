# This script tests large number of attributes in a record. 
# The schema of test data contains 699 attributes. 
# We fire two types of queries -
# 1) regular term query and 
# 2) attribute based (attr id = 97) query   

import sys, urllib2, json, time, subprocess, os, commands, signal

sys.path.insert(0, 'srch2lib')
import test_lib

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
            query=query+queryKeywords[i] # last keyword prefix
        else:
            query=query+queryKeywords[i]+'%20AND%20'
    return query
    
def testAttributes(queriesAndResultsPath, binary_path):
    #Start the engine server
    args = [ binary_path, './attributes/conf.xml','./attributes/conf-A.xml', './attributes/conf-B.xml' ]

    serverHandle = test_lib.startServer(args)
    if serverHandle == None:
        return -1
    time.sleep(15)

    #construct the query
    failCount = 0
    f_in = open(queriesAndResultsPath, 'r')
    for line in f_in:
        #get the query keyword and results
        value=line.split('||')
        queryValue=value[0].split()
        resultValue=(value[1]).split()
        #construct the query and do the query
        response_json = test_lib.searchRequest(prepareQuery(queryValue))
        #check the result
        failCount += checkResult(prepareQuery(queryValue), response_json['results'], resultValue )

    test_lib.killServer(serverHandle)
    print '=============================='
    return failCount

if __name__ == '__main__':      
    #Path of the query file
    #each line like "trust||01c90b4effb2353742080000" ---- query||record_ids(results)
    binary_path = sys.argv[1]
    queriesAndResultsPath = './attributes/queries.txt'
    exitCode = testAttributes(queriesAndResultsPath, binary_path)
    os._exit(exitCode)
